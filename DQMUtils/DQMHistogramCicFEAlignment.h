/*!
        \file                DQMHistogramLatencyScan.h
        \brief               base class to create and fill monitoring histograms
        \author              Fabio Ravera, Lorenzo Uplegger
        \version             1.0
        \date                6/5/19
        Support :            mail to : fabio.ravera@cern.ch
*/

#ifndef __DQMHistogramCicFEAlignment_H__
#define __DQMHistogramCicFEAlignment_H__
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramLatencyScan
 * \brief Class for PedeNoise monitoring histograms
 */
class DQMHistogramCicFEAlignment : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramCicFEAlignment();

    /*!
     * destructor
     */
    ~DQMHistogramCicFEAlignment();

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
    void fillManualPhaseScan(uint8_t pPhase, uint8_t pLine, DetectorDataContainer& pErrors, DetectorDataContainer& pData);

  private:
    void parseSettings(const Ph2_Parser::SettingsMap& pSettingsMap);

    DetectorDataContainer fDetectorData;
    DetectorDataContainer fManualPhaseScan;
};
#endif
