/*!
        \file                                            SSA2Interface.h
        \brief                                           User Interface to the SSA2s
        \author                                          Marc Osherson
        \version                                         1.0
        \date                        Jan 20201
        Support :                    mail to : oshersonmarc@gmail.com

 */

#ifndef __SSA2INTERFACE_H__
#define __SSA2INTERFACE_H__

#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/ReadoutChipInterface.h"
#include <fstream>
#include <iostream> // std::cout
#include <string>
#include <vector>

namespace Ph2_HwInterface
{ // start namespace

class SSA2Interface : public ReadoutChipInterface
{ // begin class
  public:
    SSA2Interface(const BeBoardFWMap& pBoardMap);
    ~SSA2Interface();
    SSA2Interface(const SSA2Interface&)            = delete;
    SSA2Interface& operator=(const SSA2Interface&) = delete;
    bool           ConfigureChip(Ph2_HwDescription::Chip* pSSA2, bool pVerify = false, uint32_t pBlockSize = 310) override; // FIXME
    void           DumpConfiguration(Ph2_HwDescription::Chip* pSSA2, std::string filename);                                 // FIXME

    void producePhaseAlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip, uint8_t pWait_ms = 10) override {}

    bool setInjectionSchema(Ph2_HwDescription::ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool pVerify = true) override;                                        // FIXME
    bool enableInjection(Ph2_HwDescription::ReadoutChip* pChip, bool inject, bool pVerify = true) override;                                                                             // FIXME
    bool setInjectionAmplitude(Ph2_HwDescription::ReadoutChip* pChip, uint8_t injectionAmplitude, bool pVerify = true) override;                                                        // FIXME
    bool maskChannelGroup(Ph2_HwDescription::ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool pVerify = true) override;                                          // FIXME
    bool maskChannelsAndSetInjectionSchema(Ph2_HwDescription::ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool mask, bool inject, bool pVerify = true) override; // FIXME
    bool ConfigureChipOriginalMask(Ph2_HwDescription::ReadoutChip* pSSA2, bool pVerify = true, uint32_t pBlockSize = 310) override;                                                     // FIXME
    bool MaskAllChannels(Ph2_HwDescription::ReadoutChip* pSSA2, bool mask, bool pVerify = true) override;                                                                               // FIXME
    bool WriteChipReg(Ph2_HwDescription::Chip* pSSA2, const std::string& pRegNode, uint16_t pValue, bool pVerify = true) override;                                                      // FIXME
    bool WriteChipMultReg(Ph2_HwDescription::Chip* pSSA2, const std::vector<std::pair<std::string, uint16_t>>& pVecReq, bool pVerify = true) override;                                  // FIXME
    bool WriteChipAllLocalReg(Ph2_HwDescription::ReadoutChip* pSSA2, const std::string& dacName, const ChipContainer& pValue, bool pVerify = true) override;                            // FIXME
    std::vector<std::pair<std::string, uint16_t>> ReadChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& theRegisterList) override;

    int32_t  ReadChipReg(Ph2_HwDescription::Chip* pSSA2, const std::string& pRegNode) override;
    uint32_t ReadADC(Ph2_HwDescription::ReadoutChip* pSSA2, uint8_t pInput);
    uint32_t readADC(Ph2_HwDescription::ReadoutChip* pSSA2, std::string pRegName, uint16_t numberOfRead = 1);
    uint32_t readADCGround(Ph2_HwDescription::ReadoutChip* pSSA2) override;
    uint32_t readADCBandGap(Ph2_HwDescription::ReadoutChip* pSSA2);
    uint32_t readADCVref(Ph2_HwDescription::ReadoutChip* pSSA2);
    uint32_t readVrefRegister(Ph2_HwDescription::ReadoutChip* pSSA2);
    uint32_t ReadChipFuseID(Ph2_HwDescription::Chip* pSSA2, uint8_t version = 1) override;
    bool     setVrefFromFuseID(Ph2_HwDescription::ReadoutChip* pSSA2) override;
    bool     setVref(Ph2_HwDescription::ReadoutChip* pSSA2, uint16_t theVrefRegisterValue) override;
    float    calculateADCLSB(Ph2_HwDescription::ReadoutChip* pSSA2, float theVrefValue = SSA2_VREF_EXPECTED) override;
    bool     disableTestPadsOutput(Ph2_HwDescription::ReadoutChip* pSSA2);
    // bool     selectTestPadsOutput(Ph2_HwDescription::ReadoutChip* pMPA2, std::string theRegisterName);
    bool injectNoiseClusters(Ph2_HwDescription::ReadoutChip* pSSA2, std::vector<Cluster> theClusterList);

    const std::map<std::string, std::pair<uint8_t, float>> getBiasStructureDefaultTable(Ph2_HwDescription::ReadoutChip* pSSA2);

    float getBandGapExpectedValue(Ph2_HwDescription::ReadoutChip* pSSA2);
    float getVrefExpectedValue(Ph2_HwDescription::ReadoutChip* pSSA2);
    float getVrefPrecision(Ph2_HwDescription::ReadoutChip* pSSA2);
    float getVrefMinValue(Ph2_HwDescription::ReadoutChip* pSSA2);
    float getVrefMaxValue(Ph2_HwDescription::ReadoutChip* pSSA2);
    bool  WriteChipRegBits(Ph2_HwDescription::Chip* theSSA, const std::string& pRegNode, uint16_t pValue, const std::string& pMaskReg, uint8_t mask, bool pVerify = false);

    std::pair<std::pair<std::string, uint16_t>, std::vector<std::pair<std::string, uint16_t>>>
    packLocalRegisters(Ph2_HwDescription::ReadoutChip* pSSA2, const std::string& dacName, const ChipContainer& localRegValues);

  private:
    uint8_t ReadChipId(Ph2_HwDescription::Chip* pChip);                                                                                                                           // FIXME
    bool    WriteChipRegBitsLocal(Ph2_HwDescription::Chip* pSSA2, const std::string& pRegNode, uint16_t pValue, const std::string& pMaskReg, uint8_t mask, bool pVerify = false); // FIXME
    // bool    ConfigureAmux(Ph2_HwDescription::Chip* pChip, const std::string& pRegister, bool pVerify = true);                                                                // FIXME

    const std::map<std::string, uint8_t> SSA2_ADC_CONTROL_TABLE = {
        {"highimpedence", 0}, {"Bias_D5BFEED", 1},   {"Bias_D5PREAMP", 2}, {"Bias_D5TDR", 3}, {"Bias_D5ALLV", 4}, {"Bias_D5ALLI", 5}, {"Bias_CALDAC", 6}, {"Bias_BOOSTERBASELINE", 7},
        {"Bias_THDAC", 8},    {"Bias_THDACHIGH", 9}, {"Bias_D5DAC8", 10},  {"VBG", 11},       {"GND", 12},        {"ADC_IREF", 13},   {"ADC_VREF", 14},   {"TESTPAD", 15},
        {"temperature", 16},  {"AVDD", 17},          {"PVDD", 18},         {"DVDD", 19}};

    // Map of the bias structure registers
    // < register name , <default register value (DAC) , expected value in V on the test pad>
    typedef std::pair<uint8_t, float>              DAC_expectedValue;
    const std::map<std::string, DAC_expectedValue> SSA2_BIAS_STRUCTURE_DEFAULT = {{"Bias_D5BFEED", std::make_pair(0x0F, 0.082)},
                                                                                  {"Bias_D5PREAMP", std::make_pair(0x0F, 0.082)},
                                                                                  {"Bias_D5TDR", std::make_pair(0x0F, 0.115)},
                                                                                  {"Bias_D5ALLV", std::make_pair(0x0F, 0.082)},
                                                                                  {"Bias_D5ALLI", std::make_pair(0x0F, 0.082)},
                                                                                  {"Bias_D5DAC8", std::make_pair(0x0F, 0.086)}};

    // std::map<std::string, uint8_t> fAmuxMap = {{"BoosterFeedback", 0}, // FIXMEEEE this map is wrong!! the one in ReadADC is correct                                                  // FIXME
    //                                            {"PreampBias", 1},
    //                                            {"Trim", 2},
    //                                            {"VoltageBias", 3},
    //                                            {"CurrentBias", 4},
    //                                            {"CalLevel", 5},
    //                                            {"BoosterBaseline", 6},
    //                                            {"Threshold", 7},
    //                                            {"HipThreshold", 8},
    //                                            {"DAC", 9},
    //                                            {"Bandgap", 10},
    //                                            {"GND", 11},
    //                                            {"HighZ", 12}};

}; // end class

} // namespace Ph2_HwInterface

#endif
