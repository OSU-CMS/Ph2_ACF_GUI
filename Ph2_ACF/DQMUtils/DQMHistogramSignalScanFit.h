/*!
        \file                DQMHistogramSignalScanFit.h
        \brief               base class to create and fill monitoring histograms
        \author              Fabio Ravera, Lorenzo Uplegger
        \version             1.0
        \date                6/5/19
        Support :            mail to : fabio.ravera@cern.ch
*/

#ifndef __DQMHISTOGRAMSIGNALSCANFIT_H__
#define __DQMHISTOGRAMSIGNALSCANFIT_H__
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramSignalScanFit
 * \brief Class for PedeNoise monitoring histograms
 */
class DQMHistogramSignalScanFit : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramSignalScanFit();

    /*!
     * destructor
     */
    ~DQMHistogramSignalScanFit();

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

  private:
    DetectorDataContainer fDetectorData;
};
#endif
