/*!
  \file                  RD53B.h
  \brief                 RD53B description class
  \author                Mauro DINARDO and Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/22
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#ifndef RD53B_H
#define RD53B_H

#include "RD53.h"
#include "RD53BCommands.h"
#include "Utils/BitMaster/BitVector.h"
#include "Utils/RD53ChannelGroupHandler.h"

// ############################
// # Chip event configuration #
// ############################
namespace RD53BEvtEncoder
{
const uint8_t NBIT_CHIPID     = 2;   // Number of chip ID bits
const uint8_t NBIT_TRIGID     = 8;   // Number of trigger ID bits
const uint8_t NBIT_TRGTAG     = 8;   // Number of trigger tag bits
const uint8_t MAX_TRGTAG      = 215; // Maximum trigger tag value for external triggers
const uint8_t MAX_TRGTAG_ERR1 = 219; // Maximum trigger tag value for error of type 1
const uint8_t MAX_TRGTAG_ERR2 = 223; // Maximum trigger tag value for error of type 2
const uint8_t NBIT_BCID       = 8;   // Number of bunch crossing ID bits
const uint8_t NBIT_TOT        = 4;   // Number of ToT bits
const uint8_t NBIT_CCOL       = 6;   // Number of core column bits
const uint8_t TRUNC_MAXHITS   = 204; // Code for truncation due to max number of hits
const uint8_t TRUNC_TIMEOUT   = 205; // Code for truncation due to readout timeout
} // namespace RD53BEvtEncoder

// ####################################################################################
// # Formula: Vref / ADCrange * VCal / electron_charge [C] * capacitance [F] + offset #
// ####################################################################################
namespace RD53BchargeConvertion
{
const float ADCrange = 4096.0; // VCal total range
const float cap      = 8.0;    // [fF]
const float ele      = 1.6;    // [e-19]
const float offset   = 64;     // Due to VCal_High vs VCal_Med offset difference [e-]
} // namespace RD53BchargeConvertion

namespace Ph2_HwDescription
{
class RD53B : public RD53
{
  public:
    static const size_t    NROWS;
    static const size_t    NCOLS;
    static const FrontEnd  RD53Bv1;
    static const FrontEnd  RD53Bv2;
    static const FrontEnd* RD53Bvx;

    static void decodeChipData(BitView<const uint32_t> bits, Ph2_HwInterface::RD53ChipEvent& e, const DataFormatOptions& options);

    static std::map<std::string, RD53::SpecialRegInfo> specialRegMap;

    RD53B() {}
    RD53B(const FrontEndType& frontEndType,
          uint8_t             pBeId,
          uint8_t             pFMCId,
          uint8_t             pOpticalGroupId,
          uint8_t             pHybridId,
          uint8_t             pRD53Id,
          uint8_t             pRD53Lane,
          int64_t             pRD53eFuseCode,
          const std::string&  fileName,
          const std::string&  cfgComment);
    RD53B(const RD53B&) = delete;

    // #############################
    // # Override member functions #
    // #############################
    size_t getMaxBCIDvalue() const override
    {
        return RD53Shared::setBits(RD53BEvtEncoder::NBIT_BCID * ((this->getRegItem("EnBCId").fValue == true) && (this->getRegItem("EnLv1Id").fValue == true) ? 1 : 2));
    }
    size_t getMaxTRIGIDvalue() const override
    {
        return RD53Shared::setBits(RD53BEvtEncoder::NBIT_TRIGID * ((this->getRegItem("EnBCId").fValue == true) && (this->getRegItem("EnLv1Id").fValue == true) ? 1 : 2));
    }
    const DataFormatOptions& getDataFormatOptions() override;
    const FrontEnd*          getFEtype(const size_t colStart = 0, const size_t colStop = 0) override { return RD53B::RD53Bvx; }
    size_t                   getNRows() const override { return RD53B::NROWS; }
    size_t                   getNCols() const override { return RD53B::NCOLS; }
    std::vector<uint16_t>    getLaneUpInitSequence() const override { return {}; }
    uint32_t                 getCalCmd(bool cal_edge_mode, size_t cal_edge_delay, size_t cal_edge_width, bool cal_aux_mode, size_t cal_aux_delay) override;
    float                    VCal2Charge(float VCal, bool isNoise = false) const override;
    float                    Charge2VCal(float Charge) const override;
    bool                     getUseGainDualSlope() const override { return this->getRegItem("ToT6to4Mapping").fValue == 0 ? false : true; };
    // #############################
};

} // namespace Ph2_HwDescription

#endif
