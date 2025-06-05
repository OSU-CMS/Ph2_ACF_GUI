/*!
  \file                  RD53BCommands.cc
  \brief                 RD53BCommands implementation
  \author                Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/22
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#include "RD53BCommands.h"

namespace RD53BCmd
{
std::vector<uint8_t> serializeFields(const WrReg& cmd)
{
    std::vector<uint8_t> fields;

    fields.reserve(6);
    fields.push_back(packAndEncode<1, 4>(0, cmd.address >> 5));
    fields.push_back(packAndEncode<5>(cmd.address));
    fields.push_back(packAndEncode<5>(cmd.value >> 11));
    fields.push_back(packAndEncode<5>(cmd.value >> 6));
    fields.push_back(packAndEncode<5>(cmd.value >> 1));
    fields.push_back(packAndEncode<1, 4>(cmd.value, 0));

    return fields;
}

std::vector<uint8_t> serializeFields(const WrRegLong& cmd)
{
    std::vector<uint8_t> fields;

    fields.reserve(2 + cmd.values.size() * 2);
    fields.push_back(packAndEncode<1, 4>(1, 0));
    fields.push_back(packAndEncode<5>(0));
    for(const auto& value: cmd.values)
    {
        fields.push_back(packAndEncode<5>(value >> 5));
        fields.push_back(packAndEncode<5>(value));
    }

    return fields;
}

std::vector<uint8_t> serializeFields(const Cal& cmd)
{
    std::vector<uint8_t> fields;

    fields.reserve(4);
    fields.push_back(packAndEncode<1, 4>(cmd.mode, cmd.edge_delay >> 1));
    fields.push_back(packAndEncode<1, 4>(cmd.edge_delay, cmd.edge_duration >> 4));
    fields.push_back(packAndEncode<4, 1>(cmd.edge_duration, cmd.aux_enable));
    fields.push_back(packAndEncode<5>(cmd.aux_delay));

    return fields;
}

std::vector<uint8_t> serializeFields(const RdReg& cmd)
{
    std::vector<uint8_t> fields;

    fields.reserve(2);
    fields.push_back(packAndEncode<1, 4>(0, cmd.address >> 5));
    fields.push_back(packAndEncode<5>(cmd.address));

    return fields;
}

} // namespace RD53BCmd
