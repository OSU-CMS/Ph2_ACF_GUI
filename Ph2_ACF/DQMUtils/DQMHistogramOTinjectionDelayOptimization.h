/*!
        \file                DQMHistogramOTinjectionDelayOptimization.h
        \brief               DQM class for OTinjectionDelayOptimization
        \author              Fabio Ravera
        \date                15/03/24
*/

#ifndef DQMHistogramOTinjectionDelayOptimization_h_
#define DQMHistogramOTinjectionDelayOptimization_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTinjectionDelayOptimization
 * \brief Class for OTinjectionDelayOptimization monitoring histograms
 */
class DQMHistogramOTinjectionDelayOptimization : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTinjectionDelayOptimization();

    /*!
     * destructor
     */
    ~DQMHistogramOTinjectionDelayOptimization();

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

    void fillThresholdVsDelayScan(uint16_t delay, DetectorDataContainer& theThresholdContainer);
    void fillBestThresholdAndDelay(DetectorDataContainer& theBestThresholdAndDelayContainer);

  private:
    DetectorContainer*    fDetectorContainer;
    DetectorDataContainer fThresholdVsDelayScanHistogramContainer;
    DetectorDataContainer fBestThresholdAndDelayHistogramContainer;
    uint16_t              fDelayStep{1};
};
#endif
