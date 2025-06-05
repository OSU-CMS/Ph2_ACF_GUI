/*!
        \file                DQMHistogramPedeNoise.h
        \brief               base class to create and fill monitoring histograms
        \author              Fabio Ravera, Lorenzo Uplegger, Younes Otarid
        \version             1.0
        \date                6/5/19
        Support :            mail to : fabio.ravera@cern.ch
        Support :            mail to : younes.otarid@cern.ch
*/

#ifndef __DQMHISTOGRAMPEDENOISE_H__
#define __DQMHISTOGRAMPEDENOISE_H__
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramPedeNoise
 * \brief Class for PedeNoise monitoring histograms
 */
class DQMHistogramPedeNoise : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramPedeNoise();

    /*!
     * destructor
     */
    ~DQMHistogramPedeNoise();

    /*!
     * Book histograms
     */
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap) override;

    /*!
     * Fill histogram
     */
    bool fill(std::string& inputStream) override;

    /*!
     * Save histogram
     */
    void process() override;

    /*!
     * Reset histogram
     */
    void reset(void) override;
    // virtual void summarizeHistos();

    /*!
     * \brief Fill validation histograms
     * \param theOccupancy : DataContainer for the occupancy
     */
    // void fillValidationPlots(DetectorDataContainer& theOccupancy);

    /*!
     * \brief Fill validation histograms
     * \param theOccupancy : DataContainer for pedestal and occupancy
     */
    void fillPedestalAndNoisePlots(DetectorDataContainer& thePedestalAndNoise);

    /*!
     * \brief Fill SCurve histograms
     * \param fSCurveOccupancyMap : maps of Vthr and DataContainer
     */
    void fillSCurvePlots(uint16_t pStripTh, uint16_t pPixelTh, DetectorDataContainer& fSCurveOccupancy);

  private:
    DetectorContainer*    fDetectorContainer;
    void                  fitSCurves();
    void                  clearPedestalAndNoisePlots();
    uint32_t              fNPixelChannels = 0, fNStripChannels = 0;
    DetectorDataContainer fThresholdAndNoiseContainer;

    DetectorDataContainer fDetectorHybridNoiseDistributionHistograms;
    DetectorDataContainer fDetectorHybridStripNoiseDistributionHistograms;
    DetectorDataContainer fDetectorHybridPixelNoiseDistributionHistograms;
    DetectorDataContainer fDetectorHybridStripNoiseHistograms;
    DetectorDataContainer fDetectorHybridPixelNoiseHistograms;
    DetectorDataContainer fDetectorHybridStripNoiseBottomHistograms; // only for CBC
    DetectorDataContainer fDetectorHybridStripNoiseTopHistograms;    // only for CBC

    DetectorDataContainer fDetectorChipStripSCurveHistograms;
    DetectorDataContainer fDetectorChipPixelSCurveHistograms;

    DetectorDataContainer fDetectorChannelStripSCurveHistograms;
    DetectorDataContainer fDetectorChannelPixelSCurveHistograms;

    // DetectorDataContainer fDetectorStripValidationHistograms;
    // DetectorDataContainer fDetectorPixelValidationHistograms;

    DetectorDataContainer fDetectorChipStripPedestalHistograms;
    DetectorDataContainer fDetectorChipPixelPedestalHistograms;

    DetectorDataContainer fDetectorChipStripNoiseHistograms;
    DetectorDataContainer fDetectorChipPixelNoiseHistograms;

    DetectorDataContainer fDetectorChannelStripNoiseHistograms;
    DetectorDataContainer fDetectorChannelPixelNoiseHistograms;

    DetectorDataContainer fDetectorChannelStripPedestalHistograms;
    DetectorDataContainer fDetectorChannelPixelPedestalHistograms;

    DetectorDataContainer fDetectorChannel2DPixelNoiseHistograms;

    DetectorDataContainer fDetectorChannelStripNoiseBottomHistograms;
    DetectorDataContainer fDetectorChannelStripNoiseTopHistograms;

    bool fWithCBC = false;
    bool fWithSSA = false;
    bool fWithMPA = false;

    bool fPlotSCurves{false};
    bool fFitSCurves{false};
};
#endif
