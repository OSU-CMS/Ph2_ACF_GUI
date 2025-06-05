/*!
  \file                  RD53ACommands.h
  \brief                 RD53ACommands description
  \author                Mauro DINARDO and Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/22
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#ifndef RD53ACOMMANDS_H
#define RD53ACOMMANDS_H

#include "Utils/BitMaster/bit_packing.h"

#include <array>
#include <cstdint>
#include <vector>

namespace RD53ACmd
{
// Map 5-bit to 8-bit fields
constexpr uint8_t map5to8bit[] = {
    0x6A, // 00: 0b01101010,
    0x6C, // 01: 0b01101100,
    0x71, // 02: 0b01110001,
    0x72, // 03: 0b01110010,
    0x74, // 04: 0b01110100,
    0x8B, // 05: 0b10001011,
    0x8D, // 06: 0b10001101,
    0x8E, // 07: 0b10001110,
    0x93, // 08: 0b10010011,
    0x95, // 09: 0b10010101,
    0x96, // 10: 0b10010110,
    0x99, // 11: 0b10011001,
    0x9A, // 12: 0b10011010,
    0x9C, // 13: 0b10011100,
    0xA3, // 14: 0b10100011,
    0xA5, // 15: 0b10100101,
    0xA6, // 16: 0b10100110,
    0xA9, // 17: 0b10101001,
    0xAA, // 18: 0b10101010,
    0xAC, // 19: 0b10101100,
    0xB1, // 20: 0b10110001,
    0xB2, // 21: 0b10110010,
    0xB4, // 22: 0b10110100,
    0xC3, // 23: 0b11000011,
    0xC5, // 24: 0b11000101,
    0xC6, // 25: 0b11000110,
    0xC9, // 26: 0b11001001,
    0xCA, // 27: 0b11001010,
    0xCC, // 28: 0b11001100,
    0xD1, // 29: 0b11010001,
    0xD2, // 30: 0b11010010,
    0xD4  // 31: 0b11010100
};

// ############
// # Commands #
// ############
namespace RD53ACmdEncoder
{
const uint16_t CAL        = 0x6363; // Calibration word
const uint16_t READ       = 0x6565; // Read command word
const uint16_t WRITE      = 0x6666; // Write command word
const uint16_t GLOB_PULSE = 0x5C5C; // Global pulse word
const uint16_t RESET_ECR  = 0x5A5A; // Event Counter Reset word
const uint16_t RESET_BCR  = 0x5959; // Bunch Counter Reset word
const uint16_t NOOP       = 0x6969; // No operation word
const uint16_t SYNC       = 0x817E; // Synchronization word
} // namespace RD53ACmdEncoder

template <class T>
auto serializeFields(const T& cmd)
{
    return std::array<uint8_t, 0>();
}

struct ECR
{
    static constexpr uint16_t cmdCode() { return RD53ACmdEncoder::RESET_ECR; }
    static constexpr uint16_t nFields() { return 0; }
};

struct BCR
{
    static constexpr uint16_t cmdCode() { return RD53ACmdEncoder::RESET_BCR; }
    static constexpr uint16_t nFields() { return 0; }
};

struct NoOp
{
    static constexpr uint16_t cmdCode() { return RD53ACmdEncoder::NOOP; }
    static constexpr uint16_t nFields() { return 0; }
};

struct Sync
{
    static constexpr uint16_t cmdCode() { return RD53ACmdEncoder::SYNC; }
    static constexpr uint16_t nFields() { return 0; }
};

struct GlobalPulse
{
    static constexpr uint16_t cmdCode() { return RD53ACmdEncoder::GLOB_PULSE; }
    static constexpr uint16_t nFields() { return 2; }

    size_t chip_id = 8;
    size_t data;
};

std::array<uint8_t, GlobalPulse::nFields()> serializeFields(const GlobalPulse& cmd);

struct Cal
{
    static constexpr uint16_t cmdCode() { return RD53ACmdEncoder::CAL; }
    static constexpr uint16_t nFields() { return 4; }

    size_t chip_id = 8;
    bool   cal_edge_mode;
    size_t cal_edge_delay;
    size_t cal_edge_width;
    bool   cal_aux_mode;
    size_t cal_aux_delay;
};

std::array<uint8_t, Cal::nFields()> serializeFields(const Cal& cmd);

struct WrReg
{
    static constexpr uint16_t cmdCode() { return RD53ACmdEncoder::WRITE; }
    static constexpr uint16_t nFields() { return 6; }

    size_t chip_id = 8;
    size_t address;
    size_t value;
};

std::array<uint8_t, WrReg::nFields()> serializeFields(const WrReg& cmd);

struct WrRegLong
{
    static constexpr uint16_t cmdCode() { return RD53ACmdEncoder::WRITE; }
    static constexpr uint16_t nFields() { return 22; }

    size_t                  chip_id = 8;
    size_t                  address;
    std::array<uint16_t, 6> values;
};

std::array<uint8_t, WrRegLong::nFields()> serializeFields(const WrRegLong& cmd);

struct RdReg
{
    static constexpr uint16_t cmdCode() { return RD53ACmdEncoder::READ; }
    static constexpr uint16_t nFields() { return 4; }

    size_t chip_id = 8;
    size_t address;
};

std::array<uint8_t, RdReg::nFields()> serializeFields(const RdReg& cmd);

template <class CmdType>
size_t getN16bitWords()
{
    return 1 + CmdType::nFields() / 2;
}

template <int... Sizes, class... Args>
uint8_t packAndEncode(Args&&... args)
{
    return map5to8bit[bits::pack<Sizes...>(std::forward<Args>(args)...)];
}

template <class CmdType>
void serialize(const CmdType& cmd, std::vector<uint16_t>& cmdStream)
{
    auto fields = serializeFields(cmd);
    cmdStream.reserve(cmdStream.size() + getN16bitWords<CmdType>());

    // Insert command code
    cmdStream.push_back(CmdType::cmdCode());

    // Insert: chip id, address and data
    for(auto i = 1u; i < fields.size(); i += 2) cmdStream.push_back(bits::pack<8, 8>(fields[i - 1], fields[i]));
}

template <class cmdType>
std::vector<uint16_t> serialize(const cmdType& cmd)
{
    std::vector<uint16_t> cmdStream;
    serialize(cmd, cmdStream);
    return cmdStream;
}

} // namespace RD53ACmd

#endif
