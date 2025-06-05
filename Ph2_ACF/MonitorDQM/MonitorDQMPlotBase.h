/*!
  \file                  MonitorDQMPlotBase.h
  \brief                 base class to create and fill monitoring histograms
  \author                Fabio Ravera, Lorenzo Uplegger
  \version               1.0
  \date                  6/5/19
  Support:               email to fabio.ravera@cern.ch
*/

#ifndef __MonitorDQMPlotBASE_H__
#define __MonitorDQMPlotBASE_H__

#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

class DetectorDataContainer;
class DetectorContainer;
class TFile;
template <typename T>
class GraphContainer;
class TGraph;
struct DetectorMonitorConfig;

/*!
 * \class MonitorDQMPlotBase
 * \brief Base class for monitoring histograms
 */
class MonitorDQMPlotBase
{
  public:
    /*!
     * constructor
     */
    MonitorDQMPlotBase() { ; }

    /*!
     * destructor
     */
    virtual ~MonitorDQMPlotBase() {}

    /*!
     * \brief Book histograms
     * \param theDetectorStructure : Container of the Detector structure
     */
    virtual void book(TFile* outputFile, DetectorContainer& theDetectorStructure, const DetectorMonitorConfig& detectorMonitorConfig) = 0;

    /*!
     * \brief Book histograms
     * \param configurationFileName : xml configuration file
     */
    virtual bool fill(std::string& inputStream) = 0;

    /*!
     * \brief SAve histograms
     * \param outFile : ouput file name
     */
    virtual void process() = 0;

    /*!
     * \brief Book histograms
     * \param configurationFileName : xml configuration file
     */
    virtual void reset(void) = 0;

  protected:
    uint32_t getTimeStampForRoot(std::string rawTime);

    void bookImplementer(TFile*                   theOutputFile,
                         const DetectorContainer& theDetectorStructure,
                         DetectorDataContainer&   dataContainer,
                         GraphContainer<TGraph>&  graphContainer,
                         const std::string&       type,
                         const char*              XTitle = nullptr,
                         const char*              YTitle = nullptr);
};

#endif
