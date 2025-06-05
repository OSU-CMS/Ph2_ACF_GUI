/*!
        \file                DQMHistogramOTPScommonNoise.h
        \brief               DQM class for OTPScommonNoise
        \author              Irene Zoi
        \date                15/08/24
*/

#ifndef DQMHistogramOTPScommonNoise_h_
#define DQMHistogramOTPScommonNoise_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "TH1F.h"
#include "TH2F.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"
#include "Utils/GenericDataArray.h"

class TFile;

/*!
 * \class DQMHistogramOTPScommonNoise
 * \brief Class for OTPScommonNoise monitoring histograms
 */
class DQMHistogramOTPScommonNoise : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTPScommonNoise();

    /*!
     * destructor
     */
    ~DQMHistogramOTPScommonNoise();

    /*!
     * \brief Book histograms
     * \param theOutputFile : where histograms will be saved
     * \param theDetectorStructure : Detector container as obtained after file parsing, used to create histograms for
     * all board/chip/hybrid/channel \param pSettingsMap : setting as for Tool setting map in case coe informations are
     * needed (i.e. FitSCurve)
     */
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap) override;

    /*!
     * \brief fill : fill histograms from TCP stream, need to be overwritten to avoid compilation errors, but it is not
     * needed if you do not fo into the SoC \param dataBuffer : vector of char with the TCP datastream
     */
    bool fill(std::string& inputStream) override;

    /*!
     * \brief process : do something with the histogram like colors, fit, drawing canvases, etc
     */
    void process() override;

    /*!
     * \brief Reset histogram
     */
    void reset(void) override;
    void fillChipHitPlots(DetectorDataContainer& theHitData, float numberOfSigma);
    void fillHybridHitPlots(DetectorDataContainer& theHitData, bool isStrip, float numberOfSigma);
    void fillModuleHitPlots(DetectorDataContainer& theHitData, bool isStrip, float numberOfSigma);
    void fillSSAtoMPACorrelationPlots(DetectorDataContainer& theHitData, float numberOfSigma);
    void fillStripPixelHybridCorrelationPlots(DetectorDataContainer& theHitData, float numberOfSigma);
    void fillStripPixelModuleCorrelationPlots(DetectorDataContainer& theHitData, float numberOfSigma);

    template <size_t T2>
    void fillEventsVsHitsHist(const BaseDataContainer* ChipContainer, TH1F& theHistogram)
    {
        const GenericDataArray<uint32_t, T2>& cDataSummary = ChipContainer->getSummary<GenericDataArray<uint32_t, T2>>();
        for(uint16_t iChan = 0; iChan < T2; iChan++) { theHistogram.SetBinContent(iChan + 1, cDataSummary.at(iChan)); }
        theHistogram.Sumw2();
    }

    template <size_t T1, size_t T2>
    void fillCorrelationHist(const BaseDataContainer* ChipContainer, TH2F* theHistogram)
    {
        const GenericDataArray<uint32_t, T1, T2>& cDataSummary = ChipContainer->getSummary<GenericDataArray<uint32_t, T1, T2>>();
        for(uint16_t iChan2 = 0; iChan2 < T2; iChan2++)
        {
            for(uint16_t iChan1 = 0; iChan1 < T1; iChan1++) { theHistogram->SetBinContent(iChan1, iChan2, cDataSummary.at(iChan1).at(iChan2)); }
        }
        theHistogram->Sumw2();
    }

  private:
    DetectorContainer*                                        fDetectorContainer;
    std::map<float, DetectorDataContainer>                    fStripHitHistograms;
    std::map<float, DetectorDataContainer>                    fPixelHitHistograms;
    std::map<float, DetectorDataContainer>                    fStripHybridHitHistograms;
    std::map<float, DetectorDataContainer>                    fPixelHybridHitHistograms;
    std::map<float, DetectorDataContainer>                    fStripModuleHitHistograms;
    std::map<float, DetectorDataContainer>                    fPixelModuleHitHistograms;
    std::map<float, std::map<uint8_t, DetectorDataContainer>> fSSAtoMPAcorrelation;
    std::map<float, DetectorDataContainer>                    fStripPixelModuleHistograms;
    std::map<float, DetectorDataContainer>                    fStripPixelHybridHistograms;
};
#endif
