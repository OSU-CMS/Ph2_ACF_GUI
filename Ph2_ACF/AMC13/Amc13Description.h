#ifndef _AMC13_DESCRIPTION_h__
#define _AMC13_DESCRIPTION_h__

#include "Utils/easylogging++.h"
#include <iostream>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

typedef std::map<std::string, uint32_t> RegMap;

// a little forward declaration!
struct Trigger;
struct BGO;

class Amc13Description
{
  public:
    Amc13Description();
    ~Amc13Description();

    uint32_t getReg(int pTounge, std::string& pReg);
    void     setReg(int pTounge, std::string& pReg, uint32_t pValue);
    RegMap   getRegMap(int pTounge) { return (pTounge == 1) ? fT1map : fT2map; }
    int      Id;

    void     setTrigger(bool pLocal, int pType, int pRate, int pBurst, int pRules);
    void     setTrigger(Trigger* pTrigger);
    Trigger* getTrigger();

    void addBGO(int pCommand, bool pRepeat, int pPrescale, int pBX);
    void addBGO(BGO* pBGO);
    BGO* getBGO(int pPos);

    void setAMCMask(const std::vector<int>& pMask);
    void setTTCSimulator(bool pSimulate);

  public:
    RegMap            fT1map;
    RegMap            fT2map;
    std::vector<int>  fAMCMask;
    std::vector<BGO*> fBGOs;
    Trigger*          fTrigger;
    bool              fSimulate;
};

struct BGO
{
    BGO(int pCommand, bool pRepeat, int pPrescale, int pBX) : fCommand(pCommand), fRepeat(pRepeat), fPrescale(pPrescale), fBX(pBX) {}
    int  fCommand;
    bool fRepeat;
    int  fPrescale;
    int  fBX;
};

struct Trigger
{
    Trigger(bool pLocal, int pMode, int pRate, int pBurst, int pRules) : fLocal(pLocal), fMode(pMode), fRate(pRate), fBurst(pBurst), fRules(pRules) {}
    bool fLocal;
    int  fMode;
    int  fRate;
    int  fBurst;
    int  fRules;
    // mode 0 = periodic trigger every rate orbits at BX=500
    // mode 1 = periodic trigger every rate BX
    // mode 2 = random trigger at rate Hz
};

#endif
