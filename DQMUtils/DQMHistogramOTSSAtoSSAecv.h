/*!
        \file                DQMHistogramOTSSAtoSSAecv.h
        \brief               DQM class for OTSSAtoSSAecv
        \author              Fabio Ravera
        \date                26/06/24
*/

#ifndef DQMHistogramOTSSAtoSSAecv_h_
#define DQMHistogramOTSSAtoSSAecv_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTSSAtoSSAecv
 * \brief Class for OTSSAtoSSAecv monitoring histograms
 */
class DQMHistogramOTSSAtoSSAecv : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTSSAtoSSAecv();

    /*!
     * destructor
     */
    ~DQMHistogramOTSSAtoSSAecv();

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

    void fillStubPatternEfficiencyScan(DetectorDataContainer& thePatternMatchingEfficiency, uint8_t clockEdge, uint8_t slvsCurrent, int samplingPhaseOffset);

  private:
    std::pair<int, int>                      getMPArange(uint8_t stubPattern) const;
    DetectorContainer*                       fDetectorContainer;
    std::map<uint8_t, DetectorDataContainer> fPhaseScanTestedBits;
    std::map<uint8_t, DetectorDataContainer> fPhaseScanErrorRate;

    int fMinimum320PhaseShift = -1;
    int fMaximum320PhaseShift = +1;
};
#endif
