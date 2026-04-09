/*
 * Copyright (C) 2013-2014 Jo-Philipp Wich
 * SPDX-License-Identifier: ISC
 */
#pragma once

#include <stddef.h>
#include <stdarg.h>

struct jp_opcode
{
    int type;
    struct jp_opcode *next;
    struct jp_opcode *down;
    struct jp_opcode *sibling;
    char *str;
    int num;
};

struct jp_state
{
    struct jp_opcode *pool;
    struct jp_opcode *path;
    int error_pos;
    int error_code;
    int off;
};

static inline struct jp_opcode *append_op(struct jp_opcode *a, struct jp_opcode *b)
{
    struct jp_opcode *tail = a;

    while (tail->sibling)
        tail = tail->sibling;

    tail->sibling = b;

    return a;
}

struct jp_opcode *jp_alloc_op(struct jp_state *s, int type, int num, char *str, ...);
struct jp_state *jp_parse(const char *expr);
void jp_free(struct jp_state *s);

void *ParseAlloc(void *(*mfunc)(size_t));
void Parse(void *pParser, int type, struct jp_opcode *op, struct jp_state *s);
void ParseFree(void *pParser, void (*ffunc)(void *));
