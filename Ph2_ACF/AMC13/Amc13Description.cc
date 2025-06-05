#include "Amc13Description.h"

Amc13Description::Amc13Description()
{
    fTrigger = nullptr;
    fT1map.clear();
    fT2map.clear();
    fAMCMask.clear();
    fBGOs.clear();
}

Amc13Description::~Amc13Description()
{
    delete fTrigger;

    for(auto& cBGO: fBGOs) delete cBGO;

    fBGOs.clear();
    fAMCMask.clear();
    fT1map.clear();
    fT2map.clear();
}

uint32_t Amc13Description::getReg(int pTounge, std::string& pReg)
{
    RegMap cMap = (pTounge == 1) ? fT1map : fT2map;
    auto   cReg = cMap.find(pReg);

    if(cReg == std::end(cMap))
        LOG(INFO) << "The AMC13 does not have a register " << pReg << " in the memory map for Tounge " << pTounge;
    else
        return cReg->second;
}

void Amc13Description::setReg(int pTounge, std::string& pReg, uint32_t pValue)
{
    RegMap cMap = (pTounge == 1) ? fT1map : fT2map;
    auto   cReg = cMap.find(pReg);

    if(cReg == std::end(cMap))
        cMap.insert({pReg, pValue});
    else
        cReg->second = pValue;
}

void Amc13Description::setAMCMask(const std::vector<int>& pMask) { fAMCMask = pMask; }

void Amc13Description::setTrigger(bool pLocal, int pMode, int pRate, int pBurst, int pRules)
{
    if(fTrigger != nullptr)
    {
        LOG(INFO) << "Warning, overwriting another Trigger object!";
        delete fTrigger;
    }

    fTrigger = new Trigger(pLocal, pMode, pRate, pBurst, pRules);
}

void Amc13Description::setTrigger(Trigger* pTrigger)
{
    if(fTrigger != nullptr)
    {
        LOG(INFO) << "Warning, overwriting another Trigger object!";
        delete fTrigger;
    }

    fTrigger = pTrigger;
}

Trigger* Amc13Description::getTrigger() { return fTrigger; }

void Amc13Description::setTTCSimulator(bool pSimulate) { fSimulate = pSimulate; }
void Amc13Description::addBGO(int pCommand, bool pRepeat, int pPrescale, int pBX)
{
    int cSize = fBGOs.size();

    if(cSize > 4)
        LOG(INFO) << "Warning, AMC13XG only supports 4 user-defined BGOs - adding this one will have no effect!";
    else
    {
        BGO* cBGO = new BGO(pCommand, pRepeat, pPrescale, pBX);
        fBGOs.push_back(cBGO);
    }
}

void Amc13Description::addBGO(BGO* pBGO)
{
    int cSize = fBGOs.size();

    if(cSize > 4)
        LOG(INFO) << "Warning, AMC13XG only supports 4 user-defined BGOs - adding this one will have no effect!";
    else
        fBGOs.push_back(pBGO);
}

BGO* Amc13Description::getBGO(int pPos)
{
    if(pPos > 3)
    {
        LOG(INFO) << "AMC13XG only supports 4 custom BGOs, ID > 3 is not allowed!";
        return nullptr;
    }
    else
        return fBGOs.at(pPos);
}
