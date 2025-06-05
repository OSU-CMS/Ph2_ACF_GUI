/*!
        \file                DQMHistogramOTPSringOscillatorTest.h
        \brief               DQM class for OTPSringOscillatorTest
        \author              Fabio Ravera
        \date                26/07/24
*/

#ifndef DQMHistogramOTPSringOscillatorTest_h_
#define DQMHistogramOTPSringOscillatorTest_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTPSringOscillatorTest
 * \brief Class for OTPSringOscillatorTest monitoring histograms
 */
class DQMHistogramOTPSringOscillatorTest : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTPSringOscillatorTest();

    /*!
     * destructor
     */
    ~DQMHistogramOTPSringOscillatorTest();

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

    void fillMPAringOscillatorInverter(const DetectorDataContainer& theRingOscillatorContainer);

    void fillMPAringOscillatorDelay(const DetectorDataContainer& theRingOscillatorContainer);

    void fillSSAringOscillatorInverter(const DetectorDataContainer& theRingOscillatorContainer);

    void fillSSAringOscillatorDelay(const DetectorDataContainer& theRingOscillatorContainer);

  private:
    void                  fillMPAringOscillator(const DetectorDataContainer& theRingOscillatorContainer, DetectorDataContainer& theMPAringOscillatorContainer);
    void                  fillSSAringOscillator(const DetectorDataContainer& theRingOscillatorContainer, DetectorDataContainer& theSSAringOscillatorContainer);
    DetectorContainer*    fDetectorContainer;
    DetectorDataContainer fMPAringOscillatorInverterContainer;
    DetectorDataContainer fMPAringOscillatorDelayContainer;
    DetectorDataContainer fSSAringOscillatorInverterContainer;
    DetectorDataContainer fSSAringOscillatorDelayContainer;
};
#endif
