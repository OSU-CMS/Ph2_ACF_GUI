/*!
 *
 * \file AntennaTester.h
 * \brief Short Finder  class - converted into a tool based on T.Gadesk's algorithm (AntennaScan, TestChannels, and
 * SaveTestingResults in HybridTester.cc) \author Sarah SEIF EL NASR_STOREY \date 20 / 10 / 16
 *
 * \Support : sarah.storey@cern.ch
 *
 */

#ifndef AntennaTester_h__
#define AntennaTester_h__

#ifdef __USE_ROOT__

#include "Channel.h"
#include "HWInterface/BeBoardFWInterface.h"
#include "Utils/CommonVisitors.h"
#include "Utils/Utilities.h"
#include "Utils/Visitor.h"
#include "tools/Tool.h"

#ifdef __ANTENNA__
#include "Antenna.h"
#endif

#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TH2.h"
#include "TMath.h"
#include "TProfile.h"
#include "TRandom.h"
#include "TString.h"
#include "TStyle.h"
#include "TText.h"
#include <map>

using namespace Ph2_System;

class AntennaTester : public Tool
{
  public:
    AntennaTester();

    // D'tor
    ~AntennaTester();

    void Initialize();
    void EnableAntenna(bool pAntennaEnable, uint8_t pDigiPotentiometer);

    void Measure(uint8_t pDigiPotentiometer);
    void SetDecisionThreshold(double pDecisionThreshold) { fDecisionThreshold = pDecisionThreshold; };

    // function to reconfigure the CBC registers
    // if pDirectoryName == "" then the values from the CBC calibration found in the Results directory (fDirectoryName)
    // are used
    void ReconfigureCBCRegisters(std::string pDirectoryName = "");
    // configure only the Vcth value
    void ConfigureVcth(uint16_t pVcth = 0x0078)
    {
        ThresholdVisitor cWriter(fReadoutChipInterface, pVcth);
        accept(cWriter);
    };

    /*!
     *\brief return mean occupancy for (TOP) pads
     */
    double GetMeanOccupancyTop() { return fHistTop->Integral() / (double)(fNCbc * 127); };
    /*!
     *\brief return mean occupancy for (BOTTOM) pads
     */
    double GetMeanOccupancyBottom() { return fHistBottom->Integral() / (double)(fNCbc * 127); };

    void writeObjects();

  private:
    // initializing functions/methods
    void InitialiseSettings();
    void InitializeHists();

    // histogram drawing functions
    void UpdateHists();
    void UpdateHistsMerged();

    /*!
     * \brief private method that checks channels malfunction based on occupancy histograms, produces output report in
     * .txt format
     */
    void TestChannels();

    // Counters
    uint32_t fTotalEvents;
    uint32_t fNCbc;
    // booleans
    bool     fHoleMode;
    uint32_t trigSource;

    /*!< Decision Threshold for channels occupancy based tests, values from 1 to 100 as % */
    double fDecisionThreshold;

    // Canvases
    TCanvas* fDataCanvas; /*!<Canvas to output single-strip efficiency */

    // Histograms
    TH1F* fHistTop;          /*!< Histogram for top pads */
    TH1F* fHistBottom;       /*!< Histogram for bottom pads */
    TH1F* fHistTopMerged;    /*!< Histogram for top pads used for segmented antenna testing routine*/
    TH1F* fHistBottomMerged; /*!< Histogram for bottom pads used for segmented antenna testing routine*/
};
#endif
#endif
