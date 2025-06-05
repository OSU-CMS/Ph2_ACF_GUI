/*!
        \file                CBCHistogramPulseShape.h
        \brief               DQM class for Calibration example -> use it as a templare
        \author              Fabio Ravera
        \date                17/1/20
        Support :            mail to : fabio.ravera@cern.ch
*/

#ifndef __CBCHistogramPulseShape_H__
#define __CBCHistogramPulseShape_H__
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class CBCHistogramPulseShape
 * \brief Class for CalibrationExample monitoring histograms
 */
class CBCHistogramPulseShape : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    CBCHistogramPulseShape();

    /*!
     * destructor
     */
    ~CBCHistogramPulseShape();

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

    /*!
     * \brief fillCBCPulseShapePlots
     * \brief threshold
     * \brief delay
     * \param theHitContainer : Container with the hits you want to plot
     */
    void fillCBCPulseShapePlots(uint16_t delay, DetectorDataContainer& theOccupancyContainer);
    void fillSCurvePlots(uint16_t vcthr, uint16_t latency, uint16_t delay, DetectorDataContainer& fSCurveOccupancy);

  private:
    DetectorContainer*                                              fDetectorContainer;
    DetectorDataContainer                                           fDetectorChannelPulseShapeHistograms;
    DetectorDataContainer                                           fDetectorChipPulseShapeHistograms;
    std::map<std::tuple<uint16_t, uint16_t>, DetectorDataContainer> fDetectorSCurveHistogramMap;
    float                                                           fInitialVcth{0};
    float                                                           fInitialLatency{0};
    float                                                           fFinalVcth{0};
    float                                                           fVcthStep{0};
    float                                                           fInitialDelay{0};
    float                                                           fFinalDelay{0};
    float                                                           fDelayStep{0};
    float                                                           fEffectiveFinalDelay{0};
    bool                                                            fPlotPulseShapeSCurves{false};
};
#endif
