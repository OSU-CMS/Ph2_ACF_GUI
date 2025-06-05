/*!
        \file                MonitorDQMPlotOT.h
        \brief               DQM class for DQM example -> use it as a templare
        \author              Fabio Ravera
        \date                29/7/24
        Support :            mail to : fabio.ravera@cern.ch
*/

#ifndef __MonitorDQMPlotOT_H__
#define __MonitorDQMPlotOT_H__
#include "MonitorDQM/MonitorDQMPlotBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;
enum class FrontEndType;

/*!
 * \class MonitorDQMPlotOT
 * \brief Class for DQMExample monitoring Plots
 */
class MonitorDQMPlotOT : public MonitorDQMPlotBase
{
  public:
    /*!
     * constructor
     */
    MonitorDQMPlotOT();

    /*!
     * destructor
     */
    ~MonitorDQMPlotOT();

    /*!
     * \brief Book Plots
     * \param theOutputFile : where Plots will be saved
     * \param theDetectorStructure : Detector container as obtained after file parsing, used to create Plots for
     * all board/chip/hybrid/channel \param pSettingsMap : setting as for Tool setting map in case coe informations are
     * needed (i.e. FitSCurve)
     */
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const DetectorMonitorConfig& detectorMonitorConfig) override;

    /*!
     * \brief fill : fill Plots from TCP stream, need to be overwritten to avoid compilation errors, but it is not
     * needed if you do not fo into the SoC \param dataBuffer : vector of char with the TCP datastream
     */
    virtual bool fill(std::string& inputStream) override;

    /*!
     * \brief process : do something with the Plot like colors, fit, drawing canvases, etc
     */
    virtual void process() override;

    /*!
     * \brief Reset Plot
     */
    virtual void reset(void) override;

    void fillLpGBTmonitorPlots(DetectorDataContainer& theInputContainer, const std::string& registerName);

  protected:
    const DetectorContainer* fDetectorContainer;
    GraphContainer<TGraph>   producePlotTemplate(const std::string& registerName, const std::string& chipName, std::string yAxisUnits = "");
    void                     fillReadoutChipPlots(DetectorDataContainer&                              theInputContainer,
                                                  const std::string&                                  monitorValueName,
                                                  const std::map<std::string, DetectorDataContainer>& theValueMonitorPlotMap,
                                                  FrontEndType                                        theFrontEndType);

  private:
    void                                         bookLpGBTPlots(TFile* theOutputFile, const std::string& registerName);
    std::map<std::string, DetectorDataContainer> fLpGBTRegisterMonitorPlotMap;
};
#endif
