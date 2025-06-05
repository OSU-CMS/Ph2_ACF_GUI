/*!

        \file                                            PSInterface.h
        \brief                                           User Interface to the PSs
        \author                                          Lorenzo BIDEGAIN, Nicolas PIERRE
        \version                                         1.0
        \date                        31/07/14
        Support :                    mail to : lorenzo.bidegain@gmail.com, nico.pierre@icloud.com

 */

#ifndef __PSINTERFACE_H__
#define __PSINTERFACE_H__

#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/MPA2Interface.h"
#include "HWInterface/ReadoutChipInterface.h"
#include "HWInterface/SSA2Interface.h"

#include "pugixml.hpp"
#include <vector>

/*!
 * \namespace Ph2_HwInterface
 * \brief Namespace regrouping all the interfaces to the hardware
 */
namespace Ph2_HwInterface
{
using BeBoardFWMap = std::map<uint16_t, BeBoardFWInterface*>; /*!< Map of Board connected */

/*!
 * \class PSInterface
 * \brief Class representing the User Interface to the PS on different boards
 */

class PSInterface : public ReadoutChipInterface
{ // begin class
  private:
    // I2C config
    bool                 fRetryI2C              = true;
    uint8_t              fMaxI2CAttempts        = 20;
    std::vector<uint8_t> fWordAlignmentPatterns = {0x7A, 0x7A, 0x7A, 0x7A, 0x7A};
    std::vector<uint8_t> fBX0AlignmentPatterns  = {0x92, 0x48, 0x12, 0x48, 0xD8};
    // std::vector<uint8_t> fBX0AlignmentPatterns  = {0x80, 0x00, 0x00, 0x00, 0x00};
  public:
    PSInterface(const BeBoardFWMap& pBoardMap);
    ~PSInterface();

    Ph2_HwInterface::SSA2Interface* fTheSSA2Interface;
    Ph2_HwInterface::MPA2Interface* fTheMPA2Interface;

    std::map<FrontEndType, ReadoutChipInterface*> CHIP_INTERFACE;
    ReadoutChipInterface*                         getInterface(Ph2_HwDescription::Chip* pPS);

    void                 setFileHandler(FileHandler* pHandler);
    bool                 ConfigureChip(Ph2_HwDescription::Chip* pPS, bool pVerifLoop = true, uint32_t pBlockSize = 310) override;
    std::vector<uint8_t> readLUT(Ph2_HwDescription::ReadoutChip* pChip, uint8_t pMode = 0);

    bool                                          WriteChipReg(Ph2_HwDescription::Chip* pPS, const std::string& pRegName, uint16_t pValue, bool pVerifLoop = true) override;
    bool                                          WriteChipMultReg(Ph2_HwDescription::Chip* pPS, const std::vector<std::pair<std::string, uint16_t>>& pVecReq, bool pVerifLoop = true) override;
    bool                                          WriteChipAllLocalReg(Ph2_HwDescription::ReadoutChip* pPS, const std::string& dacName, const ChipContainer& pValue, bool pVerifLoop = true) override;
    int32_t                                       ReadChipReg(Ph2_HwDescription::Chip* pPS, const std::string& pRegName) override;
    std::vector<std::pair<std::string, uint16_t>> ReadChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& theRegisterList) override;
    uint32_t                                      ReadChipFuseID(Ph2_HwDescription::Chip* pPS, uint8_t version = 1) override;

    void                 producePhaseAlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip, uint8_t pWait_ms = 10) override;
    void                 produceWordAlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip) override;
    void                 produceBX0AlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip) override;
    std::vector<uint8_t> getWordAlignmentPatterns() override { return fWordAlignmentPatterns; }
    std::vector<uint8_t> getBX0AlignmentPatterns() override { return fBX0AlignmentPatterns; }
    void                 digiInjection(Ph2_HwDescription::ReadoutChip* pChip, std::vector<Injection> pInjections, uint8_t pPattern = 0x01);
    std::vector<int>     decodeBendCode(Ph2_HwDescription::ReadoutChip* pChip, uint8_t pBendCode);
    bool                 enableInjection(Ph2_HwDescription::ReadoutChip* pChip, bool inject, bool pVerifLoop = true);

    bool maskChannelGroup(Ph2_HwDescription::ReadoutChip* pPS, const std::shared_ptr<ChannelGroupBase> group, bool pVerifLoop);

    bool maskChannelsAndSetInjectionSchema(Ph2_HwDescription::ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool mask, bool inject, bool pVerifLoop = false);

    uint32_t readADCGround(Ph2_HwDescription::ReadoutChip* pPS);
    uint32_t readADC(Ph2_HwDescription::ReadoutChip* pPS, std::string theADCName, uint16_t numberOfRead = 5);
    float    readADCVoltage(Ph2_HwDescription::ReadoutChip* pPS, std::string theADCName);
    float    measureTemperature(Ph2_HwDescription::ReadoutChip* pPS);
    uint32_t readADCBandGap(Ph2_HwDescription::ReadoutChip* pPS);
    uint32_t readADCVref(Ph2_HwDescription::ReadoutChip* pPS);
    uint32_t readVrefRegister(Ph2_HwDescription::ReadoutChip* pPS);
    bool     setVref(Ph2_HwDescription::ReadoutChip* pPS, uint16_t theVrefRegisterValue);
    bool     setVrefFromFuseID(Ph2_HwDescription::ReadoutChip* pPS);

    float calculateADCLSB(Ph2_HwDescription::ReadoutChip* pPS, float theVrefValue) override;

    const std::map<std::string, std::pair<uint8_t, float>> getBiasStructureDefaultTable(Ph2_HwDescription::ReadoutChip* pPS);

    float getBandGapExpectedValue(Ph2_HwDescription::ReadoutChip* pPS);
    float getVrefExpectedValue(Ph2_HwDescription::ReadoutChip* pPS);
    float getVrefPrecision(Ph2_HwDescription::ReadoutChip* pPS);
    float getVrefMinValue(Ph2_HwDescription::ReadoutChip* pPS);
    float getVrefMaxValue(Ph2_HwDescription::ReadoutChip* pPS);

    bool setInjectionSchema(Ph2_HwDescription::ReadoutChip* pCbc, const std::shared_ptr<ChannelGroupBase> group, bool pVerifLoop = false);

    //
    bool ConfigureChipOriginalMask(Ph2_HwDescription::ReadoutChip* pPS, bool pVerifLoop, uint32_t pBlockSize);
    //
    bool MaskAllChannels(Ph2_HwDescription::ReadoutChip* pPS, bool mask, bool pVerifLoop) override;
    bool disableTestPadsOutput(Ph2_HwDescription::ReadoutChip* pPS);
    // bool selectTestPadsOutput(Ph2_HwDescription::ReadoutChip* pPS, std::string theRegisterName);
    uint8_t TuneDAC(Ph2_HwDescription::ReadoutChip* theChip, float theSlope, float theExpectedValue, std::string theDACtoTuneName, uint8_t theDACValue, bool isVref = false);
    void    SetOptical()
    {
        bool cFoundLpgbt = this->lpGBTFound();
        fTheSSA2Interface->setWithLpGBT(cFoundLpgbt);
        fTheMPA2Interface->setWithLpGBT(cFoundLpgbt);
    }
    bool injectNoiseClusters(Ph2_HwDescription::ReadoutChip* pPS, std::vector<Cluster> theClusterList);
    bool injectNoiseStubs(Ph2_HwDescription::ReadoutChip* pMPA, Ph2_HwDescription::ReadoutChip* pSSA, std::vector<std::tuple<uint8_t, uint8_t, int>> theStubVector);
    // void                              printErrorSummary();
};
} // namespace Ph2_HwInterface

#endif
