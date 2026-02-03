/* Author: Richard James Howe
 * License: The Unlicense
 * Email: howe.r.j.89@gmail.com
 * Repo: https://github.com/howerj/localely
 *
 * As you may know, "locale.h" should be avoided, for a good
 * reason why, see:
 *
 * <https://github.com/mpv-player/mpv/commit/1e70e82baa9193f6f027338b0fab0f5078971fbe>.
 *
 * The functions in <ctype.h> are locale dependent. This means you can
 * get unexpected behavior with those functions (especially if some random
 * library goes and uses "setlocale" within your program).
 *
 * If you want a set of portable isX() functions in C that does not
 * depend on the locale, look no further, or look further if you want
 * super highly optimized code. 
 *
 * We could add a few other functions for printing and scanning numbers.
 *
 * Ideally we would also make a set of printf/scanf functions that were
 * also locale independent as well, but those functions are are quite
 * complex. 
 *
 * This library deals with ASCII/Bytes, not UTF-8 or anything else. 
 *
 * This is a header only library that should compile under C and C++.
 *
 * The following #define directives might be useful to you:
 *
 * #define LOCALELY_API
 * #define LOCALELY_IMPLEMENTATION
 * #define LOCALELY_UNIT_TESTS
 * #define LOCALELY_DEFINE_MAIN
 * #define NDEBUG
 *
 * You can use these to make a version of this library that define
 * the isX functions that are; just declarations, instances with
 * external linkage, instances that are static (and static inline).
 * You can also instantiate a set of unit tests and even an instance
 * of "main()" that runs the unit tests.
 *
 * By default:
 *
 * - The library behaves like an ordinary header with declarations
 *   of the functions that are to be exported.
 * - Unit tests are off.
 * - "main()" is not defined.
 *
 * To use this library you will need to do one of the following depending
 * on your use case:
 *
 * 1) Include the header file in your C file and create another C
 * file to link with that includes this header and defines
 * "LOCALELY_IMPLEMENTATION" before the header is included.
 * 2) Include the header file in your C file, and before that
 * inclusion define "LOCALELY_IMPLEMENTATION" and define LOCALELY_API 
 * as "static inline".
 * 3) Define "LOCALELY_IMPLEMENTATION", "LOCALELY_UNIT_TESTS" and
 * "LOCALELY_DEFINE_MAIN" before the header file inclusion in another C 
 * file which includes this header to make a program that runs the 
 * unit tests and exits.
 *
 * You many need to set LOCALELY_EXTERN as well when setting LOCALELY_API
 * to avoid a function being declared "static" and "extern".
 *
 * The header only depends on <assert.h> unless unit tests are needed. The
 * unit tests do what I complain about earlier, being some random library
 * that uses "setlocale", you should not need to call them in your program
 * however as they are just an aid for development, so I will allow my
 * own hypocrisy. */
#ifndef LOCALELY_H
#define LOCALELY_H
#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>

#ifndef LOCALELY_API
#define LOCALELY_API
#endif

#ifndef LOCALELY_EXTERN
#define LOCALELY_EXTERN extern
#endif

#define LOCALELY_VERSION (0x010200ul)

LOCALELY_EXTERN int C_isascii(int ch);
LOCALELY_EXTERN int C_isspace(int ch);
LOCALELY_EXTERN int C_iscntrl(int ch);
LOCALELY_EXTERN int C_isprint(int ch);
LOCALELY_EXTERN int C_isblank(int ch);
LOCALELY_EXTERN int C_isgraph(int ch);
LOCALELY_EXTERN int C_isupper(int ch);
LOCALELY_EXTERN int C_islower(int ch);
LOCALELY_EXTERN int C_isalpha(int ch);
LOCALELY_EXTERN int C_isdigit(int ch);
LOCALELY_EXTERN int C_isalnum(int ch);
LOCALELY_EXTERN int C_ispunct(int ch);
LOCALELY_EXTERN int C_isxdigit(int ch);

LOCALELY_EXTERN int C_toupper(int ch);
LOCALELY_EXTERN int C_tolower(int ch);


#ifdef LOCALELY_IMPLEMENTATION

#ifdef LOCALELY_STRICT_INPUT
static inline int C_char_range(const int ch) { assert(ch >= -1 && ch <= 255); return ch; }
#else
static inline int C_char_range(const int ch) { return ch & 0xFF; /* EOF converted to 0xFF, which does not matter for conversion functions */ }
#endif

static inline int C_char_return(const int r) { assert(r == 0 || r == 1); return r; }
LOCALELY_API int C_isascii(int ch) { ch = C_char_range(ch); return C_char_return(ch < 128 && ch >= 0); }
LOCALELY_API int C_isspace(int ch) { ch = C_char_range(ch); return C_char_return((ch >= 9 && ch <= 13) || ch == 32); }
LOCALELY_API int C_iscntrl(int ch) { ch = C_char_range(ch); return C_char_return((ch < 32 || ch == 127) && C_isascii(ch)); }
LOCALELY_API int C_isprint(int ch) { ch = C_char_range(ch); return C_char_return(!C_iscntrl(ch) && C_isascii(ch)); }
LOCALELY_API int C_isblank(int ch) { ch = C_char_range(ch); return C_char_return(ch == 32 || ch == 9); }
LOCALELY_API int C_isgraph(int ch) { ch = C_char_range(ch); return C_char_return(ch > 32 && ch < 127); }
LOCALELY_API int C_isupper(int ch) { ch = C_char_range(ch); return C_char_return(ch >= 65 && ch <= 90); }
LOCALELY_API int C_islower(int ch) { ch = C_char_range(ch); return C_char_return(ch >= 97 && ch <= 122); }
LOCALELY_API int C_isalpha(int ch) { ch = C_char_range(ch); return C_char_return(C_islower(ch) || C_isupper(ch)); }
LOCALELY_API int C_isdigit(int ch) { ch = C_char_range(ch); return C_char_return(ch >= 48 && ch <= 57); }
LOCALELY_API int C_isalnum(int ch) { ch = C_char_range(ch); return C_char_return(C_isalpha(ch) || C_isdigit(ch)); }
LOCALELY_API int C_ispunct(int ch) { ch = C_char_range(ch); return C_char_return((ch >= 33 && ch <= 47) || (ch >= 58 && ch <= 64) || (ch >= 91 && ch <= 96) || (ch >= 123 && ch <= 126)); }
LOCALELY_API int C_isxdigit(int ch) { ch = C_char_range(ch); return C_char_return((ch >= 65 && ch <= 70) || (ch >= 97 && ch <= 102) || C_isdigit(ch)); }

LOCALELY_API int C_toupper(int ch) { return C_islower(ch) ? ch ^ 0x20 : ch; }
LOCALELY_API int C_tolower(int ch) { return C_isupper(ch) ? ch ^ 0x20 : ch; }

#endif

#ifdef LOCALELY_UNIT_TESTS
#include <locale.h> /* needed to set the locale */
#include <ctype.h> /* compare our functions against the standard libs */

/* Can turn anything into a bool, things such as NULL, 0, 1, even 2,
 * or "Does this arbitrary program halt?" (which returns true, because
 * that's a string. */
#define BOOLINATOR(X) (!!(X)) 

#ifndef LOCALELY_IMPLEMENTATION

extern int localely_unit_tests(void); /* returns zero on success, negative on failure */

#else

LOCALELY_API int localely_unit_tests(void) {
	if (!setlocale(LC_ALL, "C")) /* force locale, needed, but super not ideal */
		return -1;
	for (int ch = -1; ch < 256; ch++) {
		if (C_isspace(ch) != BOOLINATOR(isspace(ch))) return -2;
		if (C_iscntrl(ch) != BOOLINATOR(iscntrl(ch))) return -3;
		if (C_isprint(ch) != BOOLINATOR(isprint(ch))) return -4;
		if (C_isblank(ch) != BOOLINATOR(isblank(ch))) return -5;
		if (C_isgraph(ch) != BOOLINATOR(isgraph(ch))) return -6;
		if (C_isupper(ch) != BOOLINATOR(isupper(ch))) return -7;
		if (C_islower(ch) != BOOLINATOR(islower(ch))) return -8;
		if (C_isalpha(ch) != BOOLINATOR(isalpha(ch))) return -9;
		if (C_isdigit(ch) != BOOLINATOR(isdigit(ch))) return -10;
		if (C_isalnum(ch) != BOOLINATOR(isalnum(ch))) return -11;
		if (C_ispunct(ch) != BOOLINATOR(ispunct(ch))) return -12;
		if (C_isxdigit(ch) != BOOLINATOR(isxdigit(ch))) return -13;
	}
	for (int ch = 0; ch < 256; ch++) {
		if (C_isupper(ch)) {
			if (C_toupper(ch) != ch) return -14;
			if (C_tolower(ch) == ch) return -15;
			if (!C_islower(C_tolower(ch))) return -16;
		} else if (C_islower(ch)) {
			if (C_tolower(ch) != ch) return -18;
			if (C_toupper(ch) == ch) return -17;
			if (!C_isupper(C_toupper(ch))) return -19;
		} else {
			if (C_tolower(ch) != ch) return -20;
			if (C_toupper(ch) != ch) return -21;

		}
	}
	return 0;
}
#endif

#undef BOOLINATOR

#ifdef LOCALELY_DEFINE_MAIN
#include <stdio.h>

int main(void) {
	int r = localely_unit_tests();
	if (printf("Localely Unit Test Results: %d %s\n", r, r ? "[FAIL]" : "[PASS]") < 0)
		return 2;
	return r == 0 ? 0 : 1;
}

#endif /* LOCALELY_UNIT_TESTS */
#endif /* LOCALELY_DEFINE_MAIN */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* LOCALELY_H */
