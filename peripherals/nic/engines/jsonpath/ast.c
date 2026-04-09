/*
 * Copyright (C) 2013-2014 Jo-Philipp Wich
 * SPDX-License-Identifier: ISC
 */

#include "ast.h"
#include "lexer.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

struct jp_opcode *jp_alloc_op(struct jp_state *s, int type, int num, char *str, ...)
{
    va_list ap;
    struct jp_opcode *newop;
    struct jp_opcode *child;

    newop = (struct jp_opcode *)calloc(1, sizeof(*newop));
    if (!newop)
        return NULL;

    newop->type = type;
    newop->num = num;

    if (str)
    {
        newop->str = strdup(str);
        if (!newop->str)
        {
            free(newop);
            return NULL;
        }
    }

    va_start(ap, str);

    while ((child = va_arg(ap, struct jp_opcode *)) != NULL)
    {
        if (!newop->down)
            newop->down = child;
        else
            append_op(newop->down, child);
    }

    va_end(ap);

    newop->next = s->pool;
    s->pool = newop;

    return newop;
}

void jp_free(struct jp_state *s)
{
    struct jp_opcode *op;
    struct jp_opcode *tmp;

    for (op = s->pool; op;)
    {
        tmp = op->next;
        free(op->str);
        free(op);
        op = tmp;
    }

    free(s);
}

struct jp_state *jp_parse(const char *expr)
{
    struct jp_state *s;
    struct jp_opcode *op;
    const char *ptr = expr;
    void *pParser;
    int len = (int)strlen(expr);
    int mlen = 0;

    s = (struct jp_state *)calloc(1, sizeof(*s));
    if (!s)
        return NULL;

    pParser = ParseAlloc(malloc);
    if (!pParser)
    {
        free(s);
        return NULL;
    }

    while (len > 0)
    {
        op = jp_get_token(s, ptr, &mlen);

        if (mlen < 0)
        {
            s->error_code = mlen;
            goto out;
        }

        if (op)
            Parse(pParser, op->type, op, s);

        len -= mlen;
        ptr += mlen;

        s->off += mlen;
    }

    Parse(pParser, 0, NULL, s);

out:
    ParseFree(pParser, free);

    return s;
}
