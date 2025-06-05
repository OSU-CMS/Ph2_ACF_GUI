/*!

        \file                   BiasSweep.h
        \brief                  Class to sweep one of the CBC2/3 biases and perform an analog measurement via the
   AnalogMux and Ke2110 DMM \author                 Georg AUZINGER \version                1.0 \date 31/10/16 Support :
   mail to : georg.auzinger@SPAMNOT.cern.ch

 */

#ifndef __STUBSWEEP_H__
#define __STUBWEEP_H__
#include "tools/Tool.h"
#ifdef __USE_ROOT__

#include <map>
#include <vector>

#include "TAxis.h"
#include "TGraph.h"
#include "TObject.h"
#include "TProfile.h"
#include "TString.h"
#include "TTree.h"
#include "Utils/CommonVisitors.h"

using namespace Ph2_System;

class StubSweep : public Tool
{
  public:
    StubSweep();
    ~StubSweep();

    void Initialize();

    // added for debugging - S.
    // ******
    // *******
    void SweepStubs(uint32_t pNEvents = 1);

  private:
    TCanvas* fSweepCanvas;

    // settings
    uint8_t fDelay;
    uint8_t fReadBackAttempts;

    // methods to fill/update histograms
    void updateHists(std::string pHistname);
    void fillStubBendHist(Ph2_HwDescription::ReadoutChip* pCbc, std::vector<uint8_t> pChannelPair, uint8_t pStubBend);
    void fillStubSweepHist(Ph2_HwDescription::ReadoutChip* pCbc, std::vector<uint8_t> pChannelPair, uint8_t pStubPosition);

    // method to configure test pulse on the CBC
    void configureTestPulse(Ph2_HwDescription::Chip* pCbc, uint8_t pPulseState);

    // method to mask all channels on the CBC
    void maskAllChannels(Ph2_HwDescription::Chip* pCbc);
    // method to return the position of the first stub in a CBC event
    uint8_t getStubPosition(std::vector<Ph2_HwInterface::Event*> pEvents, uint32_t pHybridId, uint32_t pCbcId, uint32_t pNEvents);

    /*!
     * \brief return mask for a given channel
     * \param pTestGroup: the  channel number [ between 1 and 254 ]
     */
    uint8_t getChanelMask(Ph2_HwDescription::Chip* pCbc, uint8_t pChannel);

    /*!
     * \brief find the channels of a test group
     * \param pTestGroup: the number of the test group
     * \return the channels in the pTestGroup
     */
    std::vector<uint8_t>      findChannelsInTestGroup(uint8_t pTestGroup);
    std::map<double, uint8_t> fWindowOffsetMapCBC3 = {
        {0.0, 0x00},
        {0.5, 0x01},
        {1.0, 0x02},
        {1.5, 0x03},
        {2.0, 0x04},
        {2.5, 0x05},
        {3.0, 0x06},
        {-0.5, 0x0f},
        {-1.0, 0x0e},
        {-1.5, 0x0d},
        {-2.0, 0x0c},
        {-2.5, 0x0b},
        {-3.0, 0x0a} // { -3.0 , 0x06}, {-2.5, 0x05 } , {-2.0 , 0x04} , {-1.5, 0x03} , {-1.0, 0x02} , {-0.5,0x01} , {0,0x00},
                     // { 3.0  , 0x0a} , {2.5 , 0x0b} , {2.0 , 0x0c } , {1.5,0x0d}, {1.0,0x0e}  , {0.5, 0x0f}
    };

    void setCorrelationWinodwOffsets(Ph2_HwDescription::Chip* pCbc, double pOffsetR1, double pOffsetR2, double pOffsetR3, double pOffsetR4);

    void writeObjects();
};

#endif
#endif
