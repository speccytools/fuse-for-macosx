#ifndef GDBSERVER_UTILS_H
#define GDBSERVER_UTILS_H

#include <stdint.h>

static const char hexchars[] = "0123456789abcdef";

int hex(char ch);
char *mem2hex(const uint8_t *mem, char *buf, int count);
uint8_t *hex2mem(const char *buf, uint8_t *mem, int count);
int unescape(char *msg, int len);

#endif /* GDBSERVER_UTILS_H */
