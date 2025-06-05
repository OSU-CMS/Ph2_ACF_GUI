/*!
        \file                DQMHistogramOTSSAtoMPAecv.h
        \brief               DQM class for OTSSAtoMPAecv
        \author              Fabio Ravera
        \date                11/06/24
*/

#ifndef DQMHistogramOTSSAtoMPAecv_h_
#define DQMHistogramOTSSAtoMPAecv_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTSSAtoMPAecv
 * \brief Class for OTSSAtoMPAecv monitoring histograms
 */
class DQMHistogramOTSSAtoMPAecv : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTSSAtoMPAecv();

    /*!
     * destructor
     */
    ~DQMHistogramOTSSAtoMPAecv();

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

    void fillPatternEfficiencyScan(DetectorDataContainer& thePatternMatchingEfficiency, uint8_t clockEdge, uint8_t slvsCurrent, int samplingPhaseOffset);

  private:
    DetectorContainer*                       fDetectorContainer;
    std::map<uint8_t, DetectorDataContainer> fPhaseScanTestedBits;
    std::map<uint8_t, DetectorDataContainer> fPhaseScanErrorRate;

    int fMinimum320PhaseShift = -1;
    int fMaximum320PhaseShift = +1;
};
#endif
