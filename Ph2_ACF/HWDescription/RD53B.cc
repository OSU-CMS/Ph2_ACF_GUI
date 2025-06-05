/*!
  \file                  RD53B.cc
  \brief                 RD53B implementation class
  \author                Mauro DINARDO and Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/22
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#include "RD53B.h"

using namespace Ph2_HwInterface;

namespace Ph2_HwDescription
{
// ########################################
// # Support for different FrontEnd types #
// ########################################
const size_t          RD53B::NROWS   = 336;
const size_t          RD53B::NCOLS   = 432;
const RD53::FrontEnd  RD53B::RD53Bv1 = {"RD53Bv1",
                                        {"DAC_GDAC_M_LIN", "DAC_GDAC_L_LIN", "DAC_GDAC_R_LIN"},
                                        "DAC_KRUM_CURR_LIN",
                                        "TriggerConfig",
                                        "DAC_LDAC_LIN",
                                        "VDDD",
                                        "VDDA",
                                        1,
                                        32,
                                        RD53Shared::setBits(RD53BEvtEncoder::NBIT_TOT) - 1,
                                        7,
                                        4,
                                        4,
                                        0,
                                        RD53B::NCOLS - 1,
                                        0,
                                        0x01,
                                        31,
                                        {{"RstChnSync", 0x0001},
                                         {"RstCmdDecoder", 0x0002},
                                         {"RstGlbConf", 0x0004},
                                         {"RstServiceData", 0x0008},
                                         {"RstAuroraV1", 0x0010},
                                         {"RstSerializerV1", 0x0020},
                                         {"RstADC", 0x0040},
                                         {"RstDataMerging", 0x0080},
                                         {"RstEfuses", 0x0100},
                                         {"RstTrigTable", 0x0200},
                                         {"RstBCIDCnt", 0x0400},
                                         {"SendCalReset", 0x0800},
                                         {"ADCstatConversion", 0x1000},
                                         {"SendStartRingOscA", 0x2000},
                                         {"SendStartRingOscB", 0x4000},
                                         {"SendStartEfusesPrg", 0x800}},
                                        {"EN_CORE_COL_0", "EN_CORE_COL_1", "EN_CORE_COL_2", "EN_CORE_COL_3"}};
const RD53::FrontEnd  RD53B::RD53Bv2 = {"RD53Bv2",
                                        {"DAC_GDAC_M_LIN", "DAC_GDAC_L_LIN", "DAC_GDAC_R_LIN"},
                                        "DAC_KRUM_CURR_LIN",
                                        "TriggerConfig",
                                        "DAC_LDAC_LIN",
                                        "VDDD",
                                        "VDDA",
                                        1,
                                        32,
                                        RD53Shared::setBits(RD53BEvtEncoder::NBIT_TOT) - 1,
                                        7,
                                        4,
                                        4,
                                        0,
                                        RD53B::NCOLS - 1,
                                        0,
                                        0x01,
                                        31,
                                        {{"RstChnSync", 0x0001},
                                         {"RstCmdDecoder", 0x0002},
                                         {"RstGlbConf", 0x0004},
                                         {"RstAuroraV2", 0x0008},
                                         {"RstDataPath", 0x0010},
                                         {"SendClearRstAurora", 0x0020},
                                         {"RstBCIDCnt", 0x0040},       // Reset also L1ID and, Trigger counters
                                         {"SendClearRstBCID", 0x0080}, // Reset also Trigger counter
                                         {"RstSerializerV2", 0x0100},
                                         {"RstADC", 0x0200},
                                         {"RstEfuses", 0x0400},
                                         {"SendCalReset", 0x0800},
                                         {"ADCstatConversion", 0x1000},
                                         {"SendStartRingOscA", 0x2000},
                                         {"SendStartRingOscB", 0x4000},
                                         {"SendStartEfusesPrg", 0x8000}},
                                        {"EN_CORE_COL_0", "EN_CORE_COL_1", "EN_CORE_COL_2", "EN_CORE_COL_3"}};
const RD53::FrontEnd* RD53B::RD53Bvx = &RD53B::RD53Bv2;

std::map<std::string, RD53::SpecialRegInfo> RD53B::specialRegMap = {{"CDR_CONFIG_SEL_SER_CLK", {"CDR_CONFIG", 0}},
                                                                    {"CDR_CONFIG_SEL_PD", {"CDR_CONFIG", 3}},

                                                                    {"CLK_DATA_DELAY_DATA", {"CLK_DATA_DELAY", 0}},
                                                                    {"CLK_DATA_DELAY_CLK", {"CLK_DATA_DELAY", 6}},

                                                                    {"MON_ADC_TRIM", {"MON_ADC", 0}},

                                                                    {"VOLTAGE_TRIM_DIG", {"VOLTAGE_TRIM", 0}},
                                                                    {"VOLTAGE_TRIM_ANA", {"VOLTAGE_TRIM", 4}},

                                                                    {"CML_CONFIG_EN_LANE", {"CML_CONFIG", 0}},
                                                                    {"CML_CONFIG_SER_EN_TAP", {"CML_CONFIG", 4}},
                                                                    {"CML_CONFIG_SER_INV_TAP", {"CML_CONFIG", 6}},

                                                                    {"SER_SEL_OUT_0", {"SER_SEL_OUT", 0}},
                                                                    {"SER_SEL_OUT_1", {"SER_SEL_OUT", 2}},
                                                                    {"SER_SEL_OUT_2", {"SER_SEL_OUT", 4}},
                                                                    {"SER_SEL_OUT_3", {"SER_SEL_OUT", 6}},

                                                                    {"CAL_EDGE_FINE_DELAY", {"CalibrationConfig", 0}},
                                                                    {"ANALOG_INJ_MODE", {"CalibrationConfig", 6}},
                                                                    {"DIGITAL_INJ_EN", {"CalibrationConfig", 7}},

                                                                    {"HIT_SAMPLE_MODE", {"PIX_MODE", 3}},
                                                                    {"EN_SEU_COUNT", {"PIX_MODE", 4}},

                                                                    {"SEL_CAL_RANGE", {"MEAS_CAP", 0}},
                                                                    {"EN_INJCAP_MEAS", {"MEAS_CAP", 1}},
                                                                    {"EN_INJCAP_PAR_MEAS", {"MEAS_CAP", 2}},

                                                                    {"ToT6to4Mapping", {"ToTConfig", 9}},
                                                                    {"ToTDualEdgeCount", {"ToTConfig", 10}},

                                                                    {"EnOutputDataChipId", {"DataMerging", 8}},

                                                                    {"SelfTriggerMultiplier", {"SelfTriggerConfig_0", 0}},
                                                                    {"SelfTriggerDelay", {"SelfTriggerConfig_0", 5}},

                                                                    {"SelfTriggerEn", {"SelfTriggerConfig_1", 5}},

                                                                    {"ServiceFrameSkip", {"ServiceDataConf", 0}},
                                                                    {"EnServiceData", {"ServiceDataConf", 8}},

                                                                    {"ManualChoice", {"PhaseDetectorConfig", 0}},
                                                                    {"ManualMode", {"PhaseDetectorConfig", 1}},
                                                                    {"FixedMode", {"PhaseDetectorConfig", 5}},

                                                                    {"IrefWireBonds", {"PadReadout", 0}},
                                                                    {"ChipIdWireBonds", {"PadReadout", 4}},

                                                                    {"GP_LVDS_OUT_0", {"GP_LVDS_ROUTE_0", 0}},
                                                                    {"GP_LVDS_OUT_1", {"GP_LVDS_ROUTE_0", 6}},
                                                                    {"GP_LVDS_OUT_2", {"GP_LVDS_ROUTE_1", 0}},
                                                                    {"GP_LVDS_OUT_3", {"GP_LVDS_ROUTE_1", 6}},
                                                                    {"GP_LVDS_EN_OUT", {"OUTPUT_PAD_CONFIG", 3}}};

RD53B::RD53B(const FrontEndType& frontEndType,
             uint8_t             pBeId,
             uint8_t             pFMCId,
             uint8_t             pOpticalGroupId,
             uint8_t             pHybridId,
             uint8_t             pRD53Id,
             uint8_t             pRD53Lane,
             int64_t             pRD53eFuseCode,
             const std::string&  fileName,
             const std::string&  cfgComment)
    : RD53(pBeId, pFMCId, pOpticalGroupId, pHybridId, pRD53Id, pRD53Lane, pRD53eFuseCode, fileName, cfgComment)
{
    ReadoutChip::fChipOriginalMask = std::make_shared<RD53ChannelGroup>(RD53B::NROWS, RD53B::NCOLS, true);
    RD53::loadfRegMap(fileName);
    this->setFrontEndType(frontEndType);
    if(frontEndType == FrontEndType::RD53Bv1)
    {
        RD53B::RD53Bvx = &RD53B::RD53Bv1;
        specialRegMap.insert({{"EnEoS", {"DataConcentratorConf", 8}},
                              {"EnLv1Id", {"DataConcentratorConf", 9}},
                              {"EnBCId", {"DataConcentratorConf", 10}},
                              {"EnCRC", {"DataConcentratorConf", 11}},

                              {"RawData", {"CoreColEncoderConf", 7}},
                              {"BinaryReadOut", {"CoreColEncoderConf", 8}}});
    }
    else
    {
        RD53B::RD53Bvx = &RD53B::RD53Bv2;
        specialRegMap.insert({{"EnLv1Id", {"DataConcentratorConf", 8}},
                              {"EnBCId", {"DataConcentratorConf", 9}},
                              {"EnCRC", {"DataConcentratorConf", 10}},

                              {"RawData", {"CoreColEncoderConf", 12}},
                              {"BinaryReadOut", {"CoreColEncoderConf", 13}}});
    }
}

const DataFormatOptions& RD53B::getDataFormatOptions()
{
    dataFormatOptions = DataFormatOptions{bool(this->getRegItem("EnOutputDataChipId").fValue),
                                          !bool(this->getRegItem("BinaryReadOut").fValue),
                                          bool(this->getRegItem("EnBCId").fValue),
                                          bool(this->getRegItem("EnLv1Id").fValue),
                                          bool(this->getRegItem("EnEoS").fValue),
                                          bool(this->getRegItem("RawData").fValue),
                                          bool(this->getRegItem("EnCRC").fValue)};
    return dataFormatOptions;
}

// ###########################################
// # Functions needed for decoding chip data #
// ###########################################

template <class T>
size_t decodeCompressedBitpair(BitView<T>& bits)
{
    if(bits.pop(1) == 0) return 1;
    return 2 | bits.pop(1);
}

template <class T>
auto decodeCompressedHitmap(BitView<T>& bits)
{
    std::array<std::array<bool, 8>, 2> hits{{0}};

    auto row_mask = decodeCompressedBitpair(bits);

    for(size_t row = 0; row < 2; row++)
    {
        if(row_mask & (2 >> row))
        {
            auto quad_mask = decodeCompressedBitpair(bits);

            std::vector<size_t> pair_masks;
            for(size_t i = 0; i < __builtin_popcount(quad_mask); i++)
            {
                auto pair_mask = decodeCompressedBitpair(bits);
                pair_masks.push_back(pair_mask);
            }

            int current_quad = 0;
            for(int pixel_quad = 0; pixel_quad < 2; pixel_quad++)
            {
                if(quad_mask & (2 >> pixel_quad))
                {
                    for(int pixel_pair = 0; pixel_pair < 2; pixel_pair++)
                    {
                        if(pair_masks[current_quad] & (2 >> pixel_pair))
                        {
                            size_t pixel_mask                              = decodeCompressedBitpair(bits);
                            hits[row][pixel_quad * 4 + pixel_pair * 2]     = pixel_mask & 2;
                            hits[row][pixel_quad * 4 + pixel_pair * 2 + 1] = pixel_mask & 1;
                        }
                    }

                    current_quad++;
                }
            }
        }
    }

    return hits;
}

void decodeStreamHeader(BitView<const uint32_t>& bits, RD53ChipEvent& e, const DataFormatOptions& options)
{
    if((options.enableBCID == true) && (options.enableTriggerId == false))
        e.bc_id = bits.pop(RD53BEvtEncoder::NBIT_BCID * 2);
    else if((options.enableBCID == false) && (options.enableTriggerId == true))
        e.trigger_id = bits.pop(RD53BEvtEncoder::NBIT_TRIGID * 2);
    else if((options.enableBCID == true) && (options.enableTriggerId == true))
    {
        e.bc_id      = bits.pop(RD53BEvtEncoder::NBIT_BCID);
        e.trigger_id = bits.pop(RD53BEvtEncoder::NBIT_TRIGID);
    }
    else
    {
        e.bc_id      = RD53Shared::setBits(RD53BEvtEncoder::NBIT_BCID);
        e.trigger_id = RD53Shared::setBits(RD53BEvtEncoder::NBIT_TRIGID);
    }
}

void decodeChipId(uint8_t chipId, size_t i, RD53ChipEvent& e)
{
    if(i == 0)
        e.chip_id_mod4 = chipId;
    else if(e.chip_id_mod4 != chipId)
        e.eventStatus |= RD53EvtEncoder::CHIPID;
}

auto decodeEventStream(BitView<const uint32_t>& bits, RD53ChipEvent& e, const DataFormatOptions& options)
{
    const size_t        nWords = bits.size() / 64;
    BitVector<uint32_t> payloadData;
    bool                isLast = false;

    for(size_t i = 0; i < nWords && !isLast; i++)
    {
        isLast = bits.pop(1);

        if(isLast == true)
        {
            if(i + 2 < nWords)
            {
                e.eventStatus |= RD53EvtEncoder::CHIPNS_WAS1;
                break;
            }
        }
        else if(i == nWords)
        {
            e.eventStatus |= RD53EvtEncoder::CHIPNS_WAS0;
            break;
        }

        if(options.enableChipId)
            decodeChipId(bits.pop(RD53BEvtEncoder::NBIT_CHIPID), i, e);
        else
            e.chip_id_mod4 = RD53Shared::setBits(RD53FWEvtEncoder::NBIT_CHIPID);

        payloadData.append(bits.pop_slice(63 - 2 * options.enableChipId));
    }

    return payloadData;
}

void RD53B::decodeChipData(BitView<const uint32_t> bits, RD53ChipEvent& e, const DataFormatOptions& options)
{
    std::array<int, RD53B::NCOLS / RD53Constants::NROW_CORE> last_qrow;
    last_qrow.fill(RD53B::NROWS / 2);
    const auto eventStream     = decodeEventStream(bits, e, options);
    auto       eventStreamView = bit_view(eventStream);

    if((e.eventStatus & (RD53EvtEncoder::CHIPNS_WAS0 | RD53EvtEncoder::CHIPNS_WAS1)) != 0) return;
    if(eventStreamView.size() == 0)
    {
        e.eventStatus |= RD53FWEvtEncoder::MISSCHIP;
        return;
    }

    e.trigger_tag = eventStreamView.pop(RD53BEvtEncoder::NBIT_TRGTAG);
    decodeStreamHeader(eventStreamView, e, options);

    // ##################
    // # Decode hit map #
    // ##################
    while(true)
    {
        // ##########################
        // # End-of-data conditions #
        // ##########################
        if(eventStreamView.size() < 6) return; // Good end of chip data
        const size_t ccol = eventStreamView.pop(RD53BEvtEncoder::NBIT_CCOL);
        if(ccol == 0) return; // Good end of chip data

        if(RD53Constants::NROW_CORE * (ccol - 1) >= RD53B::NCOLS)
        {
            e.eventStatus |= RD53EvtEncoder::CHIPPIX;
            return;
        }

        bool isLast = false;
        while(isLast == false)
        {
            isLast = eventStreamView.pop(1);
            size_t qrow;
            if(eventStreamView.pop(1) == true) // Check if "is neighbor"
            {
                if(last_qrow[ccol - 1] == RD53B::NROWS / 2) e.eventStatus |= RD53EvtEncoder::CHIPQROW;
                qrow = last_qrow[ccol - 1] + 1;
            }
            else
                qrow = eventStreamView.pop(8);
            last_qrow[ccol - 1] = qrow;

            // ###############################
            // # Detect truncation mechanism #
            // ###############################
            if(2 * qrow >= RD53B::NROWS)
                e.eventStatus |= RD53EvtEncoder::CHIPPIX;
            else if((isLast == true) && (qrow == RD53BEvtEncoder::TRUNC_MAXHITS))
            {
                e.eventStatus |= RD53EvtEncoder::CHIPTRUNC_MAXHITS;
                continue;
            }
            else if((isLast == true) && (qrow == RD53BEvtEncoder::TRUNC_TIMEOUT))
            {
                e.eventStatus |= RD53EvtEncoder::CHIPTRUNC_TIMEOUT;
                continue;
            }

            auto hitmap = decodeCompressedHitmap(eventStreamView);
            for(size_t row = 0; row < 2; row++)
                for(size_t col = 0; col < RD53Constants::NROW_CORE; col++)
                    if(hitmap[row][col])
                    {
                        uint8_t tot = 0;
                        if(options.enableToT == true)
                        {
                            tot = eventStreamView.pop(RD53BEvtEncoder::NBIT_TOT);
                            if(tot == RD53Shared::setBits(RD53BEvtEncoder::NBIT_TOT)) e.eventStatus |= RD53EvtEncoder::CHIPTOT;
                        }

                        e.hit_data.emplace_back(qrow * 2 + row, (ccol - 1) * RD53Constants::NROW_CORE + col, tot);
                    }
        }
    }
}

uint32_t RD53B::getCalCmd(bool cal_edge_mode, size_t cal_edge_delay, size_t cal_edge_width, bool cal_aux_mode, size_t cal_aux_delay)
{
    return bits::pack<1, 5, 8, 1, 5>(cal_edge_mode, cal_edge_delay, cal_edge_width, cal_aux_mode, cal_aux_delay);
}

float RD53B::VCal2Charge(float VCal, bool isNoise) const
{
    const auto Vref        = this->getRegItem("VREF_ADC").fValue / 1000.; // @CONST@ : Conversion from [mV] to [V]
    const auto VrefDivider = (this->getRegItem("SEL_CAL_RANGE").fValue == 0 ? 2 : 1);
    return (Vref / VrefDivider / RD53BchargeConvertion::ADCrange) * VCal / RD53BchargeConvertion::ele * (RD53BchargeConvertion::cap * 1e4) + (isNoise == false ? RD53BchargeConvertion::offset : 0);
}

float RD53B::Charge2VCal(float Charge) const
{
    const auto Vref        = this->getRegItem("VREF_ADC").fValue / 1000.; // @CONST@ : Conversion from [mV] to [V]
    const auto VrefDivider = (this->getRegItem("SEL_CAL_RANGE").fValue == 0 ? 2 : 1);
    return (Charge - RD53BchargeConvertion::offset) / (RD53BchargeConvertion::cap * 1e4) * RD53BchargeConvertion::ele / (Vref / VrefDivider / RD53BchargeConvertion::ADCrange);
}

} // namespace Ph2_HwDescription
