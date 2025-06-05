/*!
        \file                DQMHistogramPhaseScan.h
        \brief               base class to create and fill monitoring histograms
        \author              Fabio Ravera, Lorenzo Uplegger
        \version             1.0
        \date                6/5/19
        Support :            mail to : fabio.ravera@cern.ch
*/

#ifndef __DQMHISTOGRAMPHASESCAN_H__
#define __DQMHISTOGRAMPHASESCAN_H__
#include "../DQMUtils/DQMHistogramBase.h"
#include "../Utils/Container.h"
#include "../Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramPhaseScan
 * \brief Class for PedeNoise monitoring histograms
 */
class DQMHistogramPhaseScan : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramPhaseScan();

    /*!
     * destructor
     */
    ~DQMHistogramPhaseScan();

    /*!
     * Book histograms
     */
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap) override;

    /*!
     * Fill histogram
     */
    bool fill(std::string& inputStream) override;

    /*!
     * Save histogram
     */
    void process() override;

    /*!
     * Reset histogram
     */
    void reset(void) override;
    // virtual void summarizeHistos();

    // Histogram Fillers
    void fillPhasePlots(uint16_t pPhase, uint16_t pLatency, DetectorDataContainer& pOccupancy);

  private:
    void parseSettings(const Ph2_Parser::SettingsMap& pSettingsMap);

    DetectorDataContainer fDetectorData;

    DetectorDataContainer fPhaseHistograms;
    DetectorDataContainer fLatencyVsPhaseHistograms;
    DetectorDataContainer fTDCVsPhaseHistograms;

    uint32_t fStartPhase;
    uint32_t fPhaseRange;
    uint32_t fStartLatency;
    uint32_t fLatencyRange;
};
#endif
