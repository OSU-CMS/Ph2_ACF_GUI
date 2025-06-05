/*!
        \file                DQMHistogramPSCounterTest.h
        \brief               DQM class for PSCounterTest
        \author              Adam Kobert
        \date                25/07/24
*/

#ifndef DQMHistogramPSCounterTest_h_
#define DQMHistogramPSCounterTest_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramPSCounterTest
 * \brief Class for PSCounterTest monitoring histograms
 */
class DQMHistogramPSCounterTest : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramPSCounterTest();

    /*!
     * destructor
     */
    ~DQMHistogramPSCounterTest();

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

    void fillSCurvePlots(uint16_t stripThreshold, uint16_t pixelThreshold, DetectorDataContainer& theOccupancyContainer);

  private:
    DetectorContainer*    fDetectorContainer;
    DetectorDataContainer fSCurveContainer;
};
#endif
