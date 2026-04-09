/*
 * Copyright (C) 2013-2014 Jo-Philipp Wich <jo@mein.io>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"


struct token {
	int type;
	const char *pat;
	int plen;
	int (*parse)(const char *buf, struct jp_opcode *op, struct jp_state *s);
};

#define dec(o) \
	((o) - '0')

#define hex(x) \
	(((x) >= 'a') ? (10 + (x) - 'a') : \
		(((x) >= 'A') ? (10 + (x) - 'A') : dec(x)))

/*
 * Stores the given codepoint as a utf8 multibyte sequence into the given
 * output buffer and substracts the required amount of bytes from  the given
 * length pointer.
 *
 * Returns false if the multibyte sequence would not fit into the buffer,
 * otherwise true.
 */

static bool
utf8enc(char **out, int *rem, int code)
{
	if (code > 0 && code <= 0x7F)
	{
		if (*rem < 1)
			return false;

		*(*out)++ = code; (*rem)--;
		return true;
	}
	else if (code > 0 && code <= 0x7FF)
	{
		if (*rem < 2)
			return false;

		*(*out)++ = ((code >>  6) & 0x1F) | 0xC0; (*rem)--;
		*(*out)++ = ( code        & 0x3F) | 0x80; (*rem)--;
		return true;
	}
	else if (code > 0 && code <= 0xFFFF)
	{
		if (*rem < 3)
			return false;

		*(*out)++ = ((code >> 12) & 0x0F) | 0xE0; (*rem)--;
		*(*out)++ = ((code >>  6) & 0x3F) | 0x80; (*rem)--;
		*(*out)++ = ( code        & 0x3F) | 0x80; (*rem)--;
		return true;
	}
	else if (code > 0 && code <= 0x10FFFF)
	{
		if (*rem < 4)
			return false;

		*(*out)++ = ((code >> 18) & 0x07) | 0xF0; (*rem)--;
		*(*out)++ = ((code >> 12) & 0x3F) | 0x80; (*rem)--;
		*(*out)++ = ((code >>  6) & 0x3F) | 0x80; (*rem)--;
		*(*out)++ = ( code        & 0x3F) | 0x80; (*rem)--;
		return true;
	}

	return true;
}


/*
 * Parses a string literal from the given buffer.
 *
 * Returns a negative value on error, otherwise the amount of consumed
 * characters from the given buffer.
 *
 * Error values:
 *  -1	Unterminated string
 *  -2	Invalid escape sequence
 *  -3	String literal too long
 */

static int
parse_string(const char *buf, struct jp_opcode *op, struct jp_state *s)
{
	char q = *(buf++);
	char str[128] = { 0 };
	char *out = str;
	const char *in = buf;
	bool esc = false;
	int rem = sizeof(str) - 1;
	int code;

	while (*in)
	{
		/* continuation of escape sequence */
		if (esc)
		{
			/* \uFFFF */
			if (in[0] == 'u')
			{
				if (isxdigit(in[1]) && isxdigit(in[2]) &&
				    isxdigit(in[3]) && isxdigit(in[4]))
				{
					if (!utf8enc(&out, &rem,
					             hex(in[1]) * 16 * 16 * 16 +
					             hex(in[2]) * 16 * 16 +
					             hex(in[3]) * 16 +
					             hex(in[4])))
					{
						s->error_pos = s->off + (in - buf);
						return -3;
					}

					in += 5;
				}
				else
				{
					s->error_pos = s->off + (in - buf);
					return -2;
				}
			}

			/* \xFF */
			else if (in[0] == 'x')
			{
				if (isxdigit(in[1]) && isxdigit(in[2]))
				{
					if (!utf8enc(&out, &rem, hex(in[1]) * 16 + hex(in[2])))
					{
						s->error_pos = s->off + (in - buf);
						return -3;
					}

					in += 3;
				}
				else
				{
					s->error_pos = s->off + (in - buf);
					return -2;
				}
			}

			/* \377, \77 or \7 */
			else if (in[0] >= '0' && in[0] <= '7')
			{
				/* \377 */
				if (in[1] >= '0' && in[1] <= '7' &&
				    in[2] >= '0' && in[2] <= '7')
				{
					code = dec(in[0]) * 8 * 8 +
					       dec(in[1]) * 8 +
					       dec(in[2]);

					if (code > 255)
					{
						s->error_pos = s->off + (in - buf);
						return -2;
					}

					if (!utf8enc(&out, &rem, code))
					{
						s->error_pos = s->off + (in - buf);
						return -3;
					}

					in += 3;
				}

				/* \77 */
				else if (in[1] >= '0' && in[1] <= '7')
				{
					if (!utf8enc(&out, &rem, dec(in[0]) * 8 + dec(in[1])))
					{
						s->error_pos = s->off + (in - buf);
						return -3;
					}

					in += 2;
				}

				/* \7 */
				else
				{
					if (!utf8enc(&out, &rem, dec(in[0])))
					{
						s->error_pos = s->off + (in - buf);
						return -3;
					}

					in += 1;
				}
			}

			/* single character escape */
			else
			{
				if (rem-- < 1)
				{
					s->error_pos = s->off + (in - buf);
					return -3;
				}

				switch (in[0])
				{
				case 'a': *out = '\a'; break;
				case 'b': *out = '\b'; break;
				case 'e': *out = '\e'; break;
				case 'f': *out = '\f'; break;
				case 'n': *out = '\n'; break;
				case 'r': *out = '\r'; break;
				case 't': *out = '\t'; break;
				case 'v': *out = '\v'; break;
				default:
					/* in regexp mode, retain backslash */
					if (q == '/')
					{
						if (rem-- < 1)
						{
							s->error_pos = s->off + (in - buf);
							return -3;
						}

						*out++ = '\\';
					}

					*out = *in;
					break;
				}

				in++;
				out++;
			}

			esc = false;
		}

		/* begin of escape sequence */
		else if (*in == '\\')
		{
			in++;
			esc = true;
		}

		/* terminating quote */
		else if (*in == q)
		{
			op->str = strdup(str);
			return (in - buf) + 2;
		}

		/* ordinary char */
		else
		{
			if (rem-- < 1)
			{
				s->error_pos = s->off + (in - buf);
				return -3;
			}

			*out++ = *in++;
		}
	}

	return -1;
}


/*
 * Parses a regexp literal from the given buffer.
 *
 * Returns a negative value on error, otherwise the amount of consumed
 * characters from the given buffer.
 *
 * Error values:
 *  -1	Unterminated regexp
 *  -2	Invalid escape sequence
 *  -3	Regexp literal too long
 */

static int
parse_regexp(const char *buf, struct jp_opcode *op, struct jp_state *s)
{
	(void)buf;
	(void)op;
	(void)s;
	/* POSIX regex is not linked on embedded newlib; JSONPath /re/ literals are unsupported. */
	return -1;
}


/*
 * Parses a label from the given buffer.
 *
 * Returns a negative value on error, otherwise the amount of consumed
 * characters from the given buffer.
 *
 * Error values:
 *  -3	Label too long
 */

static int
parse_label(const char *buf, struct jp_opcode *op, struct jp_state *s)
{
	char str[128] = { 0 };
	char *out = str;
	const char *in = buf;
	int rem = sizeof(str) - 1;

	while (*in == '_' || isalnum(*in))
	{
		if (rem-- < 1)
		{
			s->error_pos = s->off + (in - buf);
			return -3;
		}

		*out++ = *in++;
	}

	if (!strcmp(str, "true") || !strcmp(str, "false"))
	{
		op->num = (str[0] == 't');
		op->type = T_BOOL;
	}
	else
	{
		op->str = strdup(str);
	}

	return (in - buf);
}


/*
 * Parses a number literal from the given buffer.
 *
 * Returns a negative value on error, otherwise the amount of consumed
 * characters from the given buffer.
 *
 * Error values:
 *  -2	Invalid number character
 */

static int
parse_number(const char *buf, struct jp_opcode *op, struct jp_state *s)
{
	char *e;
	int n = strtol(buf, &e, 10);

	if (e == buf)
	{
		s->error_pos = s->off;
		return -2;
	}

	op->num = n;

	return (e - buf);
}

static const struct token tokens[] = {
	{ 0,			" ",     1 },
	{ 0,			"\t",    1 },
	{ 0,			"\n",    1 },
	{ T_LE,			"<=",    2 },
	{ T_GE,			">=",    2 },
	{ T_NE,			"!=",    2 },
	{ T_AND,		"&&",    2 },
	{ T_OR,			"||",    2 },
	{ T_DOT,		".",     1 },
	{ T_BROPEN,		"[",     1 },
	{ T_BRCLOSE,	"]",     1 },
	{ T_POPEN,		"(",     1 },
	{ T_PCLOSE,		")",     1 },
	{ T_UNION,		",",     1 },
	{ T_ROOT,		"$",     1 },
	{ T_THIS,		"@",     1 },
	{ T_LT,			"<",     1 },
	{ T_GT,			">",     1 },
	{ T_EQ,			"=",     1 },
	{ T_MATCH,		"~",     1 },
	{ T_NOT,		"!",     1 },
	{ T_WILDCARD,	"*",     1 },
	{ T_REGEXP,		"/",	 1, parse_regexp },
	{ T_STRING,		"'",	 1, parse_string },
	{ T_STRING,		"\"",	 1, parse_string },
	{ T_LABEL,		"_",     1, parse_label  },
	{ T_LABEL,		"az",    0, parse_label  },
	{ T_LABEL,		"AZ",    0, parse_label  },
	{ T_NUMBER,		"-",     1, parse_number },
	{ T_NUMBER,		"09",    0, parse_number },
};

const char *tokennames[25] = {
	[0]				= "End of file",
	[T_AND]			= "'&&'",
	[T_OR]			= "'||'",
	[T_UNION]		= "','",
	[T_EQ]			= "'='",
	[T_NE]			= "'!='",
	[T_GT]			= "'>'",
	[T_GE]			= "'>='",
	[T_LT]			= "'<'",
	[T_LE]			= "'<='",
	[T_MATCH]       = "'~'",
	[T_NOT]			= "'!'",
	[T_LABEL]		= "Label",
	[T_ROOT]		= "'$'",
	[T_THIS]		= "'@'",
	[T_DOT]			= "'.'",
	[T_WILDCARD]	= "'*'",
	[T_REGEXP]      = "/.../",
	[T_BROPEN]		= "'['",
	[T_BRCLOSE]		= "']'",
	[T_BOOL]		= "Bool",
	[T_NUMBER]		= "Number",
	[T_STRING]		= "String",
	[T_POPEN]		= "'('",
	[T_PCLOSE]		= "')'",
};


static int
match_token(const char *ptr, struct jp_opcode *op, struct jp_state *s)
{
	int i;
	const struct token *tok;

	for (i = 0, tok = &tokens[0];
	     i < sizeof(tokens) / sizeof(tokens[0]);
		 i++, tok = &tokens[i])
	{
		if ((tok->plen > 0 && !strncmp(ptr, tok->pat, tok->plen)) ||
		    (tok->plen == 0 && *ptr >= tok->pat[0] && *ptr <= tok->pat[1]))
		{
			op->type = tok->type;

			if (tok->parse)
				return tok->parse(ptr, op, s);

			return tok->plen;
		}
	}

	s->error_pos = s->off;
	return -4;
}

struct jp_opcode *
jp_get_token(struct jp_state *s, const char *input, int *mlen)
{
	struct jp_opcode op = { 0 };
	struct jp_opcode *newop;

	*mlen = match_token(input, &op, s);

	if (*mlen < 0)
	{
		s->error_code = *mlen;
		return NULL;
	}
	else if (op.type == 0)
	{
		return NULL;
	}

	newop = jp_alloc_op(s, op.type, op.num, op.str, NULL);
	/* match_token might alloced str using strdup() */
	if (op.str)
		free(op.str);

	if (!newop)
	{
		s->error_code = -12;
		*mlen = -1;
		return NULL;
	}

	return newop;
}
