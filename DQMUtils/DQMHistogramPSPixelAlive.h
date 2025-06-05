/*!
        \file                DQMHistogramPSPixelAlive.h
        \brief               base class to create and fill monitoring histograms
        \author              Adam Kobert
        \version             1.0
        \date                8/21/23
        Support :            mail to :
*/

#ifndef __DQMHISTOGRAMPIXELALIVE_H__
#define __DQMHISTOGRAMPIXELALIVE_H__
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramPSPixelAlive
 * \brief Class for PSPixelAlive monitoring histograms
 */
class DQMHistogramPSPixelAlive : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramPSPixelAlive();

    /*!
     * destructor
     */
    ~DQMHistogramPSPixelAlive();

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

    void fillOccupancyPlots(DetectorDataContainer& theOccupancy);

  private:
    uint32_t fNPixelChannels = 0, fNStripChannels = 0;
    bool     fWithCBC = false;
    bool     fWithSSA = false;
    bool     fWithMPA = false;

    DetectorContainer* fDetectorContainer;
    uint32_t           NCH = 0;
    //    DetectorDataContainer fDetectorOccupancyHistograms;
    DetectorDataContainer fDetectorStripOccupancyHistograms;
    DetectorDataContainer fDetectorPixelOccupancyHistograms;
};
#endif
