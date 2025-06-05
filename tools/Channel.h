/*!

        \file                   Channel.h
        \brief                 class representing a CBC channel for calibration
        \author              Georg AUZINGER
        \version                1.0
        \date                   24/10/14
        Support :               mail to : georg.auzinger@cern.ch

 */

#ifndef _CHANNEL_H__
#define _CHANNEL_H__

#ifdef __USE_ROOT__

#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TGraphErrors.h"
#include "TH1F.h"
#include "THStack.h"
#include "TROOT.h"
#include "TString.h"
#include "Utils/ConsoleColor.h"
#include "Utils/Utilities.h"
#include "Utils/easylogging++.h"
#include <map>
#include <vector>

/*!
 * \class Channel
 * \brief Class representing a CBC channel for calibration
 */
struct Channel
{
    Channel(uint8_t pBeId, uint8_t pHybridId, uint8_t pOpticalGroupId, uint8_t pCbcId, uint8_t pChannelId);
    ~Channel();

    // members
    // Ids
    uint8_t fBeId;           /*!< Back End ID */
    uint8_t fHybridId;       /*!< Front End ID */
    uint8_t fOpticalGroupId; /*!< Optical Group ID */
    uint8_t fCbcId;          /*!< CBC ID*/
    uint8_t fChannelId;      /*!< Channel Number */
    bool    fFitted;         /*!< Flag to select the algroithm*/

    // Settings
    uint8_t fOffset; /*!<  current offset value for this channel; needs to be set manually */

    // Data
    TH1F*   fScurve;     /*!< Histogram to store the SCurve */
    TF1*    fFit;        /*!< Fit for the SCurve */
    TH1F*   fDerivative; /*!< Histogram to hold the derivative of the Scurve*/
    TGraph* fPulse;      /*!< Graph to store the curve for pulseshape measurements*/
    // Methods
    /*!
     * \brief get the SCurve midpoint affter fitting
     * \return the midpoint of the SCurve; the so-called pedestal
     */
    double getPedestal() const;
    /*!
     * \brief get the SCurve width affter fitting
     * \return the width of the SCurve; the so-called noise
     */
    double getNoise() const;
    /*!
     * \brief get the current channel offset
     * \return the current channel offset
     */
    uint8_t getOffset() const { return fOffset; };
    /*!
     * \brief set the Offset of the Channel object: this is not propagated to HW
     * \param pOffset:  set the fOffset member to pOffset
     */
    void setOffset(uint8_t pOffset);
    /*!
     * \brief Initialize the Histogram and Fit for the Channel
     * \param pParameter: the current parameter that is being varied for storing in file
     * \param pValue: the value of pParameter
     */
    void initializeHist(uint16_t pValue, TString pParameter);
    /*!
     *brief  Initialize the TGraph to store the PulseShapeMeasurement
     *param pTitle: the Title for the TGraph
     */
    void initializePulse(TString pTitle);
    /*!
     *\ brief: set a point on the Test Pulse Graph
     *\ param x: xCoordinate
     *\ param y: yCoordinate
     */
    void setPulsePoint(double x, double y) { fPulse->SetPoint(fPulse->GetN(), x, y); }
    /*!
     * \brief fill the histogram
     * \param pVcth: the bin at which to fill the histogram (normally Vcth value)
     */
    void fillHist(uint16_t pVcth);

    /*!
     * \brief fit the SCurve Histogram with the Fit object
     * \param pEventsperVcth: the number of Events taken for each Vcth setting, used to normalize SCurve
     * \param pHole: the CBC mode: electron or hole
     * \param pParameter: the current parameter that is being varied for storing in file
     * \param pValue: the value of pParameter
     *\param pResutlfile: pointer to the ROOT file where the results are supposed to be stored
     */
    void fitHist(uint32_t pEventsperVcth, bool pHole, uint16_t pValue, TString pParameter, TFile* pResultfile);

    /*!
     * \brief differentiate the SCurve Histogram with the Derivative object
     * \param pEventsperVcth: the number of Events taken for each Vcth setting, used to normalize SCurve
     * \param pHole: the CBC mode: electron or hole
     * \param pParameter: the current parameter that is being varied for storing in file
     * \param pValue: the value of pParameter
     *\param pResutlfile: pointer to the ROOT file where the results are supposed to be stored
     */
    void differentiateHist(uint32_t pEventsperVcth, bool pHole, uint16_t pValue, TString pParameter, TFile* pResultfile);

    /*!
     * \brief reset the Histogram and Fit objects
     */
    void resetHist();
};

struct TestGroup
{
    TestGroup(uint8_t pBeId, uint8_t pHybridId, uint8_t pCbcId, uint8_t pGroupId);

    uint8_t fBeId;
    uint8_t fHybridId;
    uint8_t fCbcId;
    uint8_t fGroupId;
};

struct TestGroupGraph
{
    TestGroupGraph();
    TestGroupGraph(uint8_t pBeId, uint8_t pHybridId, uint8_t pCbcId, uint8_t pGroupId);
    void          FillVplusVcthGraph(uint8_t& pVplus, double pPedestal, double pNoise);
    TGraphErrors* fVplusVcthGraph;
};

struct TestGroupComparer
{
    bool operator()(const TestGroup& g1, const TestGroup& g2) const
    {
        if(g1.fBeId == g2.fBeId)
        {
            if(g1.fHybridId == g2.fHybridId)
            {
                if(g1.fCbcId == g2.fCbcId)
                    return g1.fGroupId < g2.fGroupId;
                else
                    return g1.fCbcId < g2.fCbcId;
            }
            else
                return g1.fHybridId < g2.fHybridId;
        }
        else
            return g1.fBeId < g2.fBeId;
    }
};

typedef std::map<TestGroup, std::vector<Channel>, TestGroupComparer> TestGroupMap;
typedef std::map<TestGroup, TestGroupGraph, TestGroupComparer>       TestGroupGraphMap;

#endif
#endif
