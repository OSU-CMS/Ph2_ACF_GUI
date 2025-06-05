/*!
        \file                DQMHistogramPedestalEqualization.h
        \brief               base class to create and fill monitoring histograms
        \author              Fabio Ravera, Lorenzo Uplegger
        \version             1.0
        \date                6/5/19
        Support :            mail to : fabio.ravera@cern.ch
*/

#ifndef __DQMHISTOGRAMCALIBRATION_H__
#define __DQMHISTOGRAMCALIBRATION_H__
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramPedestalEqualization
 * \brief Class for PedeNoise monitoring histograms
 */
class DQMHistogramPedestalEqualization : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramPedestalEqualization();

    /*!
     * destructor
     */
    ~DQMHistogramPedestalEqualization();

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

    // void fillVplusPlots(DetectorDataContainer& theVthr);

    void fillOffsetPlots(DetectorDataContainer& theOffsets);

    void fillOccupancyPlots(DetectorDataContainer& theOccupancy);

  private:
    DetectorContainer* fDetectorContainer;
    // DetectorDataContainer fDetectorVplusHistograms;
    DetectorDataContainer fDetectorOffsetHistograms;
    DetectorDataContainer fDetectorOccupancyHistograms;

    DetectorDataContainer fDetectorPedestalHistograms;

    template <typename T>
    void processOccupancy(ChipDataContainer* theChipDataContainer)
    {
        T* offsetHistogram = theChipDataContainer->getSummary<HistContainer<T>>().fTheHistogram;
        offsetHistogram->DrawCopy();
    }
};
#endif
