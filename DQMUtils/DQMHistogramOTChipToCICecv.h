/*!
        \file                DQMHistogramOTChipToCICecv.h
        \brief               DQM class for OTChipToCICecv
        \author              Fabio Ravera
        \date                19/03/25
*/

#ifndef DQMHistogramOTChipToCICecv_h_
#define DQMHistogramOTChipToCICecv_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTChipToCICecv
 * \brief Class for OTChipToCICecv monitoring histograms
 */
class DQMHistogramOTChipToCICecv : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTChipToCICecv();

    /*!
     * destructor
     */
    ~DQMHistogramOTChipToCICecv();

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

    void fillPhaseScanMatchingEfficiency(DetectorDataContainer& matchingEfficiencyContainer, uint8_t cicPhase, uint8_t slvsCurrent);

  private:
    DetectorContainer*                       fDetectorContainer;
    std::map<uint8_t, DetectorDataContainer> fErrorRateContainerMap;
    std::map<uint8_t, DetectorDataContainer> fTestedBitsContainerMap;
};
#endif
