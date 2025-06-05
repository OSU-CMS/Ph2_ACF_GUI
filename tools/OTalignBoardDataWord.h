/*!
 *
 * \file OTalignBoardDataWord.h
 * \brief OTalignBoardDataWord class
 * \author Fabio Ravera
 * \date 19/01/24
 *
 */

#ifndef OTalignBoardDataWord_h__
#define OTalignBoardDataWord_h__

#include "tools/Tool.h"
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTalignBoardDataWord.h"
#endif

namespace Ph2_HwInterface
{
class D19cBackendAlignmentFWInterface;
class D19cDebugFWInterface;
} // namespace Ph2_HwInterface
namespace Ph2_HwDescription
{
class BeBoard;
}

class OTalignBoardDataWord : public Tool
{
  public:
    OTalignBoardDataWord();
    ~OTalignBoardDataWord();

    void Initialise(void);

    // State machine
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();

    static std::string fCalibrationDescription;
    bool               tryLineAlignment(Ph2_HwInterface::D19cBackendAlignmentFWInterface* theAlignerInterface, Ph2_HwDescription::Hybrid* theHybrid, uint8_t lineId);
    void               setProducePlots(bool doProducePlots) { fProducePlots = doProducePlots; }
    void               wordAlignBEdata();
    void               setSuppressErrorPrintout(bool suppressErrorPrintout) { fSuppressErrorPrintout = suppressErrorPrintout; }

  protected:
    void initializeContainers();
    void runAlignment(Ph2_HwDescription::BeBoard* theBoard);
    int  fBroadcastAlignSetting{0}; // 0 = one line at a time - 1 = one hybrid at a time - 2 = all hybrids in parallel
    bool opticalGroupWordAlignment(const Ph2_HwDescription::OpticalGroup* theOpticalGroup, Ph2_HwInterface::D19cBackendAlignmentFWInterface* theAlignerInterface);
    bool fSuppressErrorPrintout{false};

  private:
    DetectorDataContainer fBitSlipContainer;
    DetectorDataContainer fAlignmentRetryContainer;

    void boardWordAlignment(Ph2_HwDescription::BeBoard* theBoard);
    bool tryAllLineAlignment(Ph2_HwInterface::D19cBackendAlignmentFWInterface* theAlignerInterface, Ph2_HwDescription::Hybrid* theHybrid);
    bool tryAllHybridAlignment(Ph2_HwInterface::D19cBackendAlignmentFWInterface* theAlignerInterface, Ph2_HwDescription::BeBoard* theBoard);

    bool skip2SkickOff(uint16_t hybridId, uint8_t lineId, bool is2Smodule);

    int     fMaxNumberOfIterations{10};
    uint8_t fNumberOfLines;

    void disableUnalignedHybrid(Ph2_HwDescription::Hybrid* theHybrid);

    bool fProducePlots{true};
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTalignBoardDataWord fDQMHistogramOTalignBoardDataWord;
#endif
};

#endif
