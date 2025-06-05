/*!
  \file                  RD53A.cc
  \brief                 RD53A implementation class
  \author                Mauro DINARDO and Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/22
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#include "RD53A.h"

namespace Ph2_HwDescription
{
// ########################################
// # Support for different FrontEnd types #
// ########################################
const size_t          RD53A::NROWS       = 192;
const size_t          RD53A::NCOLS       = 400;
const RD53::FrontEnd  RD53A::SYNC        = {"SYNC",
                                            {"VTH_SYNC"},
                                            "IBIAS_KRUM_SYNC",
                                            "LATENCY_CONFIG",
                                            "",
                                            "VOUT_dig_ShuLDO",
                                            "VOUT_ana_ShuLDO",
                                            2,
                                            0,
                                            RD53Shared::setBits(RD53AEvtEncoder::NBIT_TOT) - 1,
                                            RD53Shared::setBits(RD53AEvtEncoder::NBIT_TOT) - 1,
                                            5,
                                            5,
                                            0,
                                            127,
                                            50000,
                                            0x08,
                                            8,
                                            {{"RstChnSync", 0x0001},
                                             {"RstCmdDecoder", 0x0002},
                                             {"RstGlbConf", 0x0004},
                                             {"RstServiceData", 0x0008},
                                             {"RstAurora", 0x0010},
                                             {"RstSerializer", 0x0020},
                                             {"RstADC", 0x0040},
                                             {"ADCstatConversion", 0x0100},
                                             {"ActivRinOsc", 0x2000},
                                             {"AcqureZeroSyncFE", 0x4000},
                                             {"RstAutozeroSyncFE", 0x800}},
                                            {"EN_CORE_COL_SYNC"}};
const RD53::FrontEnd  RD53A::LIN         = {"LIN",
                                            {"Vthreshold_LIN"},
                                            "KRUM_CURR_LIN",
                                            "LATENCY_CONFIG",
                                            "LDAC_LIN",
                                            "VOUT_dig_ShuLDO",
                                            "VOUT_ana_ShuLDO",
                                            2,
                                            16,
                                            RD53Shared::setBits(RD53AEvtEncoder::NBIT_TOT) - 1,
                                            RD53Shared::setBits(RD53AEvtEncoder::NBIT_TOT) - 1,
                                            5,
                                            5,
                                            128,
                                            263,
                                            50000,
                                            0x08,
                                            8,
                                            {{"RstChnSync", 0x0001},
                                             {"RstCmdDecoder", 0x0002},
                                             {"RstGlbConf", 0x0004},
                                             {"RstServiceData", 0x0008},
                                             {"RstAurora", 0x0010},
                                             {"RstSerializer", 0x0020},
                                             {"RstADC", 0x0040},
                                             {"ADCstatConversion", 0x0100},
                                             {"ActivRinOsc", 0x2000},
                                             {"AcqureZeroSyncFE", 0x4000},
                                             {"RstAutozeroSyncFE", 0x800}},
                                            {"EN_CORE_COL_LIN_1", "EN_CORE_COL_LIN_2"}};
const RD53::FrontEnd  RD53A::DIFF        = {"DIFF",
                                            {"VTH1_DIFF"},
                                            "VFF_DIFF",
                                            "LATENCY_CONFIG",
                                            "",
                                            "VOUT_dig_ShuLDO",
                                            "VOUT_ana_ShuLDO",
                                            2,
                                            31,
                                            RD53Shared::setBits(RD53AEvtEncoder::NBIT_TOT) - 1,
                                            RD53Shared::setBits(RD53AEvtEncoder::NBIT_TOT) - 1,
                                            5,
                                            5,
                                            264,
                                            399,
                                            50000,
                                            0x08,
                                            8,
                                            {{"RstChnSync", 0x0001},
                                             {"RstCmdDecoder", 0x0002},
                                             {"RstGlbConf", 0x0004},
                                             {"RstServiceData", 0x0008},
                                             {"RstAurora", 0x0010},
                                             {"RstSerializer", 0x0020},
                                             {"RstADC", 0x0040},
                                             {"StartMonitoring", 0x0100},
                                             {"ADCstatConversion", 0x1000},
                                             {"ActivRinOsc", 0x2000},
                                             {"AcqureZeroSyncFE", 0x4000},
                                             {"RstAutozeroSyncFE", 0x800}},
                                            {"EN_CORE_COL_DIFF_1", "EN_CORE_COL_DIFF_2"}};
const RD53::FrontEnd* RD53A::frontEnds[] = {&RD53A::SYNC, &RD53A::LIN, &RD53A::DIFF};

const std::map<std::string, RD53::SpecialRegInfo> RD53A::specialRegMap = {{"CDR_CONFIG_SEL_SER_CLK", {"CDR_CONFIG", 0}},

                                                                          {"CLK_DATA_DELAY_DATA", {"CLK_DATA_DELAY", 0}},
                                                                          {"CLK_DATA_DELAY_CLK", {"CLK_DATA_DELAY", 4}},
                                                                          {"CLK_DATA_DELAY_2INV", {"CLK_DATA_DELAY", 5}},

                                                                          {"MONITOR_CONFIG_ADC", {"MonitorConfig", 0}},
                                                                          {"MONITOR_CONFIG_BG", {"MonitorConfig", 6}},

                                                                          {"VOLTAGE_TRIM_DIG", {"VOLTAGE_TRIM", 0}},
                                                                          {"VOLTAGE_TRIM_ANA", {"VOLTAGE_TRIM", 5}},

                                                                          {"CML_CONFIG_EN_LANE", {"CML_CONFIG", 0}},
                                                                          {"CML_CONFIG_SER_EN_TAP", {"CML_CONFIG", 4}},
                                                                          {"CML_CONFIG_SER_INV_TAP", {"CML_CONFIG", 6}},

                                                                          {"SER_SEL_OUT_0", {"SER_SEL_OUT", 0}},
                                                                          {"SER_SEL_OUT_1", {"SER_SEL_OUT", 2}},
                                                                          {"SER_SEL_OUT_2", {"SER_SEL_OUT", 4}},
                                                                          {"SER_SEL_OUT_3", {"SER_SEL_OUT", 6}},

                                                                          {"CAL_EDGE_FINE_DELAY", {"INJECTION_SELECT", 0}},
                                                                          {"DIGITAL_INJ_EN", {"INJECTION_SELECT", 4}},
                                                                          {"ANALOG_INJ_MODE", {"INJECTION_SELECT", 5}}};

RD53A::RD53A(uint8_t            pBeId,
             uint8_t            pFMCId,
             uint8_t            pOpticalGroupId,
             uint8_t            pHybridId,
             uint8_t            pRD53Id,
             uint8_t            pRD53Lane,
             int64_t            pRD53eFuseCode,
             const std::string& fileName,
             const std::string& cfgComment)
    : RD53(pBeId, pFMCId, pOpticalGroupId, pHybridId, pRD53Id, pRD53Lane, pRD53eFuseCode, fileName, cfgComment)
{
    ReadoutChip::fChipOriginalMask = std::make_shared<RD53ChannelGroup>(RD53A::NROWS, RD53A::NCOLS, true);
    RD53::loadfRegMap(fileName);
    this->setFrontEndType(FrontEndType::RD53A);
}

const DataFormatOptions& RD53A::getDataFormatOptions()
{
    dataFormatOptions = DataFormatOptions{true, true, true, true, false, false, false};
    return dataFormatOptions;
}

std::vector<uint16_t> RD53A::getLaneUpInitSequence() const
{
    const int             nWordsReset    = 500;  // @CONST@
    const int             nWordsSequence = 2000; // @CONST@
    std::vector<uint16_t> initSequence;

    for(auto i = 0u; i < nWordsReset; i++) initSequence.push_back(0x0000);    // 0000 0000
    for(auto i = 0u; i < nWordsSequence; i++) initSequence.push_back(0xCCCC); // 1100 1100

    return initSequence;
}

const RD53A::FrontEnd* RD53A::getFEtype(const size_t colStart, const size_t colStop)
{
    return *std::max_element(std::begin(frontEnds),
                             std::end(frontEnds),
                             [&](const FrontEnd* a, const FrontEnd* b)
                             { return int(std::min(colStop, a->colStop)) - int(std::max(colStart, a->colStart)) < int(std::min(colStop, b->colStop)) - int(std::max(colStart, b->colStart)); });
}

// ###########################################
// # Functions needed for decoding chip data #
// ###########################################

void RD53A::decodeChipData(const uint32_t* data, size_t size, Ph2_HwInterface::RD53ChipEvent& chipEvent)
{
    uint32_t header;

    std::tie(header, chipEvent.trigger_id, chipEvent.trigger_tag, chipEvent.bc_id) =
        bits::unpack<RD53AEvtEncoder::NBIT_HEADER, RD53AEvtEncoder::NBIT_TRIGID, RD53AEvtEncoder::NBIT_TRGTAG, RD53AEvtEncoder::NBIT_BCID>(*data);
    if(header != RD53AEvtEncoder::HEADER) chipEvent.eventStatus |= RD53EvtEncoder::CHIPHEAD;

    const size_t noHitToT = RD53Shared::setBits(RD53AEvtEncoder::NBIT_TOT * RD53Constants::NPIX_REGION);

    for(auto i = 1u; i < size; i++)
    {
        if(data[i] != noHitToT)
        {
            uint32_t core_col, side, row, col, all_tots;

            std::tie(core_col, row, side, all_tots) =
                bits::unpack<RD53AEvtEncoder::NBIT_CCOL, RD53AEvtEncoder::NBIT_ROW, RD53AEvtEncoder::NBIT_SIDE, RD53AEvtEncoder::NBIT_TOT * RD53Constants::NPIX_REGION>(data[i]);
            col = RD53Constants::NPIX_REGION * bits::pack<RD53AEvtEncoder::NBIT_CCOL, RD53AEvtEncoder::NBIT_SIDE>(core_col, side);

            uint8_t tots[RD53Constants::NPIX_REGION];
            bits::RangePacker<RD53AEvtEncoder::NBIT_TOT>::unpack_reverse(all_tots, tots);

            for(int j = 0; j < RD53Constants::NPIX_REGION; j++)
                if(tots[j] != RD53Shared::setBits(RD53AEvtEncoder::NBIT_TOT)) chipEvent.hit_data.push_back({(uint16_t)row, (uint16_t)(col + j), tots[j]});
            if((row >= RD53A::NROWS) || (col >= (RD53A::NCOLS - (RD53Constants::NPIX_REGION - 1)))) chipEvent.eventStatus |= RD53EvtEncoder::CHIPPIX;
        }
    }

    // ########################################################
    // # If the number of 32bit words do not make an integer  #
    // # number of 128bit words, then special words are added #
    // # to the event                                         #
    // ########################################################
    if(size == 1) chipEvent.eventStatus |= RD53EvtEncoder::CHIPNOHIT;
}

uint32_t RD53A::getCalCmd(bool cal_edge_mode, size_t cal_edge_delay, size_t cal_edge_width, bool cal_aux_mode, size_t cal_aux_delay)
{
    return bits::pack<4, 1, 3, 6, 1, 5>(RD53A::getFEtype()->broadcastChipId, cal_edge_mode, cal_edge_delay, cal_edge_width, cal_aux_mode, cal_aux_delay);
}

float RD53A::VCal2Charge(float VCal, bool isNoise) const
{
    const auto Vref = this->getRegItem("VREF_ADC").fValue / 1000.; // @CONST@ : Conversion from [mV] to [V]
    return (Vref / RD53AchargeConvertion::ADCrange) * VCal / RD53AchargeConvertion::ele * (RD53AchargeConvertion::cap * 1e4) + (isNoise == false ? RD53AchargeConvertion::offset : 0);
}

float RD53A::Charge2VCal(float Charge) const
{
    const auto Vref = this->getRegItem("VREF_ADC").fValue / 1000.; // @CONST@ : Conversion from [mV] to [V]
    return (Charge - RD53AchargeConvertion::offset) / (RD53AchargeConvertion::cap * 1e4) * RD53AchargeConvertion::ele / (Vref / RD53AchargeConvertion::ADCrange);
}

} // namespace Ph2_HwDescription
