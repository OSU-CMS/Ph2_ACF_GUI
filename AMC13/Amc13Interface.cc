#include "Amc13Interface.h"

Amc13Interface::Amc13Interface(const std::string& uriT1, const std::string& addressT1, const std::string& uriT2, const std::string& addressT2)
{
    // Log level
    // uhal::disableLogging();
    uhal::setLogLevelTo(uhal::Error());

    // this is the way if i want to keep the syntax
    uhal::ConnectionManager cm("file://HWInterface/dummy.xml");
    uhal::HwInterface       T1(cm.getDevice("T1", uriT1, addressT1));
    uhal::HwInterface       T2(cm.getDevice("T2", uriT2, addressT2));
    fAMC13 = new amc13::AMC13(T1, T2);

    // this would be the other way!
    // fAMC13 = new amc13::AMC13(uriT1, addressT1, uriT2, addressT2);
    // whatever else I need to do here!
}

Amc13Interface::~Amc13Interface() { delete fAMC13; }

void Amc13Interface::ConfigureAmc13()
{
    fAMC13->initAMC13();
    // first start with enabling AMCs!
    uint32_t cMask = 0;

    // first reset the 2 boards
    LOG(INFO) << "Resetting T1, T2 & all counters!";
    fAMC13->reset(amc13::AMC13Simple::T1);
    fAMC13->reset(amc13::AMC13Simple::T2);
    // now reset the counters
    fAMC13->resetCounters();
    // now enable the AMC inputs as specified in the config File
    LOG(INFO) << "Enabling TTC links for the following AMCs: ";

    std::stringstream ss;

    for(auto& cAMC: fDescription->fAMCMask)
    {
        setBit(cMask, cAMC, true);
        ss << cAMC << ", ";
    }

    LOG(INFO) << ss.str();
    fAMC13->AMCInputEnable(cMask);

    // now configure the BGOs, loop through the list, get the properties and call the function
    int cIndex = 0;

    if(!fDescription->fBGOs.empty())
    {
        for(auto& cBGO: fDescription->fBGOs)
        {
            this->configureBGO(cIndex, uint8_t(cBGO->fCommand), uint16_t(cBGO->fBX), uint16_t(cBGO->fPrescale), cBGO->fRepeat);
            this->enableBGO(cIndex);
            LOG(INFO) << "Configured & enabling BGO Channel " << cIndex << " : Command: " << cBGO->fCommand << " BX: " << cBGO->fBX << " Prescale: " << cBGO->fPrescale
                      << " Repetetive: " << cBGO->fRepeat;
            cIndex++;
        }
    }

    // now configure the Trigger
    if(fDescription->fTrigger != nullptr)
    {
        fAMC13->configureLocalL1A(
            fDescription->fTrigger->fLocal, fDescription->fTrigger->fMode, uint32_t(fDescription->fTrigger->fBurst), uint32_t(fDescription->fTrigger->fRate), fDescription->fTrigger->fRules);

        // Edit GA: not sure if this is actually required
        fAMC13->write(amc13::AMC13Simple::T1, "CONF.LOCAL_TRIG.FAKE_DATA_ENABLE", 1);

        LOG(INFO) << "Configuring local L1A: Mode: " << fDescription->fTrigger->fMode << " Rate: " << fDescription->fTrigger->fRate << " Burst: " << fDescription->fTrigger->fBurst
                  << " Rules: " << fDescription->fTrigger->fRules;
    }

    // if TTC simulator is enabled, the loopback fiber is required and no external TTC stream will be received, the
    // Triggers are local by definition
    if(fDescription->fSimulate)
    {
        fAMC13->localTtcSignalEnable(fDescription->fSimulate);
        LOG(INFO) << RED << "AMC13 configured to use local TTC simulator - don't forget to plug the loopback fibre!" << RESET;
    }

    // now need to iterate the two maps of Registers and write them
    for(auto& cReg: fDescription->fT1map) fAMC13->write(amc13::AMC13Simple::T1, cReg.first, cReg.second);

    for(auto& cReg: fDescription->fT2map) fAMC13->write(amc13::AMC13Simple::T2, cReg.first, cReg.second);

    LOG(INFO) << GREEN << "AMC13 successfully configured!" << RESET;
}

void Amc13Interface::StartL1A() { fAMC13->startContinuousL1A(); }

void Amc13Interface::StopL1A() { fAMC13->stopContinuousL1A(); }

void Amc13Interface::BurstL1A() { fAMC13->sendL1ABurst(); }

void Amc13Interface::EnableBGO(int pChan) { this->enableBGO(pChan); }

void Amc13Interface::DisableBGO(int pChan) { this->disableBGO(pChan); }

void Amc13Interface::EnableTTCHistory() { fAMC13->setTTCHistoryEna(true); }

void Amc13Interface::DisableTTCHistory() { fAMC13->setTTCHistoryEna(false); }

void Amc13Interface::ConfigureTTCHistory(std::vector<std::pair<int, uint32_t>> pFilterConfig)
{
    // n = int in the pair  ... history item
    // filterVal = uint32_t ... filter Value
    for(auto& cPair: pFilterConfig) fAMC13->setTTCHistoryFilter(cPair.first, cPair.second);
}

void Amc13Interface::DumpHistory(int pNlastEntries)
{
    fAMC13->setTTCHistoryEna(true);
    std::vector<uint32_t> cVec = fAMC13->getTTCHistory(pNlastEntries);

    // now decode the Info in here!
    LOG(INFO) << BOLDRED << "TTC History showing the last " << pNlastEntries << " items!" << RESET;

    for(int index = 0; index < cVec.size() / 4; index++)
    {
        // 4 32-bit words per command in the history
        uint32_t cCommand = cVec.at(index * 4 + 0);
        uint32_t cOrbit   = cVec.at(index * 4 + 1);
        uint32_t cBX      = cVec.at(index * 4 + 2);
        uint32_t cEvent   = cVec.at(index * 4 + 3);

        LOG(INFO) << "Command: " << (cCommand & 0xFF) << " - Orbit: " << cOrbit << " - BX: " << (cBX & 0x7FF) << " - Event Nr: " << (cEvent & 0x00FFFFFF);
    }
}

void Amc13Interface::DumpTriggers(int pNlastEntries)
{
    if(pNlastEntries > 127) std::cerr << "Only last 128 Events available in L1A history buffer!";

    std::vector<uint32_t> cVec = fAMC13->getL1AHistory(pNlastEntries);

    // now decode the Info in here!
    LOG(INFO) << BOLDRED << "L1A History showing the last " << pNlastEntries << " items!" << RESET;

    for(int index = 0; index < cVec.size() / 4; index++)
    {
        // 4 32-bit words per command in the history
        uint32_t cOrbit   = cVec.at(index * 4 + 0);
        uint32_t cBunch   = cVec.at(index * 4 + 1);
        uint32_t cEventNr = cVec.at(index * 4 + 2);
        uint32_t cFlags   = cVec.at(index * 4 + 3);

        LOG(INFO) << "Orbit: " << cOrbit << " - Bunch: " << (cBunch & 0xFFF) << " - Event Nr: " << (cEventNr & 0xFFFFFF) << " - Flags: " << cFlags;
    }
}

void Amc13Interface::HaltAMC13()
{
    LOG(INFO) << "Resetting T1, T2 & all counters!";
    fAMC13->reset(amc13::AMC13Simple::T1);
    fAMC13->reset(amc13::AMC13Simple::T2);
    // now reset the counters
    fAMC13->resetCounters();
}

void Amc13Interface::ResetAMC13()
{
    LOG(INFO) << "Resetting T1, T2 & all counters! - Remind Georg to add OC0 and EC0 when you read this!";
    fAMC13->reset(amc13::AMC13Simple::T1);
    fAMC13->reset(amc13::AMC13Simple::T2);
    // now reset the counters
    fAMC13->resetCounters();
}

void Amc13Interface::configureBGO(int pChan, uint8_t pCommand, uint16_t pBX, uint16_t pPrescale, bool pRepeat)
{
    // Edit GA: updated AMC13 core libraries and this should work now!
    fAMC13->configureBGOShort(pChan, pCommand, pBX, pPrescale, pRepeat);

    // this is the old stuff
    // char tmp[32];

    // if ( pChan < 0 || pChan > 3)
    //{
    // amc13::Exception::UnexpectedRange e;
    // e.Append ("AMC13::configureBGOShort() - channel must be in range 0 to 3");
    // throw e;
    //}

    // if ( pBX > 3563)
    //{
    // amc13::Exception::UnexpectedRange e;
    // e.Append ("AMC13::configureBGOShort() - bx must be in range 0 to 3563");
    // throw e;
    //}

    // snprintf ( tmp, sizeof (tmp), "CONF.TTC.BGO%d.%s", pChan, "COMMAND");
    // fAMC13->write ( amc13::AMC13Simple::T1, tmp, pCommand);

    // snprintf ( tmp, sizeof (tmp), "CONF.TTC.BGO%d.%s", pChan, "LONG_CMD");
    // fAMC13->write ( amc13::AMC13Simple::T1, tmp, 0);

    // snprintf ( tmp, sizeof (tmp), "CONF.TTC.BGO%d.%s", pChan, "BX");
    // fAMC13->write ( amc13::AMC13Simple::T1, tmp, pBX);

    // snprintf ( tmp, sizeof (tmp), "CONF.TTC.BGO%d.%s", pChan, "ORBIT_PRESCALE");
    // fAMC13->write ( amc13::AMC13Simple::T1, tmp, pPrescale);

    // snprintf ( tmp, sizeof (tmp), "CONF.TTC.BGO%d.%s", pChan, "ENABLE_SINGLE");

    // if ( !pRepeat)
    // fAMC13->write ( amc13::AMC13Simple::T1, tmp, 1);
    // else
    // fAMC13->write ( amc13::AMC13Simple::T1, tmp, 0);
}

std::vector<uint32_t> Amc13Interface::getBGOConfig(int pChan)
{
    /*
     *   [0] - repeat enabled (1=yes, 0=no)
     *   [1] - command length (1=long, 0=short)
     *   [2] - command value
     *   [3] - bunch crossing number
     *   [4] - orbit prescale value
     *   [5] - single command (1=single command enabled 0=disabled)
     */
    std::vector<uint32_t> pBuf;
    pBuf = fAMC13->getBGOConfig(pChan);

    if(pBuf.size() > 6)
        LOG(ERROR) << "There is a problem: BGO config vector should not be longer than 6 words!";

    else
    {
        LOG(INFO) << BOLDBLUE << "Channel: " << pChan;
        LOG(INFO) << BLUE << "Repetetive: " << pBuf.at(0);
        LOG(INFO) << BLUE << "  Cmd Word: " << std::hex << pBuf.at(2) << std::dec;
        LOG(INFO) << BLUE << "     BX nr: " << pBuf.at(3);
        LOG(INFO) << BLUE << "  Prescale: " << pBuf.at(4);
        LOG(INFO) << BLUE << "Repetetive: " << pBuf.at(5);
        LOG(INFO) << RESET;
    }
}

void Amc13Interface::SendBGO()
{
    fAMC13->sendBGO();
    // fAMC13->write(amc13::AMC13Simple::T1, "ACTION.TTC.SINGLE_COMMAND", 1);
}

void Amc13Interface::enableBGO(int pChan)
{
    char tmp[32];

    if(pChan < 0 || pChan > 3)
    {
        amc13::Exception::UnexpectedRange e;
        e.Append("AMC13::enableBGO() - channel must be in range 0 to 3");
        throw e;
    }

    snprintf(tmp, sizeof(tmp), "CONF.TTC.BGO%d.%s", pChan, "ENABLE");
    fAMC13->write(amc13::AMC13Simple::T1, tmp, 1);

    // Edit GA: updated AMC13 core libraries and this should work now!
    // fAMC13->enableBGO (pChan);

    // Edit by Georg Auzinger, not in official AMC13 SW package but required
    fAMC13->write(amc13::AMC13Simple::T1, "CONF.TTC.ENABLE_BGO", 1);
}

void Amc13Interface::disableBGO(int pChan)
{
    // char tmp[32];

    // if ( pChan < 0 || pChan > 3)
    //{
    // amc13::Exception::UnexpectedRange e;
    // e.Append ("AMC13::enableBGO() - channel must be in range 0 to 3");
    // throw e;
    //}

    // snprintf ( tmp, sizeof (tmp), "CONF.TTC.BGO%d.%s", pChan, "ENABLE");
    // fAMC13->write ( amc13::AMC13Simple::T1, tmp, 0);

    // Edit GA: updated AMC13 core libraries and this should work now!
    fAMC13->disableBGO(pChan);

    // Edit by Georg Auzinger, not in official AMC13 SW package but required
    fAMC13->write(amc13::AMC13Simple::T1, "CONF.TTC.ENABLE_BGO", 0);
}

void Amc13Interface::SendEC0() { fAMC13->sendLocalEvnOrnReset(1, 0); }
