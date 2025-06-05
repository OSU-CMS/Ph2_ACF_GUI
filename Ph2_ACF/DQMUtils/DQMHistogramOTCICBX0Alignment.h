/*!
        \file                DQMHistogramOTCICBX0Alignment.h
        \brief               DQM class for OTCICBX0Alignment
        \author              Irene Zoi
        \date                22/02/24
*/

#ifndef DQMHistogramOTCICBX0Alignment_h_
#define DQMHistogramOTCICBX0Alignment_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTCICBX0Alignment
 * \brief Class for OTCICBX0Alignment monitoring histograms
 */
class DQMHistogramOTCICBX0Alignment : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTCICBX0Alignment();

    /*!
     * destructor
     */
    ~DQMHistogramOTCICBX0Alignment();

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

    void fillBX0AlignmentDelay(DetectorDataContainer& theBX0AlignmentDelayContainer);
    void fillBX0AlignmentDelayVsRetimePix(DetectorDataContainer& theBX0AlignmentDelayContainer);

  private:
    DetectorContainer*    fDetectorContainer;
    DetectorDataContainer fBX0AlignmentDelayHistogramContainer;
    DetectorDataContainer fBX0AlignmentDelayVsRetimePixHistogramContainer;
};
#endif