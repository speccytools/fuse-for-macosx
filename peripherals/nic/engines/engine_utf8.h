#pragma once

#include <stddef.h>

/**
 * Replace UTF-8 text in `buf` with ASCII-oriented output: common punctuation → ASCII
 * substitutes, Latin letters → stripped/demoted via NFKD-style tables, other code points → space.
 * Preserves TAB/LF/CR and printable ASCII; other C0/C1 controls become space.
 * In-place; result is always NUL-terminated and not longer than the input string in bytes.
 */
void engine_utf8_fold(char *buf, size_t cap);
