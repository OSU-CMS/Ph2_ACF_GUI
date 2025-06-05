/*!
 *
 * \file OTalignLpGBTinputsForBypass.h
 * \brief OTalignLpGBTinputsForBypass class
 * \author Fabio Ravera
 * \date 31/05/24
 *
 */

#ifndef OTalignLpGBTinputsForBypass_h__
#define OTalignLpGBTinputsForBypass_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTalignLpGBTinputsForBypass.h"
#endif
#include "Utils/PatternMatcher.h"

class OTPatternCheckerHelper;

class OTalignLpGBTinputsForBypass : public Tool
{
  public:
    OTalignLpGBTinputsForBypass();
    ~OTalignLpGBTinputsForBypass();

    void Initialise(void);

    // State machine
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();

    static std::string fCalibrationDescription;

  protected:
    void    runPatternMatching(Ph2_HwDescription::BeBoard* theBoard, std::map<uint8_t, DetectorDataContainer>& errorRatePerPhyPortMap, bool isPS, uint8_t phyPort);
    float   fNumberOfTestedBits{1e6};
    float   fNumberOfTestedBitsL12S{1e5};
    uint8_t fShiftRegisterPatternMPA{0xAA};
    void    produceAllPatternAndMasks(Ph2_HwDescription::BeBoard* theBoard);
    void    prepareMPAtoSendPatterns(Ph2_HwDescription::BeBoard* theBoard);
    void    prepare2StoSendStubPatterns(Ph2_HwDescription::BeBoard* theBoard);
    void    prepare2StoSendL1Patterns(Ph2_HwDescription::BeBoard* theBoard);
    void    setCICBypass(Ph2_HwDescription::BeBoard* theBoard, uint8_t phyPort);
    void    preparePatternChecker();

  private:
    void                                                    AlignLpGBTinputs();
    uint8_t                                                 getBestPhase(const GenericDataArray<float, 15, 2>& thePhaseEfficiencyList, Ph2_HwDescription::Hybrid* theHybrid, uint8_t line);
    GenericDataArray<float, 2>                              getMatchingEfficiency2SL1(std::vector<uint32_t> inputDataVector);
    std::pair<std::vector<uint32_t>, std::vector<uint32_t>> getFullPatternAndMask(uint32_t thePattern, bool is10G);

    std::vector<uint8_t> fStubPattern2S{0x25, 0x55, 0xAA, 0xAA, 0xAA}; // last two bytes cannot be changed here

    PatternMatcher          fPattern2SL1;
    OTPatternCheckerHelper* fPatternCheckerHelper;

    std::map<uint8_t, BoardDataContainer> fPatternAndMaskContainerMap;

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTalignLpGBTinputsForBypass fDQMHistogramOTalignLpGBTinputsForBypass;
#endif
};

#endif
