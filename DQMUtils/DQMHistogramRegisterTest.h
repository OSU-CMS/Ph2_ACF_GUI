/*!
        \file                DQMHistogramLatencyScan.h
        \brief               base class to create and fill monitoring histograms
        \author              Fabio Ravera, Lorenzo Uplegger
        \version             1.0
        \date                6/5/19
        Support :            mail to : fabio.ravera@cern.ch
*/

#ifndef __DQMHISTOGRAMREGISTERTEST_H__
#define __DQMHISTOGRAMREGISTERTEST_H__
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramLatencyScan
 * \brief Class for PedeNoise monitoring histograms
 */
class DQMHistogramRegisterTest : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramRegisterTest();

    /*!
     * destructor
     */
    ~DQMHistogramRegisterTest();

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
    void fillRegisterWriteMismatches(DetectorDataContainer& pWrites, DetectorDataContainer& pToggles, DetectorDataContainer& pVals);
    void fillRegisterReadMismatches(DetectorDataContainer& pReads, DetectorDataContainer& pToggles, DetectorDataContainer& pVals);
    void fillRegisterReadCounts(DetectorDataContainer& pReads, DetectorDataContainer& pToggles);
    void fillRegisterWriteCounts(DetectorDataContainer& pWrites, DetectorDataContainer& pReads, DetectorDataContainer& pToggles);

  private:
    void parseSettings(const Ph2_Parser::SettingsMap& pSettingsMap);

    DetectorDataContainer fDetectorData;
    DetectorDataContainer fWriteMismatchesPg0, fWriteMismatchesPg1;
    DetectorDataContainer fReadMismatchesPg0, fReadMismatchesPg1;
    DetectorDataContainer fRdValsMismatchesPg1, fWrValsMismatchesPg1;
    DetectorDataContainer fRdCnts, fWrCnts;

    DetectorDataContainer fRdMismatchesPg1, fWrMismatchesPg1;
};
#endif
