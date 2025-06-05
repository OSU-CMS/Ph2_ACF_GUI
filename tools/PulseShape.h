/*!

        \file                  PulseShape.h
        \brief                 class to do reconstruct the pulse shape of the CBC
        \author              Andrea Massetti & Ali Imran
        \version                1.0
        \date                   20/01/15
        Support :               mail to : georg.auzinger@cern.ch

 */

#ifndef PULSESHAPE_H__
#define PULSESHAPE_H__

#include "tools/Tool.h"
#ifdef __USE_ROOT__
#include "Channel.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TMath.h"
#include "TMultiGraph.h"
#include "TString.h"
#include "Utils/CommonVisitors.h"
#include "Utils/Utilities.h"
#include "Utils/Visitor.h"
#include <math.h>

using namespace Ph2_System;

/*!
 * \class PulseShape
 * \brief Class to reconstruct pulse shape
 */
typedef std::map<ChipContainer*, std::vector<Channel*>> ChannelMap;

class PulseShape : public Tool
{
  public:
    PulseShape();
    ~PulseShape();
    /*!
     * \Initialize the istogram
     */
    void Initialize();

    /*!
     * \scan the Vcth with the correspondent delay
     * \param pDelay: initialize the hist whith the pDelay
     */
    void ScanVcth(uint32_t pDelay, int cLow);

    /*!
     * \Scan the test pulse delay
     * \param pStepSize: scan the test pulse delay with steps of : pStepSize
     */
    void ScanTestPulseDelay(uint8_t pStepSize);

  private:
    /*!
     * \brief find the channels of a test group
     * \param pTestGroup: the number of the test group
     * \return the channels in the pTestGroup
     */
    std::vector<uint32_t> findChannelsInTestGroup(uint32_t pTestGroup);

    /*!
     * \brief parse the xml settings
     */
    void parseSettings();

    /*!
     * \brief set the system test pulse
     * \param pTPAmplitude: the amplitude of the test pulse
     */
    void setSystemTestPulse(uint8_t pTPAmplitude);

    /*!
     * \brief update the Histogram
     * \param pHistName: the name of the Hist
     * \param pFinal: true if is the last updateHists to be done
     */
    void updateHists(std::string pHistName, bool pFinal);

    uint32_t fillVcthHist(Ph2_HwDescription::BeBoard* pBoard, Ph2_HwInterface::Event* pEvent, uint32_t pVcth);
    /*!
     * \brief convert the delay before concat to the  test group number
     * \param pDelay: the actual dealy
     */
    void setDelayAndTesGroup(uint32_t pDelay);

    /*!
     * \brief enable the test group
     */
    void toggleTestGroup(bool pEnable);

    /*!
     * \brief fit the graph with the fitting function
     */
    void fitGraph(int pLow);

    ChannelMap            fChannelMap; /*!< Map Chip vs chennels number */
    bool                  fFitHist;
    uint32_t              fNevents;         /*!< Number of events */
    uint32_t              fHoleMode;        /*!< Check if is in hole mode */
    uint32_t              fNCbc;            /*!< Number of CBCs */
    uint8_t               fVplus;           /*!< Postamp  bias voltage */
    uint8_t               fTestGroup;       /*!< Number of the test group */
    uint8_t               fTPAmplitude;     /*!< Test pulse Amplitude */
    uint32_t              fDelayAfterPulse; // Delay after test pulse
    uint32_t              fChannel;         /*!< channel number */
    uint8_t               fOffset;          /*!< Offset value for the channel */
    uint32_t              fStepSize;        /*!< Step size */
    std::vector<uint32_t> fChannelVector;   /*!< Channels in the test group */
};

/*!
 * \fitting function
 * \param x: the amplitude of the fitting function
 * \param par: array with the parameters of the fitting function
 * \return the point of the fitting function
 */
double pulseshape(double* x, double* par);
double pulseshape2(double* x, double* par);

#endif
#endif
