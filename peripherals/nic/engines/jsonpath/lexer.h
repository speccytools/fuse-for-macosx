/*
 * Copyright (C) 2013-2014 Jo-Philipp Wich
 * SPDX-License-Identifier: ISC
 */
#pragma once

#include "ast.h"

extern const char *tokennames[25];

struct jp_opcode *jp_get_token(struct jp_state *s, const char *input, int *mlen);
