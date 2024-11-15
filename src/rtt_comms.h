#pragma once

#include <stdint.h>
#include <stddef.h>

/*
 */
void rtt_init();

int rtt_send_framebuffer(uint8_t *buffer, size_t length, size_t width, size_t height);