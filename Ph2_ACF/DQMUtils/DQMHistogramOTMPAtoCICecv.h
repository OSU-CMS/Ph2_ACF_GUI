/*!
        \file                DQMHistogramOTMPAtoCICecv.h
        \brief               DQM class for OTMPAtoCICecv
        \author              Fabio Ravera
        \date                28/05/24
*/

#ifndef DQMHistogramOTMPAtoCICecv_h_
#define DQMHistogramOTMPAtoCICecv_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTMPAtoCICecv
 * \brief Class for OTMPAtoCICecv monitoring histograms
 */
class DQMHistogramOTMPAtoCICecv : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTMPAtoCICecv();

    /*!
     * destructor
     */
    ~DQMHistogramOTMPAtoCICecv();

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

    void fillPhaseScanMatchingEfficiency(DetectorDataContainer& thePhaseMatchingEfficiency, uint8_t phase, uint8_t slvsCurrent);

  private:
    DetectorContainer*                       fDetectorContainer;
    std::map<uint8_t, DetectorDataContainer> fPhaseScanMatchingEfficiencies;
};
#endif
