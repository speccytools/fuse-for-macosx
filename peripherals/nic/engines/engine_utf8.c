#include "engine_utf8.h"

#include <stdbool.h>
#include <stdint.h>

/* Latin-1 0xC0-0xFF: NFKD to ASCII + manual fixes (Æ æ ß Ð ð Þ þ). */
static const uint8_t latin1_fold[64] = {
    65,  65,  65,  65,  65,  65,  65,  67,  69,  69,  69,  69,  73,  73,  73,  73,
    68,  78,  79,  79,  79,  79,  79,  32,  32,  85,  85,  85,  85,  89,  84,  115,
    97,  97,  97,  97,  97,  97,  97,  99,  101, 101, 101, 101, 105, 105, 105, 105,
    100, 110, 111, 111, 111, 111, 111, 32,  32,  117, 117, 117, 117, 121, 116, 121,
};

static const uint8_t latin_ext_a_fold[128] = {
    65,  97,  65,  97,  65,  97,  67,  99,  67,  99,  67,  99,  67,  99,  68,  100, 32,  32,  69,  101, 69,  101, 69,  101, 69,  101, 69,  101, 71,  103, 71,  103,
    71,  103, 71,  103, 72,  104, 32,  32,  73,  105, 73,  105, 73,  105, 73,  105, 73,  32,  73,  105, 74,  106, 75,  107, 32,  76,  108, 76,  108, 76,  108, 76,
    108, 32,  32,  78,  110, 78,  110, 78,  110, 110, 32,  32,  79,  111, 79,  111, 79,  111, 32,  32,  82,  114, 82,  114, 82,  114, 83,  115, 83,  115, 83,  115,
    83,  115, 84,  116, 84,  116, 32,  32,  85,  117, 85,  117, 85,  117, 85,  117, 85,  117, 85,  117, 87,  119, 89,  121, 89,  90,  122, 90,  122, 90,  122, 115,
};

static const uint8_t latin_ext_b_fold[208] = {
    32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,
    79,  111, 32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  85,  117, 32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,
    32,  32,  32,  68,  68,  100, 76,  76,  108, 78,  78,  110, 65,  97,  73,  105, 79,  111, 85,  117, 85,  117, 85,  117, 85,  117, 85,  117, 32,  65,  97,  65,
    97,  32,  32,  32,  32,  71,  103, 75,  107, 79,  111, 79,  111, 32,  32,  106, 68,  68,  100, 71,  103, 32,  32,  78,  110, 65,  97,  32,  32,  32,  32,  65,
    97,  65,  97,  69,  101, 69,  101, 73,  105, 73,  105, 79,  111, 79,  111, 82,  114, 82,  114, 85,  117, 85,  117, 83,  115, 84,  116, 32,  32,  72,  104, 32,
    32,  32,  32,  32,  32,  65,  97,  69,  101, 79,  111, 79,  111, 79,  111, 79,  111, 89,  121, 32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,
    32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,
};

static size_t utf8_decode(const char *s, uint32_t *cp_out)
{
    const unsigned char *p = (const unsigned char *)s;
    unsigned char c = p[0];
    if (c == 0)
    {
        *cp_out = 0;
        return 0;
    }
    if (c < 0x80u)
    {
        *cp_out = c;
        return 1;
    }
    if (c < 0xC2u)
        return 0;
    if (c <= 0xDFu)
    {
        unsigned char c1 = p[1];
        if ((c1 & 0xC0u) != 0x80u)
            return 0;
        uint32_t cp = ((uint32_t)(c & 0x1Fu) << 6) | (uint32_t)(c1 & 0x3Fu);
        if (cp < 0x80u)
            return 0;
        *cp_out = cp;
        return 2;
    }
    if (c <= 0xEFu)
    {
        unsigned char c1 = p[1];
        unsigned char c2 = p[2];
        if (((c1 | c2) & 0xC0u) != 0x80u)
            return 0;
        uint32_t cp = ((uint32_t)(c & 0x0Fu) << 12) | ((uint32_t)(c1 & 0x3Fu) << 6) | (uint32_t)(c2 & 0x3Fu);
        if (cp < 0x800u || (cp >= 0xD800u && cp <= 0xDFFFu))
            return 0;
        *cp_out = cp;
        return 3;
    }
    if (c <= 0xF4u)
    {
        unsigned char c1 = p[1];
        unsigned char c2 = p[2];
        unsigned char c3 = p[3];
        if (((c1 | c2 | c3) & 0xC0u) != 0x80u)
            return 0;
        uint32_t cp = ((uint32_t)(c & 0x07u) << 18) | ((uint32_t)(c1 & 0x3Fu) << 12) | ((uint32_t)(c2 & 0x3Fu) << 6) |
                      (uint32_t)(c3 & 0x3Fu);
        if (cp < 0x10000u || cp > 0x10FFFFu)
            return 0;
        *cp_out = cp;
        return 4;
    }
    return 0;
}

static bool try_fold_punct(uint32_t cp, uint8_t *out)
{
    switch (cp)
    {
        case 0x00A0u:
        case 0x1680u:
        case 0x2000u:
        case 0x2001u:
        case 0x2002u:
        case 0x2003u:
        case 0x2004u:
        case 0x2005u:
        case 0x2006u:
        case 0x2007u:
        case 0x2008u:
        case 0x2009u:
        case 0x200Au:
        case 0x200Bu:
        case 0x200Cu:
        case 0x200Du:
        case 0x202Fu:
        case 0x205Fu:
        case 0x3000u:
        case 0xFEFFu:
            *out = (uint8_t)' ';
            return true;
        case 0x00ADu:
        case 0x2010u:
        case 0x2011u:
        case 0x2012u:
        case 0x2013u:
        case 0x2014u:
        case 0x2015u:
            *out = (uint8_t)'-';
            return true;
        case 0x2018u:
        case 0x2019u:
        case 0x201Au:
        case 0x201Bu:
        case 0x2032u:
        case 0x2035u:
            *out = (uint8_t)'\'';
            return true;
        case 0x201Cu:
        case 0x201Du:
        case 0x201Eu:
        case 0x201Fu:
        case 0x2033u:
        case 0x2036u:
            *out = (uint8_t)'"';
            return true;
        case 0x2026u:
            *out = (uint8_t)'.';
            return true;
        case 0x00B4u:
        case 0x02B9u:
        case 0x02BCu:
        case 0x02C8u:
            *out = (uint8_t)'\'';
            return true;
        case 0x00ABu:
        case 0x00BBu:
            *out = (uint8_t)'"';
            return true;
        default:
            return false;
    }
}

static uint8_t fold_codepoint(uint32_t cp)
{
    if (cp < 0x80u)
    {
        if (cp >= 32u && cp <= 126u)
            return (uint8_t)cp;
        if (cp == 9u || cp == 10u || cp == 13u)
            return (uint8_t)cp;
        return (uint8_t)' ';
    }

    uint8_t punct_out;
    if (try_fold_punct(cp, &punct_out))
        return punct_out;

    if (cp >= 0xC0u && cp <= 0xFFu)
        return latin1_fold[cp - 0xC0u];
    if (cp >= 0x100u && cp <= 0x17Fu)
        return latin_ext_a_fold[cp - 0x100u];
    if (cp >= 0x180u && cp <= 0x24Fu)
        return latin_ext_b_fold[cp - 0x180u];

    return (uint8_t)' ';
}

void engine_utf8_fold(char *buf, size_t cap)
{
    if (!buf || cap < 2u)
    {
        if (buf && cap)
            buf[0] = '\0';
        return;
    }

    size_t r = 0;
    size_t w = 0;
    while (buf[r] != '\0' && w + 1u < cap)
    {
        uint32_t cp = 0;
        const size_t n = utf8_decode(buf + r, &cp);
        if (n == 0u)
        {
            buf[w++] = (uint8_t)' ';
            r++;
            continue;
        }
        r += n;
        buf[w++] = fold_codepoint(cp);
    }
    buf[w] = '\0';
}
