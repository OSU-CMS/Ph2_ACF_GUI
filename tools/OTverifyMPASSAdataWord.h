/*!
 *
 * \file OTverifyMPASSAdataWord.h
 * \brief OTverifyMPASSAdataWord class
 * \author Fabio Ravera
 * \date 07/03/24
 *
 */

#ifndef OTverifyMPASSAdataWord_h__
#define OTverifyMPASSAdataWord_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTverifyMPASSAdataWord.h"
#endif
#include "tools/OTverifyCICdataWord.h"

class PatternMatcher;

class OTverifyMPASSAdataWord : public OTverifyCICdataWord
{
  public:
    OTverifyMPASSAdataWord();
    ~OTverifyMPASSAdataWord();

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
    uint8_t                fFirstStrip{15};
    uint8_t                fStripGap{10};
    std::map<int, uint8_t> fBendingToCode{{0, 7}};

    std::vector<Cluster>                produceMatchingPixelClusterList(uint8_t stubRow, uint8_t stubSeed);
    virtual std::vector<Cluster>        produceStripClusterList();
    virtual void                        setStubLogicParameters(Ph2_HwDescription::ReadoutChip* theMPA);
    virtual void                        setStripOffsetParameters(Ph2_HwDescription::ReadoutChip* theSSA);
    virtual void                        prepareForStubInjection(Ph2_HwDescription::BeBoard* theBoard) override;
    DetectorDataContainer               fPatternMatchingEfficiencyContainer;
    std::vector<Cluster>                fListOfInjectedStrips;
    virtual GenericDataArray<float, 2>& getStorageForStubErrorRate(Ph2_HwDescription::Hybrid* theHybrid, uint8_t chipId, uint8_t line, size_t stubPatternCounter) override;
    GenericDataArray<float, 2>&         getStorageForL1ErrorRate(Ph2_HwDescription::Chip* theChip) override;

  private:
    void                                   fillHistograms();
    void                                   injectStubsPS(Ph2_HwDescription::ReadoutChip* theMPA, uint8_t numberOfBytesInSinglePacket, const std::vector<Stub>& listOfStubs) override;
    PatternMatcher                         producePatternMatcherPS(uint8_t chipIdForCIC, uint8_t numberOfBytesInSinglePacket, const std::vector<Stub>& listOfStubs) override;
    virtual std::vector<std::vector<Stub>> createPSstubList() override;

    PatternMatcher injectL1PS(Ph2_HwDescription::ReadoutChip* theMPA, uint8_t chipIdForCIC, Ph2_HwInterface::D19cFWInterface* theFWInterface, uint8_t numberOfBytesInSinglePacket) override;
    PatternMatcher produceStubPatternMatcher(const std::vector<Stub>& theStubVector, uint8_t numberOfBytesInSinglePacket, uint8_t chipIdForCIC);
    PatternMatcher produceL1PatternMatcher(const std::vector<Cluster>& thePixelClusterList, const std::vector<Cluster>& theStripClusterList, uint8_t numberOfBytesInSinglePacket, uint8_t chipIdForCIC);

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTverifyMPASSAdataWord fDQMHistogramOTverifyMPASSAdataWord;
#endif
};

#endif
