/*!

        \file                   KIRA.h
        \brief                  Class to control the KIRA systems for 2S Modules
        \author                 Roland Koppenhöfer / Stefan Maier
        \version                1.0
        \date                   03/05/2022
        Support :               mail to : s.maier@kit.edu

 */
#ifndef KIRA_h__
#define KIRA_h__

#include "NetworkUtils/TCPClient.h"
#include "NetworkUtils/TCPPublishServer.h"
#include "OTTool.h"
#include "Utils/ContainerRecycleBin.h"

#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/time.h>

#ifdef __USE_ROOT__
#include "DQMUtils/DQMHistogramKira.h"
#endif

class Occupancy;

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

class KIRA : public OTTool
{
  public:
    KIRA();
    ~KIRA();
    // void IntensityCalibration();
    void Initialise(int kiraPort, std::string kiraId);
    void PrepareForExternal();
    void determineLatency();
    void performKIRATest();
    void calibrateIntensity();
    void initializeRecycleBin() { fRecycleBin.setDetectorContainer(fDetectorContainer); }
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;
    bool check_channel_illumination(BeBoard* pBoard, DetectorDataContainer& pContainer, uint16_t pLED);

  private:
    TCPClient*                     fKiraClient{nullptr};
    std::string                    fKiraId;
    ContainerRecycleBin<Occupancy> fRecycleBin;
    uint32_t                       fTargetIntensity;
#ifdef __USE_ROOT__
    DQMHistogramKira fDQMHistogrammer;
#endif
    DetectorDataContainer analyseEvents(BeBoard* pBoard, const std::vector<Event*>& pEvents, uint16_t pSensor, uint16_t pLED, bool pSkipChips = true);
};

#endif
