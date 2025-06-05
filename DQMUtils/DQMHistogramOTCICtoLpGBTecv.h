/*!
        \file                DQMHistogramOTCICtoLpGBTecv.h
        \brief               DQM class for OTCICtoLpGBTecv
        \author              Irene Zoi
        \date                23/05/24
*/

#ifndef DQMHistogramOTCICtoLpGBTecv_h_
#define DQMHistogramOTCICtoLpGBTecv_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTCICtoLpGBTecv
 * \brief Class for OTCICtoLpGBTecv monitoring histograms
 */
class DQMHistogramOTCICtoLpGBTecv : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTCICtoLpGBTecv();

    /*!
     * destructor
     */
    ~DQMHistogramOTCICtoLpGBTecv();

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
    void fillEfficiency(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, uint8_t pPhase, DetectorDataContainer& theEfficiencyContainer);

    /*!
     * \brief process : do something with the histogram like colors, fit, drawing canvases, etc
     */
    void process() override;

    /*!
     * \brief Reset histogram
     */
    void reset(void) override;

  private:
    DetectorContainer*                       fDetectorContainer;
    std::map<uint8_t, DetectorDataContainer> fErrorRate;
    std::map<uint8_t, DetectorDataContainer> fTestedBits;
};
#endif
