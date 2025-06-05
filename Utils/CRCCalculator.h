/*
 * crc.h
 *
 * Algorithm based on universal_crc by Danjel McGougan
 *
 * CRC parameters used:
 *   bits:       16
 *   poly:       0x8005
 *   init:       0xffff
 *   xor:        0x0000
 *   reverse:    false
 *   non-direct: false
 *
 * CRC of the string "123456789" is 0xaee7
 *
 * adapted for Ph2_ACF by G. Auzinger based on code from R. Momsen:
 * https://github.com/mommsen/evb/blob/master/include/evb/CRCCalculator.h
 */

#ifndef _CRCCalculator_h_
#define _CRCCalculator_h_

#include <cassert>
#include <stddef.h>
#include <stdint.h>
// #include "crc32c.cc"

extern const uint16_t crcTable[1024];

class CRCCalculator
{
  public:
    CRCCalculator();

    /**
     * Return the CRC of the buffer
     */
    uint16_t compute(const uint8_t* buffer, size_t bufSize) const;

    /**
     * Compute the CRC of the buffer updating the passed CRC value
     */
    void compute(uint16_t& crc, const uint8_t* buffer, size_t bufSize) const;

    /**
     * Compute CRC32-C
     */
    // uint32_t crc32c (uint32_t crc, const unsigned char* buf, size_t len) const;

  private:
    static inline void computeCRC_8bit(uint16_t& crc, const uint8_t data) { crc = (crc << 8) ^ crcTable[((crc >> 8) & 0xff) ^ data]; }

    static inline void computeCRC_32bit(uint16_t& crc, uint32_t data)
    {
        crc ^= data >> 16;
        crc = crcTable[data & 0xff] ^ crcTable[((data >> 8) & 0xff) + 0x100] ^ crcTable[(crc & 0xff) + 0x200] ^ crcTable[((crc >> 8) & 0xff) + 0x300];
    }

    const bool havePCLMULQDQ_;
    const bool haveSSE42_;
};

// extern "C"
//{
// uint16_t crc16_T10DIF_128x_extended
//(
// uint16_t init_crc,            // initial CRC value, 16 bits
// const unsigned char* buffer,  // buffer pointer to calculate CRC on
// size_t bufSize                // buffer length in bytes (64-bit data)
//);
//}

// extern uint32_t crc32c_sw (uint32_t crc, const unsigned char* buf, size_t len);
// extern uint32_t crc32c_hw (uint32_t crc, const unsigned char* buf, size_t len);

#endif // _CRCCalculator_h_
