#pragma once

#include <stddef.h>
#include <stdint.h>

uint32_t crc32c(uint32_t crc, const unsigned char* buf, size_t len)
{
    int k;

    crc = ~crc;
    while (len--) {
        crc ^= *buf++;
        for (k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1;
    }
    return ~crc;
}
