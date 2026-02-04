#include "config.h"

#include "packets.h"

#include "libspectrum.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

// External socket fd (set by gdbserver)
extern int gdbserver_client_socket;

// RSP deframer state machine
typedef enum
{
    rsp_idle,
    rsp_in_payload,
    rsp_hash_hi,
    rsp_hash_lo
} rsp_state_t;

typedef struct
{
    rsp_state_t state;
    uint8_t    *buf;
    uint16_t    cap;
    uint16_t    len;
    uint8_t     csum;
    int         hi;
} rsp_deframer_t;

// Static incoming buffer for raw socket data
static uint8_t incoming_raw[PACKET_BUF_SIZE];
static size_t incoming_raw_len = 0;

// Deframer state
static rsp_deframer_t deframer = {};
static uint8_t deframer_buffer[PACKET_BUF_SIZE];

// Helper: hex nibble to value
static inline int hex_nib(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Send ACK response
static void send_ack(void)
{
    if (gdbserver_client_socket >= 0)
    {
        const char ack = '+';
        send(gdbserver_client_socket, &ack, 1, 0);
    }
}

// Send NAK response
static void send_nak(void)
{
    if (gdbserver_client_socket >= 0)
    {
        const char nak = '-';
        send(gdbserver_client_socket, &nak, 1, 0);
    }
}

// Send data directly to socket
static void send_data(const uint8_t *data, size_t len)
{
    if (gdbserver_client_socket < 0) return;
    
    size_t write_index = 0;
    while (write_index < len)
    {
        ssize_t nwritten = send(gdbserver_client_socket, (const void *)(data + write_index), len - write_index, 0);
        if (nwritten < 0)
        {
#ifdef WIN32
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK || err == WSAEINTR)
            {
                // Would block or interrupted - retry
                continue;
            }
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
                // Would block or interrupted - retry
                continue;
            }
#endif
            printf("Write error: send() failed\n");
            return;
        }
        if (nwritten == 0)
        {
            // Socket closed
            return;
        }
        write_index += nwritten;
    }
}

// Initialize packets subsystem
void packets_init(void)
{
    deframer.state = rsp_idle;
    deframer.buf   = deframer_buffer;
    deframer.cap   = PACKET_BUF_SIZE;
    deframer.len   = 0;
    deframer.csum  = 0;
    deframer.hi    = -1;
    incoming_raw_len = 0;
}

// Reset deframer state
void packets_reset(void)
{
    deframer.state = rsp_idle;
    deframer.len   = 0;
    deframer.csum  = 0;
    deframer.hi    = -1;
}

// Get pointer to current packet buffer
const uint8_t* packets_get_packet(void)
{
    return deframer.buf;
}

// Get length of current packet
size_t packets_get_packet_len(void)
{
    return deframer.len;
}

// Feed a byte to the RSP deframer
packets_feed_result_t packets_feed_byte(uint8_t byte)
{
    switch (deframer.state)
    {
        case rsp_idle:
        {
            if (byte == '$')
            {
                deframer.state = rsp_in_payload;
                deframer.len   = 0;
                deframer.csum  = 0;
                return 1;
            }
            
            if (byte == INTERRUPT_CHAR)
            {
                // Interrupt character received in idle state
                packets_reset();
                return PACKETS_FEED_INTERRUPT;
            }
            
            return PACKETS_FEED_NOT_CONSUMED;
        }
        
        case rsp_in_payload:
        {
            if (byte == '#')
            {
                deframer.state = rsp_hash_hi;
            }
            else
            {
                if (deframer.len >= deframer.cap)
                {
                    // Buffer overflow - drop this frame
                    send_nak();
                    packets_reset();
                    return PACKETS_FEED_NOT_CONSUMED;
                }
                
                deframer.buf[deframer.len++] = byte;
                deframer.csum = (uint8_t)(deframer.csum + byte);
            }
            
            return PACKETS_FEED_CONSUMED;
        }
        
        case rsp_hash_hi:
        {
            int h = hex_nib(byte);
            if (h < 0) 
            { 
                send_nak();
                packets_reset(); 
                return PACKETS_FEED_NOT_CONSUMED; 
            }
            deframer.hi = h;
            deframer.state = rsp_hash_lo;
            return PACKETS_FEED_CONSUMED;
        }
        
        case rsp_hash_lo:
        {
            int lo = hex_nib(byte);
            bool ok = (deframer.hi >= 0) && (lo >= 0) &&
                      (((deframer.hi << 4) | lo) == deframer.csum);
            
            if (ok)
            {
                // Ensure buffer is null-terminated for string operations (if there's space)
                // Note: We don't overwrite the last byte if buffer is full, as payload
                // may contain null bytes and we track length separately via packets_get_packet_len()
                if (deframer.len < deframer.cap)
                {
                    deframer.buf[deframer.len] = '\0';
                }
                // If buffer is full, we can't null-terminate without corrupting data
                // Callers should use packets_get_packet_len() to get the actual length
                
                // Send ACK for valid packet
                send_ack();
                
                return PACKETS_FEED_COMPLETE;  // Packet successfully received
            }
            else
            {
                // Invalid checksum - send NAK
                send_nak();
                packets_reset();
                return PACKETS_FEED_NOT_CONSUMED;
            }
        }
    }
    return PACKETS_FEED_NOT_CONSUMED;
}

// Read raw bytes from socket into incoming buffer
// Returns: number of bytes read (>0), 0 = no data available (would block), -1 = error/EOF (disconnect)
int packets_read_socket(int sockfd)
{
    if (sockfd < 0) return -1;
    
    // Read available data
    ssize_t nread = recv(sockfd, (void*)incoming_raw, sizeof(incoming_raw), 0);
    if (nread < 0)
    {
#ifdef WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSAEINTR)
        {
            // Would block or interrupted - no data available
            return 0;
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            // Would block or interrupted - no data available
            return 0;
        }
#endif
        // Real error
        return -1;
    }
    if (nread == 0)
    {
        // EOF - peer closed connection
        return -1;
    }
    
    incoming_raw_len = (size_t)nread;
    return (int)nread;
}

// Get pointer to incoming raw buffer
const uint8_t* packets_get_incoming_raw(void)
{
    return incoming_raw;
}

// Get length of incoming raw buffer
size_t packets_get_incoming_raw_len(void)
{
    return incoming_raw_len;
}

// Clear incoming raw buffer
void packets_clear_incoming_raw(void)
{
    incoming_raw_len = 0;
}

// Send packet with CRC calculation
void packet_send_message(const uint8_t *data, size_t len)
{
    if (gdbserver_client_socket < 0) return;
    
    // Allocate buffer: $ + data + # + checksum (2 hex chars)
    size_t packet_size = 1 + len + 1 + 2;  // $ + data + # + checksum
    uint8_t *packet_buf = libspectrum_malloc(packet_size);
    if (!packet_buf) return;  // Allocation failed
    
    size_t pos = 0;
    uint8_t checksum = 0;
    
    // Build packet: $<data>#<checksum>
    // Empty packets are valid (len == 0) - they send as $#00
    packet_buf[pos++] = '$';
    
    // Calculate checksum and copy data
    for (size_t i = 0; i < len; ++i)
    {
        checksum += data[i];
        packet_buf[pos++] = data[i];
    }
    
    // Add checksum delimiter
    packet_buf[pos++] = '#';
    
    // Add checksum hex (2 bytes)
    packet_buf[pos++] = "0123456789abcdef"[(checksum >> 4) & 0xF];
    packet_buf[pos++] = "0123456789abcdef"[checksum & 0xF];
    
    // Send packet directly
    send_data(packet_buf, pos);
    
    // Free allocated buffer
    libspectrum_free(packet_buf);
}

// Send ACK
void acknowledge_packet(void)
{
    send_ack();
}

// Send NAK
void nak_packet(void)
{
    send_nak();
}
