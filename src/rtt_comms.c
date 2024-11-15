#include "rtt_comms.h"
#include "SEGGER_RTT.h"

const char TERMINATOR = '\0';

void rtt_init()
{
    SEGGER_RTT_Init();
    SEGGER_RTT_ConfigUpBuffer(0, "Framebuffer", NULL, 512, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
}

/*
 * Simple channel to send a framebuffer over RTT.
 * The encoding scheme is:
 *  - A frame is denoted by a \0 byte
 *  - The dimensions of the frame are sent
 *  - The framebuffer is sent as a sequence of bytes
 */
int rtt_send_framebuffer(uint8_t *buffer, size_t length, size_t width, size_t height)
{
    uint8_t byte;

    // We use \0 as a marker of the start of a new frame (1 bytes)
    SEGGER_RTT_Write(0, &TERMINATOR, 1);
    // We send the length of the frame (4 bytes)
    SEGGER_RTT_Write(0, &length, sizeof(length));
    SEGGER_RTT_Write(0, &width, sizeof(width));
    SEGGER_RTT_Write(0, &height, sizeof(height));

    // We send the encoded framebuffer (length bytes)
    for (int i = 0; i < length; i++)
    {
        // Simple encoding scheme:
        //  - We send the two nibbles of the byte separately
        //  - The 4 bits will reside in the high-side of the byte
        //  - The lower 4 bits will be set to 0xF, as they are used by the signaling
        uint8_t high_byte = buffer[i] | 0x0F;
        SEGGER_RTT_Write(0, &high_byte, 1);
        uint8_t low_byte = (buffer[i] << 4) | 0x0F;
        SEGGER_RTT_Write(0, &low_byte, 1);
    }
}