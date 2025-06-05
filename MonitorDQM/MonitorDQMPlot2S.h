/*!
        \file                MonitorDQMPlot2S.h
        \brief               DQM class for DQM example -> use it as a templare
        \author              Fabio Ravera
        \date                25/7/19
        Support :            mail to : fabio.ravera@cern.ch
*/

#ifndef __MonitorDQMPlot2S_H__
#define __MonitorDQMPlot2S_H__
#include "MonitorDQM/MonitorDQMPlotOT.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class MonitorDQMPlot2S
 * \brief Class for DQMExample monitoring Plots
 */
class MonitorDQMPlot2S : public MonitorDQMPlotOT
{
  public:
    /*!
     * constructor
     */
    MonitorDQMPlot2S();

    /*!
     * destructor
     */
    ~MonitorDQMPlot2S();

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
    bool fill(std::string& inputStream) override;

    /*!
     * \brief process : do something with the Plot like colors, fit, drawing canvases, etc
     */
    void process() override;

    /*!
     * \brief Reset Plot
     */
    void reset(void) override;

    void fillCBCRegisterPlots(DetectorDataContainer& theInputContainer, const std::string& registerName);

  private:
    void                                         bookCBCPlots(TFile* theOutputFile, std::string registerName);
    std::map<std::string, DetectorDataContainer> fCBCRegisterMonitorPlotMap;
};
#endif
