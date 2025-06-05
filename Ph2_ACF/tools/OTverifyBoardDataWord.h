/*!
 *
 * \file OTverifyBoardDataWord.h
 * \brief OTverifyBoardDataWord class
 * \author Fabio Ravera
 * \date 01/02/24
 *
 */

#ifndef OTverifyBoardDataWord_h__
#define OTverifyBoardDataWord_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTverifyBoardDataWord.h"
#endif

namespace Ph2_HwDescription
{
class BeBoard;
class OpticalGroup;
} // namespace Ph2_HwDescription

namespace Ph2_HwInterface
{
class D19cFWInterface;
}

class OTPatternCheckerHelper;
class PatternMatcher;

class OTverifyBoardDataWord : public Tool
{
  public:
    OTverifyBoardDataWord();
    ~OTverifyBoardDataWord();

    void Initialise(void);

    // State machine
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();

    static std::string fCalibrationDescription;
    uint8_t            fFlagCharacter = 0xea;
    uint8_t            fIdleCharacter = 0xaa;
    uint32_t           fHeader        = 0x0ffffffe;
    uint32_t           fHeaderMask    = 0xffffffff;

  private:
    void runIntegrityTest();

  protected:
    void           runStubIntegrityTestSoftwareMatch(Ph2_HwDescription::BeBoard* theBoard, Ph2_HwInterface::D19cFWInterface* theFWInterface, uint8_t numberOfBytesInSinglePacket);
    void           runStubIntegrityTestFirmwareMatch(Ph2_HwDescription::BeBoard* theBoard, bool runAlignment = false);
    void           runL1IntegrityTest(Ph2_HwDescription::BeBoard*       theBoard,
                                      Ph2_HwInterface::D19cFWInterface* theFWInterface,
                                      uint8_t                           numberOfBytesInSinglePacket,
                                      PatternMatcher&                   thePatternMatcher,
                                      BoardDataContainer*               theAlignmentResultContainer = nullptr);
    bool           isStubPatternMatched(const std::vector<uint32_t>& theWordVector, uint8_t numberOfBytesInSinglePacket, uint8_t flagCharacter, uint8_t idleCharacter);
    bool           isL1HeaderFound(const std::vector<uint32_t>& theWordVector, uint8_t numberOfBytesInSinglePacket, uint32_t header, uint32_t headerMask);
    void           prepareHybridForStubIntegrityTest(Ph2_HwDescription::Hybrid* theHybrid);
    void           prepareHybridForL1IntegrityTest(Ph2_HwDescription::Hybrid* theHybrid);
    void           prepareFWForL1IntegrityTest(Ph2_HwDescription::BeBoard* theBoard, uint32_t theTriggerFrequency = 100);
    uint8_t        getNumberOfBytesInSinglePacket(Ph2_HwDescription::OpticalGroup* cOpticalGroup) const;
    void           setUpPatternMatching();
    PatternMatcher createTheL1PatternMatcher(Ph2_HwDescription::BeBoard* theBoard);

    bool                    fIsKickoff{false};
    bool                    fDoMatchingInFirmware{true};
    DetectorDataContainer   fPatternMatchingBitErrorContainer;
    OTPatternCheckerHelper* fPatternCheckerHelper;
    float                   fNumberOfStubBits{1e8};
    float                   fNumberOfL1Bits{320000};
    bool                    fPrintError{true};

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTverifyBoardDataWord fDQMHistogramOTverifyBoardDataWord;
#endif
};

#endif
