/*!
        \file                DQMHistogramOTMeasureOccupancy.h
        \brief               DQM class for OTMeasureOccupancy
        \author              Fabio Ravera
        \date                03/04/24
*/

#ifndef DQMHistogramOTMeasureOccupancy_h_
#define DQMHistogramOTMeasureOccupancy_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTMeasureOccupancy
 * \brief Class for OTMeasureOccupancy monitoring histograms
 */
class DQMHistogramOTMeasureOccupancy : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTMeasureOccupancy();

    /*!
     * destructor
     */
    ~DQMHistogramOTMeasureOccupancy();

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

    void fillOccupancy(const DetectorDataContainer& theOccupancyContainer, size_t iteration);

  protected:
    void               bookPlotsForInjection(TFile* theOutputFile, double theNumberOfEvents, float theCBCtestPulseValue, float theSSAtestPulseValue, float theMPAtestPulseValue, int iteration = 0);
    DetectorContainer* fDetectorContainer;

  private:
    std::map<int, DetectorDataContainer> fOccupancyHistogramContainer;
};
#endif
