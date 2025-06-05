/*!
        \file                DQMHistogramECV.h
        \brief               class to create and fill data from ECV measurements
        \author              Stefan Maier
        \version             1.0
        \date                27/7/22
        Support :            mail to : s.maier@kit.edu
 */

#ifndef __DQMHISTOGRAMECV_H__
#define __DQMHISTOGRAMECV_H__
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramLatencyScan
 * \brief Class for PedeNoise monitoring histograms
 */
class DQMHistogramECV : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramECV();

    /*!
     * destructor
     */
    ~DQMHistogramECV();

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

    void fillWordAlign(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, uint8_t pPhase, uint8_t pHybridId, uint8_t pLine, DetectorDataContainer& pWordAlignSummary);

    void fillBER(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, uint8_t pPhase, uint8_t pHybridId, uint8_t pLine, DetectorDataContainer& pBERSummary);

    void fillPhases(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, uint8_t pHybridId, uint8_t pLine, DetectorDataContainer& pPhase);

    void fillChosenPhase(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, DetectorDataContainer& pPhase);

  private:
    DetectorDataContainer fDetectorData;
    DetectorDataContainer fBitErrorScanPolarity0;
    DetectorDataContainer fBitErrorScanPolarity1;

    DetectorDataContainer fWordAlignmentScanPolarity0;
    DetectorDataContainer fWordAlignmentScanPolarity1;

    DetectorDataContainer fPhaseScanPolarity0;
    DetectorDataContainer fPhaseScanPolarity1;

    DetectorDataContainer fChosenPhasePolarity0;
    DetectorDataContainer fChosenPhasePolarity1;
};
#endif
