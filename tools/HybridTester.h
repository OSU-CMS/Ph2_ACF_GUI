/*!

        \file                   HybridTester.h
        \brief                 class for validating x*CBC2 hybrids
        \author              Alireza KOKABI & Georg AUZINGER
        \version                1.0
        \date                   24/10/14
        Support :               mail to : georg.auzinger@cern.ch

 */

#ifndef HybridTester_h__
#define HybridTester_h__

#include "tools/Tool.h"
#ifdef __USE_ROOT__

#include "Utils/CommonVisitors.h"
#include "Utils/Utilities.h"
#include "Utils/Visitor.h"
#ifdef __ANTENNA__
#include "Antenna.h"
#endif

#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TH1F.h"
#include "TLine.h"
#include "TStyle.h"

#define CH_DIAGNOSIS_DECISION_TH 80 // decision threshold value for channels diagnosis, expressed in % from 0 to 100

#ifndef __ANTENNA__
namespace patch
{
template <typename T>
std::string to_string(const T& n)
{
    std::ostringstream stm;
    stm << n;
    return stm.str();
}
} // namespace patch
#endif

using namespace Ph2_System;

/*!
 * \class HybridTester
 * \brief Class to test x*CBC2 Hybrids
 */
class HybridTester : public Tool
{
  public:
    // Default C'tor
    HybridTester();
    // Default D'tor
    ~HybridTester();
    /*!
     * \brief Initialize the Histograms and Canvasses for CMD line applications
     * \param pThresholdScan :  bool flag to initialize the additional canvas for the Threshold scan
     */
    void Initialize(bool pThresholdScan = false);
    /*!
     * \brief Test CBC registers by writing complimentary bit patterns (0x55, 0xAA)
     */
    void TestRegisters();
    /*!
     * \brief Test CBC registers by writing complimentary bit patterns (0x55, 0xAA)
     */
    void FindShorts();
    /*!
     * \brief Scan the thresholds to identify the threshold with no noise occupancy
     */
    void ScanThresholds();
    void ScanThreshold();
    /*!
     * \brief Measure the single strip efficiency
     */
    void Measure();
    // scan latency
    void ScanLatency();

    /*!
     *\brief return mean occupancy for (TOP) pads
     */
    double GetMeanOccupancyTop() { return fHistOccupancyTop->GetMean(); };
    /*!
     *\brief return mean occupancy for (BOTTOM) pads
     */
    double GetMeanOccupancyBottom() { return fHistOccupancyBottom->GetMean(); };

    /*!
     *\brief return RMS occupancy for (TOP) pads
     */
    double GetRMSOccupancyTop() { return fHistOccupancyTop->GetRMS(); };
    /*!
     *\brief return RMS occupancy for (BOTTOM) pads
     */
    double GetRMSOccupancyBottom() { return fHistOccupancyBottom->GetRMS(); };

    /*!
     * \brief Measure the single strip efficiency
     */
    void AntennaScan(uint8_t pDigiPotentiometer);
    /*!
     * \brief Save the results of channels testing performed with antenna scan
     */
    void SaveTestingResults(std::string pHybridId);
    /*!
     * \brief Save the results to the file created with SystemController::InitializeResultFile
     */
    void writeObjects();
    void ReconfigureCBCRegisters(std::string pDirectoryName = "");
    /*!
     * \brief re-configure only the Vcth value on the CBCs
     */
    void ConfigureVcth(uint16_t pVcth = 0x0078)
    {
        ThresholdVisitor cWriter(fReadoutChipInterface, pVcth);
        accept(cWriter);
    };

    void DisplayNoisyChannels(std::ostream& os = std::cout);
    void DisplayDeadChannels(std::ostream& os = std::cout);

  private:
    uint32_t fNCbc;             /*!< Number of CBCs in the Setup */
    TCanvas* fDataCanvas;       /*!<Canvas to output single-strip efficiency */
    TH1F*    fHistTop;          /*!< Histogram for top pads */
    TH1F*    fHistBottom;       /*!< Histogram for bottom pads */
    TH1F*    fHistTopMerged;    /*!< Histogram for top pads used for segmented antenna testing routine*/
    TH1F*    fHistBottomMerged; /*!< Histogram for bottom pads used for segmented antenna testing routine*/

    TCanvas* fSummaryCanvas;       /*!<Canvas to output single-strip efficiency */
    TH1F*    fHistOccupancyBottom; /*!< Distibution of measured occupancies for bottom pads */
    TH1F*    fHistOccupancyTop;    /*!< Distibution of measured occupancies for top pads */

    bool     fThresholdScan; /*!< Flag for SCurve Canvas */
    TCanvas* fSCurveCanvas;  /*!< Canvas for threshold scan */

    std::map<ChipContainer*, TH1F*> fSCurveMap; /*!< Histograms for SCurve */
    std::map<ChipContainer*, TF1*>  fFitMap;    /*!< fits for SCurve*/

    // noisy and dead channels on top/bottom sensors
    std::vector<int> fNoisyChannelsTop;
    std::vector<int> fNoisyChannelsBottom;
    std::vector<int> fDeadChannelsTop;
    std::vector<int> fDeadChannelsBottom;

    uint32_t fTotalEvents;
    bool     fHoleMode;
    int      fSigmas;
    uint8_t  fVcth;
    uint8_t  fTestPulseAmplitude;
    double   fDecisionThreshold = 10.0; /*!< Decision Threshold for channels occupancy based tests, values from 1 to 100 as % */
    uint32_t trigSource;

    void                            SetBeBoardForShortsFinding(Ph2_HwDescription::BeBoard* pBoard);
    void                            SetTestGroup(Ph2_HwDescription::BeBoard* pBoard, uint8_t pTestGroup);
    void                            ReconstructShorts(std::array<std::vector<std::array<int, 5>>, 8> pShortedGroupsArray);
    void                            DisplayGroupsContent(std::array<std::vector<std::array<int, 5>>, 8> pShortedGroupsArray);
    bool                            CheckShortsConnection(std::vector<std::array<int, 2>> pShortA, std::vector<std::array<int, 2>> pShortB);
    bool                            CheckChannelInShortPresence(std::array<int, 2> pShortedChannel, std::vector<std::array<int, 2>> pShort);
    std::vector<std::array<int, 2>> MergeShorts(std::vector<std::array<int, 2>> pShortA, std::vector<std::array<int, 2>> pShortB);

    /*!
     * \brief private method that classifies the channels on the top/bottom sensors into "Noisy/Dead"
     */
    void ClassifyChannels(double pNoiseLevel = 65, double pDeadLevel = 25);

    // double fChannelDiagnosisThreshold;
    /*!
     * \brief private method that calls the constructors for the histograms
     */
    void InitializeHists();
    /*!
     * \brief private method that calls reads the settings from the settings map in private member variables
     */
    void InitialiseSettings();
    /*!
     * \brief private method to periodically update the output graphs
     */
    void UpdateHists()
    {
        fDataCanvas->cd(1);
        fHistTop->Draw();
        fDataCanvas->cd(2);
        fHistBottom->Draw();
        fDataCanvas->Update();

        fSummaryCanvas->cd(1);
        fHistOccupancyTop->Draw();
        fSummaryCanvas->cd(2);
        fHistOccupancyBottom->Draw();
        fSummaryCanvas->Update();
        this->HttpServerProcess();
    }

    /*!
     * \brief private method to update histograms after antenna scan was completed
     */
    void UpdateHistsMerged()
    {
        fDataCanvas->cd(1);
        fHistTopMerged->Draw();
        fDataCanvas->cd(2);
        fHistBottomMerged->Draw();
        fDataCanvas->Update();
        this->HttpServerProcess();
    }

    // To measure the occupancy per Chip
    uint32_t fillSCurves(Ph2_HwDescription::BeBoard* pBoard, Ph2_HwInterface::Event* pEvent, uint16_t pValue);
    void     updateSCurveCanvas(Ph2_HwDescription::BeBoard* pBoard);
    void     processSCurves(uint32_t pEventsperVcth);

    // Helper Functions to create the test report!
    std::string DecimalToBinaryString(int a)
    {
        std::string binary = "";
        int         mask   = 1;

        for(int i = 0; i < 31; i++)
        {
            if((mask & a) >= 1)
                binary = "1" + binary;
            else
                binary = "0" + binary;

            mask <<= 1;
        }

        return binary;
    }

    std::string CharToBinaryString(int a)
    {
        std::string binary = "";
        int         mask   = 1;

        for(int i = 0; i < 7; i++)
        {
            if((mask & a) >= 1)
                binary = "1" + binary;
            else
                binary = "0" + binary;

            mask <<= 1;
        }

        return binary;
    }

    void print_buffer(char* buf, int8_t buf_size)
    {
        std::stringstream ss;

        for(uint8_t i = 0; i < buf_size; i++) ss << buf[i] + 0 << " ";

        ss << std::endl;
        LOG(INFO) << ss.str();
    }

    std::string int_vector_to_string(std::vector<int> int_vector)
    {
        std::string output_string = "";

        for(std::vector<int>::iterator it = int_vector.begin(); it != int_vector.end(); ++it) output_string += patch::to_string(*it) + "; ";

        return output_string;
    }

    std::string double_vector_to_string(std::vector<double> int_vector)
    {
        std::string output_string = "";

        for(std::vector<double>::iterator it = int_vector.begin(); it != int_vector.end(); ++it) output_string += patch::to_string(*it) + "; ";

        return output_string;
    }
};

#endif
#endif
