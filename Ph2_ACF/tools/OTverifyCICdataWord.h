/*!
 *
 * \file OTverifyCICdataWord.h
 * \brief OTverifyCICdataWord class
 * \author Fabio Ravera
 * \date 14/02/24
 *
 */

#ifndef OTverifyCICdataWord_h__
#define OTverifyCICdataWord_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTverifyCICdataWord.h"
#endif

namespace Ph2_HwDescription
{
class ReadoutChip;
class Hybrid;
class BeBoard;
} // namespace Ph2_HwDescription

namespace Ph2_HwInterface
{
class D19cFWInterface;
}

class PatternMatcher;
class OTPatternCheckerHelper;

class OTverifyCICdataWord : public Tool
{
  public:
    OTverifyCICdataWord();
    ~OTverifyCICdataWord();

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
    void runIntegrityTest();
    void runL1IntegrityTest(Ph2_HwDescription::BeBoard* theBoard, Ph2_HwInterface::D19cFWInterface* theFWInterface);

    virtual void           injectStubs2S(Ph2_HwDescription::ReadoutChip* theCBC, const std::vector<Stub>& listOfStubs);
    virtual PatternMatcher producePatternMatcher2S(uint8_t chipIdForCIC, uint8_t hybridId, const std::vector<Stub>& listOfStubs);
    virtual void           injectStubsPS(Ph2_HwDescription::ReadoutChip* theMPA, uint8_t numberOfBytesInSinglePacket, const std::vector<Stub>& listOfStubs);
    virtual PatternMatcher producePatternMatcherPS(uint8_t chipIdForCIC, uint8_t numberOfBytesInSinglePacket, const std::vector<Stub>& listOfStubs);

    PatternMatcher         injectL12S(Ph2_HwDescription::ReadoutChip* theChip, uint8_t chipIdForCIC, Ph2_HwInterface::D19cFWInterface* theFWInterface, uint8_t numberOfBytesInSinglePacket);
    virtual PatternMatcher injectL1PS(Ph2_HwDescription::ReadoutChip* theMPA, uint8_t chipIdForCIC, Ph2_HwInterface::D19cFWInterface* theFWInterface, uint8_t numberOfBytesInSinglePacket);
    float                  matchStubPattern(const std::vector<uint32_t>& theWordVector, const PatternMatcher& thePatternMatcher, uint8_t numberOfBytesInSinglePacket, size_t numberOfLines);

    virtual std::vector<std::vector<Stub>> createPSstubList();
    virtual std::vector<std::vector<Stub>> create2SstubList();

    virtual void prepareForStubInjection(Ph2_HwDescription::BeBoard* theBoard) {};

    std::vector<uint32_t> mergeCICStubOuput(const std::vector<std::vector<uint32_t>>& stubLineDataList, uint8_t numberOfBytesInSinglePacket);
    // For simplicity, make sure bendind code is always greater than half value (0x7)
    std::map<uint8_t, uint8_t>          fBendingAndCode{{0, 0x9}, {2, 0xB}, {4, 0xF}};
    float                               fNumberOfStubBits{1e8};
    float                               fNumberOfL1Bits{1e6};
    bool                                fDoMatchingInFirmware{true};
    uint8_t                             prepareCICforStubIntegrityTest(Ph2_HwDescription::Hybrid* theHybrid, uint8_t chipId);
    void                                setUpPatternMatching();
    void                                runStubIntegrityTest(Ph2_HwDescription::BeBoard* theBoard, Ph2_HwInterface::D19cFWInterface* theFWInterface);
    virtual GenericDataArray<float, 2>& getStorageForStubErrorRate(Ph2_HwDescription::Hybrid* theHybrid, uint8_t chipId, uint8_t line, size_t stubPatternCounter);
    virtual GenericDataArray<float, 2>& getStorageForL1ErrorRate(Ph2_HwDescription::Chip* theChip);

    void realignBoardDataWords();

  private:
    DetectorDataContainer fPatternMatchingEfficiencyContainer;
    void                  fillHistograms();
    bool                  fIsKickoff{false};
    void runL1Interations(Ph2_HwInterface::D19cFWInterface* theFWInterface, PatternMatcher& thePatternMatcher, Ph2_HwDescription::ReadoutChip* theChip, uint8_t numberOfBytesInSinglePacket);
    void runStubInterationsSoftwareMatching(Ph2_HwInterface::D19cFWInterface* theFWInterface,
                                            BoardDataContainer&               thePatternContainer,
                                            Ph2_HwDescription::BeBoard*       theBoard,
                                            uint8_t                           numberOfBytesInSinglePacket,
                                            uint8_t                           chipId,
                                            size_t                            stubPatternCounter);
    void
    runStubInterationsFirmwareMatching(BoardDataContainer& thePatternContainer, Ph2_HwDescription::BeBoard* theBoard, uint8_t chipId, uint8_t numberOfLines, bool is10G, size_t stubPatternCounter);
    OTPatternCheckerHelper* fPatternCheckerHelper;

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTverifyCICdataWord fDQMHistogramOTverifyCICdataWord;
#endif
};

#endif
