/*!
        \file                DQMHistogramOTBitErrorRateTest.h
        \brief               DQM class for OTBitErrorRateTest
        \author              Fabio Ravera
        \date                02/07/24
*/

#ifndef DQMHistogramOTBitErrorRateTest_h_
#define DQMHistogramOTBitErrorRateTest_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTBitErrorRateTest
 * \brief Class for OTBitErrorRateTest monitoring histograms
 */
class DQMHistogramOTBitErrorRateTest : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTBitErrorRateTest();

    /*!
     * destructor
     */
    ~DQMHistogramOTBitErrorRateTest();

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

    void fillErrorCounterPhaseScan(DetectorDataContainer& theErrorCountainer, uint16_t phaseDelay, uint8_t line);

    void fillErrorCounter(DetectorDataContainer& theErrorCountainer, uint8_t line);

    void fillFECcounter(DetectorDataContainer& theFECContainer, uint8_t line);

    void fillBERTbestPhase(DetectorDataContainer& BestPhaseContainer, uint8_t line);

  private:
    DetectorContainer*    fDetectorContainer;
    DetectorDataContainer fBERTerrorRatePhaseScanHistogram;
    DetectorDataContainer fBERTbitCounterPhaseScanHistogram;
    DetectorDataContainer fBestPhaseHistogram;
    DetectorDataContainer fBERTerrorRateHistogram;
    DetectorDataContainer fBERTbitCounterHistogram;
    DetectorDataContainer fFECcounterHistogram;
    uint8_t               fNumberOfLines;
};
#endif
