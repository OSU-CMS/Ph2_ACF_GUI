/*!
  \file                  RD53ACommands.cc
  \brief                 RD53ACommands implementation
  \author                Mauro DINARDO and Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/22
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#include "RD53ACommands.h"

namespace RD53ACmd
{
std::array<uint8_t, GlobalPulse::nFields()> serializeFields(const GlobalPulse& cmd)
{
    std::array<uint8_t, GlobalPulse::nFields()> fields;

    fields[0] = packAndEncode<4, 1>(cmd.chip_id, 0);
    fields[1] = packAndEncode<4, 1>(cmd.data, 0);

    return fields;
}

std::array<uint8_t, Cal::nFields()> serializeFields(const Cal& cmd)
{
    std::array<uint8_t, Cal::nFields()> fields;

    fields[0] = packAndEncode<4, 1>(cmd.chip_id, cmd.cal_edge_mode);
    fields[1] = packAndEncode<3, 2>(cmd.cal_edge_delay, cmd.cal_edge_width >> 4);
    fields[2] = packAndEncode<4, 1>(cmd.cal_edge_width, cmd.cal_aux_mode);
    fields[3] = packAndEncode<5>(cmd.cal_aux_delay);

    return fields;
}

std::array<uint8_t, WrReg::nFields()> serializeFields(const WrReg& cmd)
{
    std::array<uint8_t, WrReg::nFields()> fields;

    fields[0] = packAndEncode<4, 1>(cmd.chip_id, 0);
    fields[1] = packAndEncode<5>(cmd.address >> 4);
    fields[2] = packAndEncode<4, 1>(cmd.address, cmd.value >> 15);
    fields[3] = packAndEncode<5>(cmd.value >> 10);
    fields[4] = packAndEncode<5>(cmd.value >> 5);
    fields[5] = packAndEncode<5>(cmd.value);

    return fields;
}

std::array<uint8_t, WrRegLong::nFields()> serializeFields(const WrRegLong& cmd)
{
    std::array<uint8_t, WrRegLong::nFields()> fields;

    fields[0] = packAndEncode<4, 1>(cmd.chip_id, 1);
    fields[1] = packAndEncode<5>(cmd.address >> 4);
    fields[2] = packAndEncode<4, 1>(cmd.address, cmd.values[0] >> 15);
    fields[3] = packAndEncode<5>(cmd.values[0] >> 10);
    fields[4] = packAndEncode<5>(cmd.values[0] >> 5);
    fields[5] = packAndEncode<5>(cmd.values[0]);

    bits::unpack_range<5>(cmd.values.begin() + 1, cmd.values.end(), fields.begin() + 6);
    for(auto i = 6u; i < fields.size(); i++) fields[i] = map5to8bit[fields[i]];

    return fields;
}

std::array<uint8_t, RdReg::nFields()> serializeFields(const RdReg& cmd)
{
    std::array<uint8_t, RdReg::nFields()> fields;

    fields[0] = packAndEncode<4, 1>(cmd.chip_id, 0);
    fields[1] = packAndEncode<5>(cmd.address >> 4);
    fields[2] = packAndEncode<4, 1>(cmd.address, 0);
    fields[3] = packAndEncode<5>(0);

    return fields;
}

} // namespace RD53ACmd
