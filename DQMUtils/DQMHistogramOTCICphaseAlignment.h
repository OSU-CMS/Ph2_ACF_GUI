/*!
        \file                DQMHistogramOTCICphaseAlignment.h
        \brief               DQM class for OTCICphaseAlignment
        \author              Fabio Ravera
        \date                07/02/24
*/

#ifndef DQMHistogramOTCICphaseAlignment_h_
#define DQMHistogramOTCICphaseAlignment_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTCICphaseAlignment
 * \brief Class for OTCICphaseAlignment monitoring histograms
 */
class DQMHistogramOTCICphaseAlignment : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTCICphaseAlignment();

    /*!
     * destructor
     */
    ~DQMHistogramOTCICphaseAlignment();

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

    void fillPhaseHistogramResults(DetectorDataContainer& thePhaseHistogramResultContainer);
    void fillBestPhaseResults(DetectorDataContainer& thePhaseAlignmentResultContainer);
    void fillLockingEfficiencyResults(DetectorDataContainer& theLockingEfficiencyContainer);

  private:
    DetectorContainer*    fDetectorContainer;
    DetectorDataContainer fPhaseHistogramContainer;
    DetectorDataContainer fBestPhaseHistogramContainer;
    DetectorDataContainer fLockingEfficiencyHistogramContainer;
};
#endif
