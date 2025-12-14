
#include "dns_resolver.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#define DNS_MAX_NAME 96

static bool get_u16(const uint8_t *buf, uint16_t len, uint16_t off, uint16_t *v)
{
    if (off + 2 > len) return false;
    *v = (uint16_t)((buf[off] << 8) | buf[off + 1]);
    return true;
}

static bool get_u32(const uint8_t *buf, uint16_t len, uint16_t off, uint32_t *v)
{
    if (off + 4 > len) return false;
    *v = ((uint32_t)buf[off] << 24) |
         ((uint32_t)buf[off + 1] << 16) |
         ((uint32_t)buf[off + 2] << 8) |
         ((uint32_t)buf[off + 3]);
    return true;
}

static int dns_read_name(const uint8_t *buf, uint16_t len, uint16_t start_off, char *out, uint16_t out_sz, uint16_t *bytes_consumed)
{
    uint16_t off = start_off;
    uint16_t out_pos = 0;
    bool jumped = false;
    uint16_t guard = 0;
    *bytes_consumed = 0;

    while (true)
    {
        if (++guard > 512) return -1;
        if (off >= len) return -1;

        uint8_t lab_len = buf[off];

        if ((lab_len & 0xC0) == 0xC0)
        {
            if (off + 1 >= len) return -1;
            uint16_t ptr = (uint16_t)(((lab_len & 0x3F) << 8) | buf[off + 1]);
            if (ptr >= len) return -1;
            if (!jumped) *bytes_consumed = (uint16_t)((off + 2) - start_off);
            off = ptr;
            jumped = true;
            continue;
        }
        else if (lab_len == 0)
        {
            if (!jumped) *bytes_consumed = (uint16_t)((off + 1) - start_off);
            if (out_pos == 0)
            {
                if (out_sz < 2) return -1;
                out[out_pos++] = '.';
            }
            if (out_pos >= out_sz) return -1;
            out[out_pos] = '\0';
            return (int)out_pos;
        }
        else
        {
            if ((uint16_t)(off + 1 + lab_len) > len) return -1;
            if (out_pos && out_pos < out_sz) out[out_pos++] = '.';
            if ((uint16_t)(out_pos + lab_len) >= out_sz) return -1;
            memcpy(&out[out_pos], &buf[(uint16_t)(off + 1)], lab_len);
            out_pos = (uint16_t)(out_pos + lab_len);
            off = (uint16_t)(off + 1 + lab_len);
        }
    }
}

static bool dns_skip_name(const uint8_t *buf, uint16_t len, uint16_t off, uint16_t *next_off)
{
    uint16_t pos = off;
    uint16_t guard = 0;

    while (true)
    {
        if (pos >= len) return false;
        if (++guard > 512) return false;

        uint8_t lab = buf[pos];

        if ((lab & 0xC0) == 0xC0)
        {
            if ((uint16_t)(pos + 1) >= len) return false;
            *next_off = (uint16_t)(pos + 2);     /* pointer terminates the name in-stream */
            return true;
        }
        else if (lab == 0)
        {
            *next_off = (uint16_t)(pos + 1);
            return true;
        }
        else
        {
            if ((uint16_t)(pos + 1 + lab) > len) return false;
            pos = (uint16_t)(pos + 1 + lab);
        }
    }
}

void dns_answers_process_udp(const uint8_t *buf, uint16_t len)
{
    if (!buf || len < 12) return;

    uint16_t flags, qdcount, ancount;
    if (!get_u16(buf, len, 2, &flags)) return;
    if (!get_u16(buf, len, 4, &qdcount)) return;
    if (!get_u16(buf, len, 6, &ancount)) return;

    if ((flags & 0x8000) == 0) return;
    if ((flags & 0x000F) != 0) return;

    uint16_t off = 12;

    static char qname[DNS_MAX_NAME + 1];
    bool have_qname = false;

    if (qdcount > 0)
    {
        uint16_t consumed = 0;
        int rn = dns_read_name(buf, len, off, qname, (uint16_t)sizeof(qname), &consumed);
        if (rn < 0) return;
        have_qname = true;

        off = (uint16_t)(off + consumed);
        if ((uint16_t)(off + 4) > len) return;   /* QTYPE + QCLASS */
        off = (uint16_t)(off + 4);

        for (uint16_t i = 1; i < qdcount; i++)
        {
            if (!dns_skip_name(buf, len, off, &off)) return;
            if ((uint16_t)(off + 4) > len) return;
            off = (uint16_t)(off + 4);
        }
    }
    else
    {
        /* no questions: nothing to alias; just continue */
    }

    for (uint16_t i = 0; i < ancount; i++)
    {
        static char owner[DNS_MAX_NAME + 1];
        uint16_t consumed = 0;

        int rn = dns_read_name(buf, len, off, owner, (uint16_t)sizeof(owner), &consumed);
        if (rn < 0) return;

        uint16_t name_end = (uint16_t)(off + consumed);
        if ((uint16_t)(name_end + 10) > len) return;

        uint16_t type, class_in, rdlen;
        uint32_t ttl;

        if (!get_u16(buf, len, (uint16_t)(name_end + 0), &type)) return;
        if (!get_u16(buf, len, (uint16_t)(name_end + 2), &class_in)) return;
        if (!get_u32(buf, len, (uint16_t)(name_end + 4), &ttl)) return;
        if (!get_u16(buf, len, (uint16_t)(name_end + 8), &rdlen)) return;

        uint16_t rdata_off = (uint16_t)(name_end + 10);
        if ((uint16_t)(rdata_off + rdlen) > len) return;

        if (class_in == 1 && type == 1 && rdlen == 4)
        {
            uint32_t ipv4;
            if (!get_u32(buf, len, rdata_off, &ipv4)) return;

            const char *host = have_qname ? qname : owner;
            dns_response_hostname_identified(host, ipv4);
        }

        off = (uint16_t)(rdata_off + rdlen);
    }
}