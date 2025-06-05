/*!
        \file                DQMHistogramOTVTRxLightYieldScan.h
        \brief               DQM class for OTVTRxLightYieldScan
        \author              Fabio Ravera
        \date                12/09/24
*/

#ifndef DQMHistogramOTVTRxLightYieldScan_h_
#define DQMHistogramOTVTRxLightYieldScan_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTVTRxLightYieldScan
 * \brief Class for OTVTRxLightYieldScan monitoring histograms
 */
class DQMHistogramOTVTRxLightYieldScan : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTVTRxLightYieldScan();

    /*!
     * destructor
     */
    ~DQMHistogramOTVTRxLightYieldScan();

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

    void fillOpticalPower(DetectorDataContainer& theOpticalPowerContainer, uint8_t biasValue, uint8_t modulationValue);

  private:
    DetectorContainer*    fDetectorContainer;
    DetectorDataContainer fLightYieldScanContainer;
};
#endif
