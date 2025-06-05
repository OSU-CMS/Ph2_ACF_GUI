/*!

        \file                   MPA2.h
        \brief                  MPA2 Description class, config of the MPA2s
        \author                 Kevin Nash
        \version                1.0
        \date                   12/06/21
        Support :               mail to : knash201@gmail.com

 */

#ifndef __MPA2INTERFACE_H__
#define __MPA2INTERFACE_H__

#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/D19clpGBTInterface.h"
#include "HWInterface/ReadoutChipInterface.h"
#include "pugixml.hpp"
#include <vector>

/*!
 * \namespace Ph2_HwInterface
 * \brief Namespace regrouping all the interfaces to the hardware
 */
namespace Ph2_HwInterface
{
using BeBoardFWMap = std::map<uint16_t, BeBoardFWInterface*>; /*!< Map of Board connected */

struct Injection
{
    uint8_t fRow;
    uint8_t fColumn;
    uint8_t fChipId;
};

/*!
 * \class MPA2Interface
 * \brief Class representing the User Interface to the MPA on different boards
 */

/// @brief
class MPA2Interface : public ReadoutChipInterface
{ // begin class
  public:
    MPA2Interface(const BeBoardFWMap& pBoardMap);
    ~MPA2Interface();

    void     setFileHandler(FileHandler* pHandler);
    bool     ConfigureChip(Ph2_HwDescription::Chip* pMPA, bool pVerify = false, uint32_t pBlockSize = 310) override;
    uint32_t ReadData(Ph2_HwDescription::BeBoard* pBoard, bool pBreakTrigger, std::vector<uint32_t>& pData, bool pWait);
    void     ReadMPA(Ph2_HwDescription::ReadoutChip* pMPA);

    bool WriteChipRegBits(Ph2_HwDescription::Chip* pMPA, const std::string& pRegNode, uint16_t pValue, const std::string& pMaskReg, uint8_t mask, bool pVerify = false);
    bool WriteChipReg(Ph2_HwDescription::Chip* pMPA, const std::string& pRegName, uint16_t pValue, bool pVerify = true) override;
    bool WriteChipMultReg(Ph2_HwDescription::Chip* pMPA, const std::vector<std::pair<std::string, uint16_t>>& pVecReq, bool pVerify = false) override;
    bool WriteChipAllLocalReg(Ph2_HwDescription::ReadoutChip* pMPA, const std::string& dacName, const ChipContainer& pValue, bool pVerify = false) override;

    int32_t                                       ReadChipReg(Ph2_HwDescription::Chip* pMPA, const std::string& pRegName) override;
    std::vector<std::pair<std::string, uint16_t>> ReadChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& theRegisterList) override;
    uint32_t                                      ReadChipFuseID(Ph2_HwDescription::Chip* pMPA2, uint8_t version = 1) override;

    void producePhaseAlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip, uint8_t pWait_ms = 10) override;
    void produceWordAlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip) override;
    void produceBX0AlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip) override;

    // void                  Activate_async(Ph2_HwDescription::Chip* pMPA);
    // void                  Activate_sync(Ph2_HwDescription::Chip* pMPA);
    void Activate_pp(Ph2_HwDescription::Chip* pMPA, uint8_t win = 0);
    void Activate_ss(Ph2_HwDescription::Chip* pMPA, uint8_t win = 0);
    void Activate_ps(Ph2_HwDescription::Chip* pMPA, uint8_t win = 8);
    // uint32_t Read_pixel_counter(Ph2_HwDescription::ReadoutChip* pMPA, uint32_t p);
    void Pix_Smode(Ph2_HwDescription::ReadoutChip* pMPA, uint32_t p, std::string smode);
    void Enable_pix_BRcal(Ph2_HwDescription::ReadoutChip* pMPA, uint32_t p, std::string polarity = "rise", std::string smode = "edge");
    void Pix_Set_enable(Ph2_HwDescription::ReadoutChip* pMPA,
                        uint32_t                        p,
                        uint32_t                        PixelMask,
                        uint32_t                        Polarity,
                        uint32_t                        EnEdgeBR,
                        uint32_t                        EnLevelBR,
                        uint32_t                        Encount,
                        uint32_t                        DigCal,
                        uint32_t                        AnCal,
                        uint32_t                        BRclk);

    bool Set_calibration(Ph2_HwDescription::Chip* pMPA, uint32_t cal);
    bool setThreshold(Ph2_HwDescription::Chip* pMPA, uint8_t threshold);
    bool setInjectionDelay(Ph2_HwDescription::Chip* pMPA, uint8_t delay);

    /*!
     * @brief set same register for all MPA bias block in
     * @param pMPA2 the MPA to write
     * @param registerName Name of the register without the bias block number
     * @param value value to write in the register
     * @return success
     */
    bool setAllBiasBlockRegisters(Ph2_HwDescription::Chip* pMPA2, std::string registerName, uint8_t value);

    void Send_pulses(uint32_t n_pulse, uint32_t duration = 0);
    bool enableInjection(Ph2_HwDescription::ReadoutChip* pChip, bool inject, bool pVerify = false);

    bool maskChannelGroup(Ph2_HwDescription::ReadoutChip* pMPA, const std::shared_ptr<ChannelGroupBase> group, bool pVerify = false) override;
    //
    bool maskChannelsAndSetInjectionSchema(Ph2_HwDescription::ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool mask, bool inject, bool pVerify = false);

    bool setInjectionSchema(Ph2_HwDescription::ReadoutChip* pCbc, const std::shared_ptr<ChannelGroupBase> group, bool pVerify = false);

    bool ConfigureChipOriginalMask(Ph2_HwDescription::ReadoutChip* pMPA, bool pVerify, uint32_t pBlockSize);
    //
    bool MaskAllChannels(Ph2_HwDescription::ReadoutChip* pMPA, bool mask, bool pVerify = true);
    bool disableTestPadsOutput(Ph2_HwDescription::ReadoutChip* pMPA2);
    // bool selectTestPadsOutput(Ph2_HwDescription::ReadoutChip* pMPA2, std::string theRegisterName);

    std::vector<uint8_t> getWordAlignmentPatterns() override { return fWordAlignmentPatterns; }
    std::vector<uint8_t> getBX0AlignmentPatterns() override { return fBX0AlignmentPatterns; }
    void                 Cleardata();
    //
    void                 digiInjection(Ph2_HwDescription::ReadoutChip* pChip, std::vector<Injection> pInjections, uint8_t pPattern = 0xFF);
    std::vector<int>     decodeBendCode(Ph2_HwDescription::ReadoutChip* pChip, uint8_t pBendCode);
    std::vector<uint8_t> readLUT(Ph2_HwDescription::ReadoutChip* pChip, uint8_t pMode = 0);
    bool                 configPixel(Ph2_HwDescription::Chip* pChip, std::string cReg, uint16_t row, uint16_t col, uint8_t pValue, bool pVerify);
    uint16_t             readPixel(Ph2_HwDescription::Chip* pChip, std::string cReg, uint16_t row, uint16_t col);
    bool                 setVrefFromFuseID(Ph2_HwDescription::ReadoutChip* pMPA2) override;
    bool                 setVref(Ph2_HwDescription::ReadoutChip* pMPA2, uint16_t theVrefRegisterValue) override;
    float                ADCMeasure(Ph2_HwDescription::Chip* pMPA2, uint8_t block, uint8_t testPoint = 0, uint8_t swEn = 0, uint32_t nreads = 5);
    bool                 selectBlock(Ph2_HwDescription::Chip* pMPA2, uint8_t block, uint8_t testPoint = 0, uint8_t swEn = 0);
    uint32_t             readADCGround(Ph2_HwDescription::ReadoutChip* pMPA2);
    uint32_t             readADC(Ph2_HwDescription::ReadoutChip* pMPA2, std::string pRegName, uint16_t numberOfRead = 5);
    uint32_t             readADCVref(Ph2_HwDescription::ReadoutChip* pMPA2);
    uint32_t             readVrefRegister(Ph2_HwDescription::ReadoutChip* pMPA2);
    uint32_t             readADCBandGap(Ph2_HwDescription::ReadoutChip* pMPA2);

    const std::map<std::string, std::pair<uint8_t, float>> getBiasStructureDefaultTable(Ph2_HwDescription::ReadoutChip* pMPA);

    float getBandGapExpectedValue(Ph2_HwDescription::ReadoutChip* pMPA2);
    float getVrefExpectedValue(Ph2_HwDescription::ReadoutChip* pMPA2);
    float getVrefPrecision(Ph2_HwDescription::ReadoutChip* pMPA2);
    float getVrefMinValue(Ph2_HwDescription::ReadoutChip* pMPA2);
    float getVrefMaxValue(Ph2_HwDescription::ReadoutChip* pMPA2);

    float                                calculateADCLSB(Ph2_HwDescription::ReadoutChip* pMPA2, float theVrefValue = MPA2_VREF_EXPECTED) override;
    bool                                 injectNoiseClusters(Ph2_HwDescription::ReadoutChip* pMPA, std::vector<Cluster> theClusterList);
    const std::map<std::string, uint8_t> ECM_TABLE = {{"StubWindow", 0}, {"StubMode", 6}};

    std::pair<std::pair<std::string, uint16_t>, std::vector<std::pair<std::string, uint16_t>>>
    packLocalRegisters(Ph2_HwDescription::ReadoutChip* pMPA2, const std::string& dacName, const ChipContainer& localRegValues);

  private:
    std::vector<std::pair<std::string, uint16_t>> fTriggerReadAdcSequence{{"Mask", 0xE0}, {"ADCcontrol", 0xE0}, {"ADCcontrol", 0xC0}, {"Mask", 0xFF}};

    // pixelEnable bits
    const std::map<std::string, uint8_t> PIXEL_ENABLE_TABLE =
        {{"PixelMask", 0}, {"Polarity", 1}, {"EnEdgeBR", 2}, {"EnLvlBR", 3}, {"CounterEnable", 4}, {"DigitalInjection", 5}, {"AnalogueInjection", 6}, {"BrClk", 7}};

    const std::map<std::string, uint8_t> CONTROL_TABLE = {{"ReadoutMode", 0}, {"RetimePix", 2}, {"PhaseShift", 5}}; // I think the doc should read 3,3,2 for the bits -- to check

    // MPA2 ADC Multiplexer control
    // < register name , < block for AMUX selection, switch sel (shift) for analog bias >>
    typedef std::pair<uint8_t, uint8_t>       block_switch;
    const std::map<std::string, block_switch> ADC_CONTROL_TABLE = {
        {"disabled", std::make_pair(0, 0)}, // not sure on the shift selection when we are not looking at a bias
        {"A0", std::make_pair(1, 0)},       {"A1", std::make_pair(2, 0)},      {"A2", std::make_pair(3, 0)},        {"A3", std::make_pair(4, 0)},           {"A4", std::make_pair(5, 0)},
        {"A5", std::make_pair(6, 0)},       {"A6", std::make_pair(7, 0)},      {"B0", std::make_pair(1, 1)},        {"B1", std::make_pair(2, 1)},           {"B2", std::make_pair(3, 1)},
        {"B3", std::make_pair(4, 1)},       {"B4", std::make_pair(5, 1)},      {"B5", std::make_pair(6, 1)},        {"B6", std::make_pair(7, 1)},           {"C0", std::make_pair(1, 2)},
        {"C1", std::make_pair(2, 2)},       {"C2", std::make_pair(3, 2)},      {"C3", std::make_pair(4, 2)},        {"C4", std::make_pair(5, 2)},           {"C5", std::make_pair(6, 2)},
        {"C6", std::make_pair(7, 2)},       {"D0", std::make_pair(1, 3)},      {"D1", std::make_pair(2, 3)},        {"D2", std::make_pair(3, 3)},           {"D3", std::make_pair(4, 3)},
        {"D4", std::make_pair(5, 3)},       {"D5", std::make_pair(6, 3)},      {"D6", std::make_pair(7, 3)},        {"E0", std::make_pair(1, 4)},           {"E1", std::make_pair(2, 4)},
        {"E2", std::make_pair(3, 4)},       {"E3", std::make_pair(4, 4)},      {"E4", std::make_pair(5, 4)},        {"E5", std::make_pair(6, 4)},           {"E6", std::make_pair(7, 4)},
        {"ThDAC0", std::make_pair(1, 5)},   {"ThDAC1", std::make_pair(2, 5)},  {"ThDAC2", std::make_pair(3, 5)},    {"ThDAC3", std::make_pair(4, 5)},       {"ThDAC4", std::make_pair(5, 5)},
        {"ThDAC5", std::make_pair(6, 5)},   {"ThDAC6", std::make_pair(7, 5)},  {"CalDAC0", std::make_pair(1, 6)},   {"CalDAC1", std::make_pair(2, 6)},      {"CalDAC2", std::make_pair(3, 6)},
        {"CalDAC3", std::make_pair(4, 6)},  {"CalDAC4", std::make_pair(5, 6)}, {"CalDAC5", std::make_pair(6, 6)},   {"CalDAC6", std::make_pair(7, 6)},      {"GND", std::make_pair(1, 7)},
        {"VBG", std::make_pair(8, 0)},      {"dac_ref", std::make_pair(9, 0)}, {"ADC_VREF", std::make_pair(10, 0)}, {"temperature", std::make_pair(11, 0)}, {"AVDD", std::make_pair(12, 0)},
        {"io_vdd", std::make_pair(13, 0)},  {"DVDD", std::make_pair(14, 0)}};

    // Map of the bias structure registers
    // < register name , <default register value (DAC) , expected value in V on the test pad>
    typedef std::pair<uint8_t, float>              DAC_expectedValue;
    const std::map<std::string, DAC_expectedValue> MPA2_BIAS_STRUCTURE_DEFAULT = {{"A0", std::make_pair(0x0F, 0.082)},
                                                                                  {"A1", std::make_pair(0x0F, 0.082)},
                                                                                  {"A2", std::make_pair(0x0F, 0.082)},
                                                                                  {"A3", std::make_pair(0x0F, 0.082)},
                                                                                  {"A4", std::make_pair(0x0F, 0.082)},
                                                                                  {"A5", std::make_pair(0x0F, 0.082)},
                                                                                  {"A6", std::make_pair(0x0F, 0.082)},
                                                                                  {"B0", std::make_pair(0x0F, 0.082)},
                                                                                  {"B1", std::make_pair(0x0F, 0.082)},
                                                                                  {"B2", std::make_pair(0x0F, 0.082)},
                                                                                  {"B3", std::make_pair(0x0F, 0.082)},
                                                                                  {"B4", std::make_pair(0x0F, 0.082)},
                                                                                  {"B5", std::make_pair(0x0F, 0.082)},
                                                                                  {"B6", std::make_pair(0x0F, 0.082)},
                                                                                  // {"C0", std::make_pair(0x0F, 0.108)},
                                                                                  // {"C1", std::make_pair(0x0F, 0.108)},
                                                                                  // {"C2", std::make_pair(0x0F, 0.108)},
                                                                                  // {"C3", std::make_pair(0x0F, 0.108)},
                                                                                  // {"C4", std::make_pair(0x0F, 0.108)},
                                                                                  // {"C5", std::make_pair(0x0F, 0.108)},
                                                                                  // {"C6", std::make_pair(0x0F, 0.108)},
                                                                                  {"D0", std::make_pair(0x0F, 0.082)},
                                                                                  {"D1", std::make_pair(0x0F, 0.082)},
                                                                                  {"D2", std::make_pair(0x0F, 0.082)},
                                                                                  {"D3", std::make_pair(0x0F, 0.082)},
                                                                                  {"D4", std::make_pair(0x0F, 0.082)},
                                                                                  {"D5", std::make_pair(0x0F, 0.082)},
                                                                                  {"D6", std::make_pair(0x0F, 0.082)},
                                                                                  {"E0", std::make_pair(0x0F, 0.082)},
                                                                                  {"E1", std::make_pair(0x0F, 0.082)},
                                                                                  {"E2", std::make_pair(0x0F, 0.082)},
                                                                                  {"E3", std::make_pair(0x0F, 0.082)},
                                                                                  {"E4", std::make_pair(0x0F, 0.082)},
                                                                                  {"E5", std::make_pair(0x0F, 0.082)},
                                                                                  {"E6", std::make_pair(0x0F, 0.082)}};

    // MPA2 periphery config register map
    const std::map<std::string, std::pair<uint8_t, uint8_t>> PERI_CONFIG_TABLE = {{"Control_1", std::pair<uint8_t, uint8_t>{0x11, 0}}, // MPA2 has 0x11 and 0x12 peri blocks
                                                                                  {"ECM", std::pair<uint8_t, uint8_t>{0x11, 1}},
                                                                                  {"ErrorL1", std::pair<uint8_t, uint8_t>{0x12, 0}},
                                                                                  {"LatencyRx320", std::pair<uint8_t, uint8_t>{0x11, 24}},
                                                                                  {"OutSetting_1_0", std::pair<uint8_t, uint8_t>{0x11, 12}},
                                                                                  {"OutSetting_2_1", std::pair<uint8_t, uint8_t>{0x11, 13}},
                                                                                  {"OutSetting_4_3", std::pair<uint8_t, uint8_t>{0x11, 14}}};

    // MPA2 row config register map -- some overlap in naming, to find a better way
    const std::map<std::string, uint8_t> ROW_CONFIG_TABLE = {{"MemoryControl_1", 0}, {"MemoryControl_2", 1}, {"PixelControl", 2}, {"Mask", 13}, {"L1Offset_1", 0}, {"L1Offset_2", 1}};

    // MPA2 pixel config register map
    const std::map<std::string, uint8_t> PIXEL_CONFIG_TABLE = {{"ENFLAGS", 0}, {"TrimDAC", 1}, {"DigiPattern", 2}, {"ACCounter_LSB", 4}, {"ACCounter_MSB", 5}};
    std::map<uint16_t, std::string>      fMap;
    std::vector<uint8_t>                 fWordAlignmentPatterns = {0x7A, 0x7A, 0x7A, 0x7A, 0x7A, 0x7A};
    std::vector<uint8_t>                 fBX0AlignmentPatterns  = {0x92, 0x48, 0x12, 0x48, 0xD8}; // stubs
    // std::vector<uint8_t>                 fBX0AlignmentPatterns  = {0x80, 0x00, 0x00, 0x00, 0x00}; // sync bit

    bool                WriteChipSingleReg(Ph2_HwDescription::Chip* pMPA, const std::string& pRegNode, uint16_t pValue, bool pVerify = false);
    uint16_t            ReadChipSingleReg(Ph2_HwDescription::Chip* pMPA, const std::string& pRegNode);
    bool                maskPixel(Ph2_HwDescription::Chip* pChip, uint16_t row, uint16_t col, bool doMask, bool pVerify = false);
    bool                enablePixelInjection(Ph2_HwDescription::Chip* pChip, uint16_t row, uint16_t col, bool inject, bool pVerify);
    void                readAllBias(Ph2_HwDescription::Chip* pMPA);
    std::pair<int, int> extractMaskedPixelAddress(const std::string& registerName) const;
};
} // namespace Ph2_HwInterface

#endif
