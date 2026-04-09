/*
 * Copyright (C) 2013-2014 Jo-Philipp Wich
 * SPDX-License-Identifier: ISC
 *
 * JSON tree: Parson (JSON_Value)
 */

#include "matcher.h"
#include "parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static JSON_Value *jp_match_next(struct jp_opcode *ptr, JSON_Value *root, JSON_Value *cur, jp_match_cb_t cb,
                                 void *priv);

static bool jp_json_to_op(JSON_Value *obj, struct jp_opcode *op)
{
    switch (json_value_get_type(obj))
    {
        case JSONBoolean:
            op->type = T_BOOL;
            op->num = json_value_get_boolean(obj) ? 1 : 0;
            return true;

        case JSONNumber:
            op->type = T_NUMBER;
            op->num = (int)json_value_get_number(obj);
            return true;

        case JSONString:
            op->type = T_STRING;
            op->str = (char *)json_value_get_string(obj);
            return true;

        default:
            return false;
    }
}

static bool jp_resolve(JSON_Value *root, JSON_Value *cur, struct jp_opcode *op, struct jp_opcode *res)
{
    JSON_Value *val;

    switch (op->type)
    {
        case T_THIS:
            val = jp_match(op, cur, NULL, NULL);
            if (val)
                return jp_json_to_op(val, res);
            return false;

        case T_ROOT:
            val = jp_match(op, root, NULL, NULL);
            if (val)
                return jp_json_to_op(val, res);
            return false;

        default:
            *res = *op;
            return true;
    }
}

static bool jp_cmp(struct jp_opcode *op, JSON_Value *root, JSON_Value *cur)
{
    int delta;
    struct jp_opcode left, right;

    if (!jp_resolve(root, cur, op->down, &left) || !jp_resolve(root, cur, op->down->sibling, &right))
        return false;

    if (left.type != right.type)
        return false;

    switch (left.type)
    {
        case T_BOOL:
        case T_NUMBER:
            delta = left.num - right.num;
            break;

        case T_STRING:
            delta = strcmp(left.str, right.str);
            break;

        default:
            return false;
    }

    switch (op->type)
    {
        case T_EQ:
            return (delta == 0);
        case T_LT:
            return (delta < 0);
        case T_LE:
            return (delta <= 0);
        case T_GT:
            return (delta > 0);
        case T_GE:
            return (delta >= 0);
        case T_NE:
            return (delta != 0);
        default:
            return false;
    }
}

static bool jp_regmatch(struct jp_opcode *op, JSON_Value *root, JSON_Value *cur)
{
	(void)op;
	(void)root;
	(void)cur;
	/* ~ operator requires POSIX regex; not available on this target. */
	return false;
}

static bool jp_expr(struct jp_opcode *op, JSON_Value *root, JSON_Value *cur, int idx, const char *key,
                    jp_match_cb_t cb, void *priv)
{
    struct jp_opcode *sop;

    switch (op->type)
    {
        case T_WILDCARD:
            return true;

        case T_EQ:
        case T_NE:
        case T_LT:
        case T_LE:
        case T_GT:
        case T_GE:
            return jp_cmp(op, root, cur);

        case T_MATCH:
            return jp_regmatch(op, root, cur);

        case T_ROOT:
            return !!jp_match(op, root, NULL, NULL);

        case T_THIS:
            return !!jp_match(op, cur, NULL, NULL);

        case T_NOT:
            return !jp_expr(op->down, root, cur, idx, key, cb, priv);

        case T_AND:
            for (sop = op->down; sop; sop = sop->sibling)
                if (!jp_expr(sop, root, cur, idx, key, cb, priv))
                    return false;
            return true;

        case T_OR:
        case T_UNION:
            for (sop = op->down; sop; sop = sop->sibling)
                if (jp_expr(sop, root, cur, idx, key, cb, priv))
                    return true;
            return false;

        case T_STRING:
            return (key != NULL && !strcmp(op->str, key));

        case T_NUMBER:
            return (idx == op->num);

        default:
            return false;
    }
}

static JSON_Value *jp_match_expr(struct jp_opcode *ptr, JSON_Value *root, JSON_Value *cur, jp_match_cb_t cb,
                                 void *priv)
{
    JSON_Value *tmp;
    JSON_Value *res = NULL;

    switch (json_value_get_type(cur))
    {
        case JSONObject: {
            JSON_Object *obj = json_value_get_object(cur);
            const size_t n = json_object_get_count(obj);
            for (size_t i = 0; i < n; i++)
            {
                const char *k = json_object_get_name(obj, i);
                JSON_Value *val = json_object_get_value(obj, k);
                if (jp_expr(ptr, root, val, -1, k, cb, priv))
                {
                    tmp = jp_match_next(ptr->sibling, root, val, cb, priv);
                    if (tmp && !res)
                        res = tmp;
                }
            }
            break;
        }

        case JSONArray: {
            JSON_Array *arr = json_value_get_array(cur);
            const size_t len = json_array_get_count(arr);
            for (size_t idx = 0; idx < len; idx++)
            {
                JSON_Value *elem = json_array_get_value(arr, idx);
                if (jp_expr(ptr, root, elem, (int)idx, NULL, cb, priv))
                {
                    tmp = jp_match_next(ptr->sibling, root, elem, cb, priv);
                    if (tmp && !res)
                        res = tmp;
                }
            }
            break;
        }

        default:
            break;
    }

    return res;
}

static JSON_Value *jp_match_next(struct jp_opcode *ptr, JSON_Value *root, JSON_Value *cur, jp_match_cb_t cb,
                                 void *priv)
{
    int idx;
    JSON_Value *next = NULL;

    if (!ptr)
    {
        if (cb)
            cb(cur, priv);

        return cur;
    }

    switch (ptr->type)
    {
        case T_STRING:
        case T_LABEL:
            if (json_value_get_type(cur) == JSONObject)
            {
                JSON_Object *obj = json_value_get_object(cur);
                next = json_object_get_value(obj, ptr->str);
                if (next)
                    return jp_match_next(ptr->sibling, root, next, cb, priv);
            }
            break;

        case T_NUMBER:
            if (json_value_get_type(cur) == JSONArray)
            {
                JSON_Array *arr = json_value_get_array(cur);
                idx = ptr->num;
                if (idx < 0)
                    idx += (int)json_array_get_count(arr);

                if (idx >= 0 && (size_t)idx < json_array_get_count(arr))
                    next = json_array_get_value(arr, (size_t)idx);

                if (next)
                    return jp_match_next(ptr->sibling, root, next, cb, priv);
            }
            break;

        default:
            return jp_match_expr(ptr, root, cur, cb, priv);
    }

    return NULL;
}

JSON_Value *jp_match(struct jp_opcode *path, JSON_Value *jsobj, jp_match_cb_t cb, void *priv)
{
    if (path->type == T_LABEL)
        path = path->down;

    return jp_match_next(path->down, jsobj, jsobj, cb, priv);
}
