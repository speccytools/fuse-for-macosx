#pragma once

#include <stdint.h>

#define DNS_DEBUG (1)

// register a hostname to IP. Called by dns_resolve internally or by dns_answers_process
extern void dns_response_hostname_identified(const char *hostname, uint32_t ipv4);

// every DNS response requested by clients shall be processed
extern void dns_answers_process_udp(const uint8_t *buf, uint16_t len);

// lookup hostname by IP
const char* dns_resolve_hostname(uint32_t ipv4);

extern void dns_resolver_init();
