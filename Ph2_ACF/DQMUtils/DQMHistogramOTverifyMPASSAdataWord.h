/*!
        \file                DQMHistogramOTverifyMPASSAdataWord.h
        \brief               DQM class for OTverifyMPASSAdataWord
        \author              Fabio Ravera
        \date                07/03/24
*/

#ifndef DQMHistogramOTverifyMPASSAdataWord_h_
#define DQMHistogramOTverifyMPASSAdataWord_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTverifyMPASSAdataWord
 * \brief Class for OTverifyMPASSAdataWord monitoring histograms
 */
class DQMHistogramOTverifyMPASSAdataWord : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTverifyMPASSAdataWord();

    /*!
     * destructor
     */
    ~DQMHistogramOTverifyMPASSAdataWord();

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

    void fillPatternMatchingEfficiencyResults(DetectorDataContainer& thePatternMatchingEfficiencyContainer);

  private:
    DetectorContainer*    fDetectorContainer;
    DetectorDataContainer fPatternMatchingErrorRateHistogramContainer;
    DetectorDataContainer fPatternMatchingTestedBitsHistogramContainer;
};
#endif
