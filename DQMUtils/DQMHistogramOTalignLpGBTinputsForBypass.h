/*!
        \file                DQMHistogramOTalignLpGBTinputsForBypass.h
        \brief               DQM class for OTalignLpGBTinputsForBypass
        \author              Fabio Ravera
        \date                31/05/24
*/

#ifndef DQMHistogramOTalignLpGBTinputsForBypass_h_
#define DQMHistogramOTalignLpGBTinputsForBypass_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTalignLpGBTinputsForBypass
 * \brief Class for OTalignLpGBTinputsForBypass monitoring histograms
 */
class DQMHistogramOTalignLpGBTinputsForBypass : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTalignLpGBTinputsForBypass();

    /*!
     * destructor
     */
    ~DQMHistogramOTalignLpGBTinputsForBypass();

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

    void fillMatchingEfficiency(DetectorDataContainer& matchingEfficiencyContainer, uint8_t phyPort, uint8_t lpgbtPhase);
    void fillBestPhase(DetectorDataContainer& bestPhaseContainer, uint8_t phyPort);

  private:
    DetectorContainer*                       fDetectorContainer;
    std::map<uint8_t, DetectorDataContainer> fPhaseScanMatchingBitErrorRateContainer;
    std::map<uint8_t, DetectorDataContainer> fPhaseScanMatchingTestedBitsContainer;
    std::map<uint8_t, DetectorDataContainer> fBestPhase;
};
#endif
