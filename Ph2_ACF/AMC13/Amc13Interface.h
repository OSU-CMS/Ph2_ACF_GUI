#ifndef _AMC13_INTERFACE_H__
#define _AMC13_INTERFACE_H__

#include "Amc13Description.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"
#include "amc13/AMC13.hh"
#include "uhal/uhal.hpp"
#include <iostream>
#include <string>

class Amc13Interface
{
  public:
    Amc13Interface(const std::string& uriT1, const std::string& addressT1, const std::string& uriT2, const std::string& addressT2);
    ~Amc13Interface();

    void setAmc13Description(Amc13Description* pDescription) { fDescription = pDescription; }
    // a long list of methods wrapping the BU AMC13 methods!
    // Configure and Halt for something like a State Machine!
    void ConfigureAmc13();
    void HaltAMC13();
    void ResetAMC13();
    // Start / Stop L1A
    void StartL1A();
    void StopL1A();
    void BurstL1A();

    void                  SendBGO();
    void                  EnableBGO(int pChan);
    void                  DisableBGO(int pChan);
    std::vector<uint32_t> getBGOConfig(int pChan);
    void                  SendEC0();

    // TTC History methods!
    void ConfigureTTCHistory(std::vector<std::pair<int, uint32_t>> pFilterConfig);
    void EnableTTCHistory();
    void DisableTTCHistory();
    void DumpHistory(int pNlastEntries);
    void DumpTriggers(int pNlastEntries);

  private:
    amc13::AMC13*     fAMC13;
    Amc13Description* fDescription;

    void setBit(uint32_t& pRegValue, uint8_t pPos, bool pValue) { pRegValue ^= (-pValue ^ pRegValue) & (1 << pPos - 1); }
    void enableBGO(int pChan);
    void disableBGO(int pChan);
    void configureBGO(int pChan, uint8_t pCommand, uint16_t pBX, uint16_t pPrescale, bool pRepeat);
};

#endif
