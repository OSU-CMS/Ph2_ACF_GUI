/*!
        \file                DQMHistogramOTverifyBoardDataWord.h
        \brief               DQM class for OTverifyBoardDataWord
        \author              Fabio Ravera
        \date                01/02/24
*/

#ifndef DQMHistogramOTverifyBoardDataWord_h_
#define DQMHistogramOTverifyBoardDataWord_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTverifyBoardDataWord
 * \brief Class for OTverifyBoardDataWord monitoring histograms
 */
class DQMHistogramOTverifyBoardDataWord : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTverifyBoardDataWord();

    /*!
     * destructor
     */
    ~DQMHistogramOTverifyBoardDataWord();

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

    void fillPatternErrorRate(DetectorDataContainer& thePatternMatchingEfficiencyContainer);

  private:
    DetectorContainer*    fDetectorContainer;
    DetectorDataContainer fMatchingErrorRateHistogramContainer;
    DetectorDataContainer fMatchingTestedBitsHistogramContainer;
};
#endif
