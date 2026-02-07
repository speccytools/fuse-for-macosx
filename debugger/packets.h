#ifndef PACKETS_H
#define PACKETS_H

#include <stdint.h>
#include <stddef.h>

#define PACKET_BUF_SIZE 0x4000

static const char INTERRUPT_CHAR = '\x03';

// Return values for packets_feed_byte()
typedef enum
{
    PACKETS_FEED_NOT_CONSUMED = 0,      // Byte not consumed by deframer
    PACKETS_FEED_CONSUMED = 1,          // Byte consumed by deframer (processing)
    PACKETS_FEED_COMPLETE = 2,          // Packet complete (ACK sent)
    PACKETS_FEED_INTERRUPT = 3          // Interrupt character received in idle state
} packets_feed_result_t;

// Initialize packets subsystem
void packets_init(void);

// Reset deframer state
void packets_reset(void);

// Feed a byte to the RSP deframer
packets_feed_result_t packets_feed_byte(uint8_t byte);

// Get pointer to current packet buffer (for process_packet() to use)
const uint8_t* packets_get_packet(void);

// Get length of current packet
size_t packets_get_packet_len(void);

// Read raw bytes from socket into incoming buffer
// Returns: number of bytes read, or -1 on error
int packets_read_socket(int sockfd);

// Get pointer to incoming raw buffer
const uint8_t* packets_get_incoming_raw(void);

// Get length of incoming raw buffer
size_t packets_get_incoming_raw_len(void);

// Clear incoming raw buffer
void packets_clear_incoming_raw(void);

// Send packet with CRC calculation
void packet_send_message(const uint8_t *data, size_t len);

// Send ACK
void acknowledge_packet(void);

// Send NAK
void nak_packet(void);

#endif /* PACKETS_H */
