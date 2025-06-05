/*

        FileName :                     CbcInterface.cc
        Content :                      User Interface to the Cbcs
        Programmer :                   Lorenzo BIDEGAIN, Nicolas PIERRE, Georg AUZINGER
        Version :                      1.0
        Date of creation :             10/07/14
        Support :                      mail to : lorenzo.bidegain@gmail.com, nico.pierre@icloud.com

 */

#include "HWInterface/CbcInterface.h"
#include "Utils/ChannelGroupHandler.h"
#include "Utils/ConsoleColor.h"
#include "Utils/Container.h"
#include <bitset>

#define DEV_FLAG 0

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
CbcInterface::CbcInterface(const BeBoardFWMap& pBoardMap) : ReadoutChipInterface(pBoardMap)
{
    fActiveChannels.reset();
    resetPageMap();
}

CbcInterface::~CbcInterface() {}

bool CbcInterface::ConfigureChip(Chip* pCbc, bool pVerify, uint32_t pBlockSize)
{
    std::stringstream cOutput;
    setBoard(pCbc->getBeBoardId());
    pCbc->printChipType(cOutput);
    LOG(INFO) << BOLDBLUE << cOutput.str() << "...Configuring chip with Id[" << +pCbc->getId() << "] oh Hybrid" << +pCbc->getHybridId() << RESET;

    // sort registers by page + address
    ChipRegMap                                       cCbcRegMap = pCbc->getRegMap();
    std::vector<std::pair<std::string, ChipRegItem>> cRegList;
    cRegList.clear();
    auto theListOfFreeRegisters = pCbc->getFreeRegisters();
    for(auto cMapItem: cCbcRegMap)
    {
        bool isFreeRegister = false;
        for(const auto& freeRegister: theListOfFreeRegisters)
        {
            isFreeRegister = std::regex_match(cMapItem.first, freeRegister.first);
            if(isFreeRegister) break;
        }
        if(isFreeRegister) continue; // skipping readonly registers
        std::pair<std::string, ChipRegItem> cItem;
        cItem.first  = cMapItem.first;
        cItem.second = cMapItem.second;
        cRegList.push_back(cItem);
    }

    // sort register map
    if(fSortPageInc)
        std::sort(cRegList.begin(), cRegList.end(), customPageInc); // all to page0 then page 1
    else
        std::sort(cRegList.begin(), cRegList.end(), customPageDec); // all to page1 then page 0
    std::sort(cRegList.begin(), cRegList.end(), customAddressInc);  // sort by address - for convenience later

    // First configure all registers on page0
    std::vector<ChipRegItem> cRegItemsPg0;
    std::vector<ChipRegItem> cRegItemsPg1;
    for(auto& cReg: cRegList)
    {
        if(cReg.second.fPage == 0)
            cRegItemsPg0.push_back(cReg.second);
        else
            cRegItemsPg1.push_back(cReg.second);
    }
    // set page 0
    resetPageMap();
    if(!ConfigurePage(pCbc, 0, pVerify)) return false;
    if(!fBoardFW->MultiRegisterWrite(pCbc, cRegItemsPg0, pVerify)) return false;
    LOG(INFO) << BOLDGREEN << "Configured all registers on page0 [" << cRegItemsPg0.size() << " regs]" RESET;

    if(!ConfigurePage(pCbc, 1, pVerify)) return false;
    if(!fBoardFW->MultiRegisterWrite(pCbc, cRegItemsPg1, pVerify)) return false;
    LOG(INFO) << BOLDGREEN << "Configured all registers on page1 [" << cRegItemsPg1.size() << " regs]" RESET;

    return true;
}

bool CbcInterface::enableInjection(ReadoutChip* pChip, bool inject, bool pVerify) { return this->WriteChipReg(pChip, "TestPulse", (int)inject, pVerify); }

bool CbcInterface::setInjectionAmplitude(ReadoutChip* pChip, uint8_t injectionAmplitude, bool pVerify) { return this->WriteChipReg(pChip, "TestPulsePotNodeSel", injectionAmplitude, pVerify); }

bool CbcInterface::setInjectionSchema(ReadoutChip* pCbc, const std::shared_ptr<ChannelGroupBase> group, bool pVerify)
{
    std::bitset<NCHANNELS> cBitset = std::bitset<NCHANNELS>(std::static_pointer_cast<const ChannelGroup<1, NCHANNELS>>(group)->getBitset());
    if(cBitset.count() == 0) // no mask set... so do nothing
        return true;
    // LOG (DEBUG) << BOLDBLUE << "Setting injection scheme for " << std::bitset<NCHANNELS>(cBitset) << RESET;
    uint16_t cFirstHit;
    for(cFirstHit = 0; cFirstHit < NCHANNELS; cFirstHit++)
    {
        if(cBitset[cFirstHit] != 0) break;
    }
    uint8_t cGroupId = std::floor((cFirstHit % 16) / 2);
    // LOG (DEBUG) << BOLDBLUE << "First unmasked channel in position " << +cFirstHit << " --- i.e. in TP group " <<
    // +cGroupId << RESET;
    if(cGroupId > 7)
        throw Exception("bool CbcInterface::setInjectionSchema (ReadoutChip* pCbc, const ChannelGroupBase *group, bool "
                        "pVerify): CBC is not able to inject the channel pattern");
    // write register which selects group
    return this->WriteChipReg(pCbc, "TestPulseGroup", cGroupId, pVerify);
}

bool CbcInterface::maskChannelGroup(ReadoutChip* pCbc, const std::shared_ptr<ChannelGroupBase> group, bool pVerify)
{
    auto originalMask = std::static_pointer_cast<const ChannelGroup<1, NCHANNELS>>(pCbc->getChipOriginalMask());
    auto groupToMask  = std::static_pointer_cast<const ChannelGroup<1, NCHANNELS>>(group);
    LOG(DEBUG) << BOLDBLUE << "\t... Applying mask to CBC" << +pCbc->getId() << " with " << group->getNumberOfEnabledChannels()
               << " enabled channels\t... mask : " << std::bitset<NCHANNELS>(groupToMask->getBitset()) << RESET;

    fActiveChannels = groupToMask->getBitset();
    // auto  a = static_cast<const ChannelGroup<1, NCHANNELS>*>(groupToMask);
    std::vector<std::pair<std::string, uint16_t>> cRegVec;
    cRegVec.clear();
    std::bitset<NCHANNELS> tmpBit(255);
    for(uint8_t maskGroup = 0; maskGroup < 32; ++maskGroup)
    {
        // uint16_t cValue = (uint16_t)((originalMask->getBitset() & fActiveChannels) >> (maskGroup << 3) & tmpBit).to_ulong();
        // LOG(DEBUG) << BOLDBLUE << "\t...Group" << +maskGroup << " : " << std::bitset<8>(cValue) << RESET;
        cRegVec.push_back(make_pair(fChannelMaskMapCBC3.at(maskGroup), (uint16_t)((originalMask->getBitset() & fActiveChannels) >> (maskGroup << 3) & tmpBit).to_ulong()));
    }
    return WriteChipMultReg(pCbc, cRegVec, pVerify);
}

bool CbcInterface::maskChannelsAndSetInjectionSchema(ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool mask, bool inject, bool pVerify)
{
    bool success = true;
    if(mask) success &= maskChannelGroup(pChip, group, pVerify);
    if(inject) success &= setInjectionSchema(pChip, group, pVerify);
    return success;
}

bool CbcInterface::ConfigureChipOriginalMask(ReadoutChip* pCbc, bool pVerify, uint32_t pBlockSize)
{
    auto allChannelEnabledGroup = std::make_shared<ChannelGroup<1, NCHANNELS>>();
    return CbcInterface::maskChannelGroup(pCbc, allChannelEnabledGroup, pVerify);
}

std::vector<uint8_t> CbcInterface::createHitListFromStubs(uint8_t pSeed, bool pSeedLayer)
{
    std::vector<uint8_t> cChannelList(0);
    uint32_t             cSeedStrip = std::floor(pSeed / 2.0); // counting from 1
    // LOG(DEBUG) << BOLDMAGENTA << "Seed of " << +pSeed << " means first hit is in strip " << +cSeedStrip << RESET;
    size_t cNumberOfChannels = 1 + (pSeed % 2 != 0);
    for(size_t cIndex = 0; cIndex < cNumberOfChannels; cIndex++)
    {
        uint32_t cSeedChannel = 2 * (cSeedStrip - 1) + !pSeedLayer + 2 * cIndex;
        cChannelList.push_back(static_cast<uint32_t>(cSeedChannel));
    }
    return cChannelList;
}
std::vector<uint8_t> CbcInterface::stubInjectionPattern(uint8_t pStubAddress, int pStubBend, bool pLayerSwap)
{
    // LOG(DEBUG) << BOLDBLUE << "Injecting... stub in position " << +pStubAddress << " [half strips] with a bend of " << pStubBend << " [half strips]." << RESET;
    std::vector<uint8_t> cSeedHits = createHitListFromStubs(pStubAddress, !pLayerSwap);
    // correlation layer
    uint8_t              cCorrelated     = pStubAddress + pStubBend; // start counting strips from 0
    std::vector<uint8_t> cCorrelatedHits = createHitListFromStubs(cCorrelated, pLayerSwap);
    // merge two lists and unmask
    cSeedHits.insert(cSeedHits.end(), cCorrelatedHits.begin(), cCorrelatedHits.end());
    return cSeedHits;
}
std::vector<uint8_t> CbcInterface::stubInjectionPattern(ReadoutChip* pChip, uint8_t pStubAddress, int pStubBend)
{
    bool cLayerSwap = (this->ReadChipReg(pChip, "LayerSwap") == 1);
    return stubInjectionPattern(pStubAddress, pStubBend, cLayerSwap);
}

bool CbcInterface::injectClusters(ReadoutChip* pCbc, std::vector<std::pair<uint8_t, uint8_t>> theClusterAddressAndWidthVector)
{
    ChannelGroup<1, NCHANNELS> theChannelMask;
    theChannelMask.disableAllChannels();
    std::vector<uint8_t> cActiveChannels(0);
    std::vector<uint8_t> cDisabledChannels(0);
    for(const auto& theClusterAddressAndWidth: theClusterAddressAndWidthVector)
    {
        for(size_t strip = 0; strip < theClusterAddressAndWidth.second; ++strip) { theChannelMask.enableChannel(0, theClusterAddressAndWidth.first + 2 * strip); }
    }

    uint16_t cVcth = 1023;
    this->WriteChipReg(pCbc, "VCth", cVcth);
    return this->maskChannelGroup(pCbc, std::make_shared<ChannelGroup<1, NCHANNELS>>(std::move(theChannelMask)));
}

bool CbcInterface::injectStubs(ReadoutChip* pCbc, std::vector<std::pair<uint8_t, int>> theStubAddressesAndBendVector, bool pUseNoise, bool pUseOffsets, uint8_t pAllOff)
{
    setBoard(pCbc->getBeBoardId());

    ChannelGroup<1, NCHANNELS> cChannelMask;
    cChannelMask.disableAllChannels();
    std::vector<uint8_t> cActiveChannels(0);
    std::vector<uint8_t> cDisabledChannels(0);
    for(const auto& theStubAddressesAndBend: theStubAddressesAndBendVector)
    {
        std::vector<uint8_t> cPattern = this->stubInjectionPattern(pCbc, theStubAddressesAndBend.first, theStubAddressesAndBend.second);
        // for(auto cChannel: cPattern) cChannelMask.enableChannel(0, cChannel);
        for(size_t cChnl = 0; cChnl < pCbc->size(); cChnl++)
        {
            if(std::find(cPattern.begin(), cPattern.end(), cChnl) != cPattern.end())
            {
                cActiveChannels.push_back(cChnl);
                cChannelMask.enableChannel(0, cChnl);
            }
            else
                cDisabledChannels.push_back(cChnl);
        }
    }
    if(!pUseNoise) // with TP
    {
        uint16_t               cFirstHit = 0;
        std::bitset<NCHANNELS> cBitset   = std::bitset<NCHANNELS>(cChannelMask.getBitset());
        // LOG(DEBUG) << BOLDMAGENTA << "Bitset for this mask is " << cBitset << RESET;
        for(cFirstHit = 0; cFirstHit < NCHANNELS; cFirstHit++)
        {
            if(cBitset[cFirstHit] != 0) break;
        }
        uint8_t cGroupId = std::floor((cFirstHit % 16) / 2);
        // LOG(DEBUG) << BOLDBLUE << "First unmasked channel in position " << +cFirstHit << " --- i.e. in TP group " << +cGroupId << RESET;
        if(cGroupId > 7)
            throw Exception("bool CbcInterface::setInjectionSchema (ReadoutChip* pCbc, const ChannelGroupBase *group, "
                            "bool pVerify): CBC is not able to inject the channel pattern");
        // write register which selects group
        this->WriteChipReg(pCbc, "TestPulseGroup", cGroupId);
        // write registers which enable injection
        this->enableInjection(pCbc, true); // enable injection
        // write register which sets TP amplitude
        // this->setInjectionAmplitude(pCbc, 0xFF - 100); // fix injection amplitude
        return this->maskChannelGroup(pCbc, std::make_shared<ChannelGroup<1, NCHANNELS>>(std::move(cChannelMask)));
    }
    else // with noise
    {
        // assuming global chip threshold
        // is already at the pedestal
        if(pUseOffsets)
        {
            uint8_t cAllOff = pAllOff;
            uint8_t cAllOn  = 0x00;
            // use offsets on individual disc. to shift output to all 1 or all 0
            // always off for disabled channels
            std::vector<std::pair<std::string, uint16_t>> cVecReq;
            for(auto cDisabledChannel: cDisabledChannels)
            {
                char cDacName[20];
                sprintf(cDacName, "Channel%03d", cDisabledChannel + 1);
                cVecReq.push_back({cDacName, cAllOff});
            }
            // always on for active channels
            for(auto cActiveChannel: cActiveChannels)
            {
                char cDacName[20];
                sprintf(cDacName, "Channel%03d", cActiveChannel + 1);
                cVecReq.push_back({cDacName, cAllOn});
            }
            return this->WriteChipMultReg(pCbc, cVecReq, true);
        }
        else
        {
            uint16_t cVcth = 1023;
            this->WriteChipReg(pCbc, "VCth", cVcth);
            return this->maskChannelGroup(pCbc, std::make_shared<ChannelGroup<1, NCHANNELS>>(std::move(cChannelMask)));
        }
    }
}

std::vector<uint8_t> CbcInterface::readLUT(ReadoutChip* pCbc, uint8_t pMode)
{
    setBoard(pCbc->getBeBoardId());
    std::vector<uint8_t> cBendCodes(30, 0); // bend registers are 0 -- 14. Each register encodes 2 codes
    for(size_t cIndex = 0; cIndex < 15; cIndex += 1)
    {
        const size_t cLength = (cIndex < 10) ? 5 : 6;
        char         cBuffer[20];
        sprintf(cBuffer, "Bend%d", static_cast<int>(cIndex));
        std::string cRegName(cBuffer, cLength);
        // LOG(DEBUG) << BOLDBLUE << "Reading bend register " << cRegName << RESET;
        uint16_t cValue               = (pMode == 0) ? this->ReadChipReg(pCbc, cRegName) : pCbc->getReg(cRegName);
        cBendCodes.at(cIndex * 2)     = (cValue & 0x0F);
        cBendCodes.at(cIndex * 2 + 1) = (cValue & 0xF0) >> 4;
    }
    return cBendCodes;
}

uint16_t CbcInterface::readErrorRegister(ReadoutChip* pCbc)
{
    // read I2c register with error flags
    setBoard(pCbc->getBeBoardId());
    ChipRegItem cRegItem;
    cRegItem.fPage    = 0x01;
    cRegItem.fAddress = 0x1D;
    ConfigurePage(pCbc, cRegItem.fPage, true);
    return fBoardFW->SingleRegisterRead(pCbc, cRegItem);
}

bool CbcInterface::selectLogicMode(ReadoutChip* pCbc, std::string pModeSelect, bool pForHits, bool pForStubs, bool pVerify)
{
    uint8_t pMode;
    if(pModeSelect == "Sampled")
        pMode = 0;
    else if(pModeSelect == "OR")
        pMode = 1;
    else if(pModeSelect == "HIP")
        pMode = 2;
    else if(pModeSelect == "Latched")
        pMode = 3;
    else
    {
        LOG(INFO) << BOLDYELLOW << "Invalid mode selected! Valid modes are Sampled/OR/Latched/HIP" << RESET;
        return false;
    }
    std::vector<std::pair<std::string, uint16_t>> cRegVec;
    setBoard(pCbc->getBeBoardId());
    std::string cRegName       = "Pipe&StubInpSel&Ptwidth";
    uint16_t    cOriginalValue = this->ReadChipReg(pCbc, cRegName);
    uint16_t    cMask          = 0xFF - ((3 * pForHits << 6) | (3 * pForStubs << 4));
    uint16_t    cRegValue      = (cOriginalValue & cMask) | (pMode * pForHits << 6) | (pMode * pForStubs << 4);
    // LOG (DEBUG) << BOLDBLUE << "Original register value : 0x" << std::hex << cOriginalValue << std::dec << "\t logic
    // register set to 0x" << std::hex << +cRegValue << std::dec << " to select " << pModeSelect << " mode on CBC" <<
    // +pCbc->getId() << RESET;
    return WriteChipSingleReg(pCbc, cRegName, cRegValue, pVerify);
    // WriteChipSingleReg
    // uint8_t cMask = (uint8_t)(~(((3*pForHits) << 6) | ((3*pForStubs) << 4)));
    // uint8_t cValue = (((pMode*pForHits) << 6) | ((pMode*pForStubs) << 4)) | (cOriginalValue & cMask);
    // LOG (DEBUG) << BOLDBLUE << "Settinng logic selection register to 0x" << std::hex << +cValue << std::dec << " to
    // select " << pModeSelect << " mode on CBC" << +pCbc->getId() << RESET; cRegVec.emplace_back (cRegName, cValue);
    // return WriteChipMultReg (pCbc, cRegVec, pVerify);
}

bool CbcInterface::enableHipSuppression(ReadoutChip* pCbc, bool pForHits, bool pForStubs, uint8_t pClocks, bool pVerify)
{
    std::vector<std::pair<std::string, uint16_t>> cRegVec;
    setBoard(pCbc->getBeBoardId());
    // configure logic mode
    if(!this->selectLogicMode(pCbc, "HIP", pForHits, pForStubs, pVerify))
    {
        LOG(INFO) << BOLDYELLOW << "Could not select HIP mode..." << RESET;
        return false;
    }
    // configure hip logic
    std::string cRegName       = "HIP&TestMode";
    uint8_t     cOriginalValue = this->ReadChipReg(pCbc, cRegName);
    uint8_t     cMask          = 0x0F;
    bool        cEnableHips    = true;
    uint8_t     cMaxClocks     = 7;
    uint8_t     cValue         = (((pClocks << 5) & (cMaxClocks << 5)) & (!cEnableHips << 4)) | (cOriginalValue & cMask);
    // LOG (DEBUG) << BOLDBLUE << "Settinng HIP configuration register to 0x" << std::hex << +cValue << std::dec << " to
    // enable max. " << +pClocks << " clocks on CBC" << +pCbc->getId() << RESET;
    cRegVec.emplace_back(cRegName, cValue);
    return WriteChipMultReg(pCbc, cRegVec, pVerify);
}

bool CbcInterface::MaskAllChannels(ReadoutChip* pCbc, bool mask, bool pVerify)
{
    ChannelGroup<1, NCHANNELS> cChannelMask;
    if(mask)
        cChannelMask.disableAllChannels();
    else
        cChannelMask.enableAllChannels();
    // LOG (DEBUG)  << BOLDBLUE << "Mask to be set is " << std::bitset<254>( cChannelMask.getBitset() ) << RESET;
    return this->maskChannelGroup(pCbc, std::make_shared<ChannelGroup<1, NCHANNELS>>(std::move(cChannelMask)), pVerify);
}

bool CbcInterface::WriteChipReg(Chip* pCbc, const std::string& dacName, uint16_t dacValue, bool pVerify)
{
    // LOG(DEBUG) << BOLDYELLOW << "CbcInterface::WriteChipReg " << dacName << RESET;
    if(dacName == "VCth" || dacName == "Threshold")
    {
        if(pCbc->getFrontEndType() == FrontEndType::CBC3)
        {
            if(dacValue > 1023)
                LOG(ERROR) << "Error, Threshold for CBC3 can only be 10 bit max (1023)!";
            else
            {
                std::vector<std::pair<std::string, uint16_t>> cRegVec;
                // VCth1 holds bits 0-7 and VCth2 holds 8-9
                uint16_t cVCth1 = dacValue & 0x00FF;
                uint16_t cVCth2 = (dacValue & 0x0300) >> 8;
                cRegVec.emplace_back("VCth1", cVCth1);
                cRegVec.emplace_back("VCth2", cVCth2);
                return WriteChipMultReg(pCbc, cRegVec, pVerify);
            }
        }
        else
            LOG(ERROR) << __PRETTY_FUNCTION__ << " Not a valid chip type!";
    }
    else if(dacName == "ClusterCut")
    {
        uint8_t cRegValue = ReadChipSingleReg(pCbc, "LayerSwap&CluWidth");
        uint8_t cValue    = (cRegValue & 0xF8) | dacValue;
        return WriteChipSingleReg(pCbc, "LayerSwap&CluWidth", cValue, pVerify);
    }
    else if(dacName == "TriggerLatency")
    {
        if(pCbc->getFrontEndType() == FrontEndType::CBC3)
        {
            if(dacValue > 511)
                LOG(ERROR) << "Error, Threshold for CBC3 can only be 10 bit max (1023)!";
            else
            {
                std::vector<std::pair<std::string, uint16_t>> cRegVec;
                // TriggerLatency1 holds bits 0-7 and FeCtrl&TrgLate2 holds 8
                uint16_t cLat1 = dacValue & 0x00FF;
                uint16_t cLat2 = (pCbc->getReg("FeCtrl&TrgLat2") & 0xFE) | ((dacValue & 0x0100) >> 8);
                cRegVec.emplace_back("TriggerLatency1", cLat1);
                cRegVec.emplace_back("FeCtrl&TrgLat2", cLat2);
                // LOG(INFO) << BOLDBLUE << "Setting latency on " << +pCbc->getId() << " to " << +dacValue << " 0x" << std::hex << +cLat1 << std::dec << " --- 0x" << std::hex << +cLat2 << std::dec
                //            << " for a latency vale of " << dacValue << RESET;
                return WriteChipMultReg(pCbc, cRegVec, pVerify);
            }
        }
        else
            LOG(ERROR) << __PRETTY_FUNCTION__ << " Not a valid chip type!";
    }
    else if(dacName == "TestPulseDelay")
    {
        uint8_t cValue = pCbc->getReg("TestPulseDel&ChanGroup");
        uint8_t cGroup = cValue & 0x07;
        // groupId is reversed in this register
        std::bitset<5> cDelay  = dacValue;
        std::string    cSelect = cDelay.to_string();
        std::reverse(cSelect.begin(), cSelect.end());
        std::bitset<5> cTestPulseDelay(cSelect);
        uint8_t        cRegValue = (cGroup | (static_cast<uint8_t>(cTestPulseDelay.to_ulong()) << 3));
        // LOG(DEBUG) << BOLDBLUE << "Setting test pulse delay for goup [rev.] " << std::bitset<3>(cGroup) << " to " << +dacValue << " --  register to  0x" << std::bitset<8>(+cRegValue) << std::dec
        //            << RESET;
        return WriteChipSingleReg(pCbc, "TestPulseDel&ChanGroup", cRegValue, pVerify);
    }
    else if(dacName == "TestPulseGroup")
    {
        uint8_t cValue = pCbc->getReg("TestPulseDel&ChanGroup");
        uint8_t cDelay = cValue & 0xF8;
        // groupId is reversed in this register
        std::bitset<3> cGroup  = dacValue;
        std::string    cSelect = cGroup.to_string();
        std::reverse(cSelect.begin(), cSelect.end());
        std::bitset<3> cTestPulseGroup(cSelect);
        uint8_t        cRegValue = (cDelay | (static_cast<uint8_t>(cTestPulseGroup.to_ulong())));
        // LOG(DEBUG) << BOLDBLUE << "Setting test pulse register on CBC" << +pCbc->getId() << " to select group " << +dacValue << " --  register to  0x" << std::bitset<8>(+cRegValue) << std::dec
        //            << RESET;
        return WriteChipSingleReg(pCbc, "TestPulseDel&ChanGroup", cRegValue, pVerify);
    }
    else if(dacName == "AmuxOutput")
    {
        uint8_t cValue    = pCbc->getReg("MiscTestPulseCtrl&AnalogMux");
        uint8_t cRegValue = (cValue & 0xE0) | dacValue;
        // LOG(DEBUG) << BOLDBLUE << "Setting AmuxOutput on Chip" << +pCbc->getId() << " to " << +dacValue << " - register set to : 0x" << std::hex << +cRegValue << std::dec << RESET;
        bool cSuccess = WriteChipSingleReg(pCbc, "MiscTestPulseCtrl&AnalogMux", cRegValue, pVerify);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return cSuccess;
    }
    else if(dacName == "TestPulse" || dacName == "InjectedCharge")
    {
        uint8_t cValue    = pCbc->getReg("MiscTestPulseCtrl&AnalogMux");
        uint8_t cRegValue = (cValue & 0xBF) | (dacValue << 6);
        return WriteChipSingleReg(pCbc, "MiscTestPulseCtrl&AnalogMux", cRegValue, pVerify);
    }
    else if(dacName == "HitOr")
    {
        uint8_t cValue    = pCbc->getReg("40MhzClk&Or254");
        uint8_t cRegValue = (cValue & 0xBf) | (dacValue << 6);
        // LOG(DEBUG) << BOLDBLUE << "Setting HITOr on Chip" << +pCbc->getId() << " from 0x" << std::hex << +cValue << std::dec << " to " << +dacValue << " - register set to : 0x" << std::hex
        //            << +cRegValue << std::dec << RESET;
        return WriteChipSingleReg(pCbc, "40MhzClk&Or254", cRegValue, pVerify);
    }
    else if(dacName == "DLL")
    {
        uint8_t cOriginalValue = pCbc->getReg("40MhzClk&Or254");
        // dll is reversed in this register
        std::bitset<5> cDelay  = dacValue;
        std::string    cSelect = cDelay.to_string();
        std::reverse(cSelect.begin(), cSelect.end());
        std::bitset<5> cClockDelay(cSelect);
        uint8_t        cNewRegValue = ((cOriginalValue & 0xE0) | static_cast<uint8_t>(cClockDelay.to_ulong()));
        // LOG(DEBUG) << BOLDBLUE << "Setting clock delay on Chip" << +pCbc->getId() << " to " << std::bitset<5>(+dacValue) << " - register set to : 0x" << std::hex << +cNewRegValue << std::dec <<
        // RESET;
        return WriteChipSingleReg(pCbc, "40MhzClk&Or254", cNewRegValue, pVerify);
    }
    else if(dacName == "PtCut")
    {
        uint8_t cValue    = pCbc->getReg("Pipe&StubInpSel&Ptwidth");
        uint8_t cRegValue = (cValue & 0xF0) | dacValue;
        return WriteChipSingleReg(pCbc, "Pipe&StubInpSel&Ptwidth", cRegValue, pVerify);
    }
    else if(dacName == "EnableSLVS")
    {
        uint8_t cValue    = pCbc->getReg("HIP&TestMode");
        uint8_t cRegValue = (cValue & 0xFE) | !dacValue;
        // if(dacValue == 1)
        //     LOG(DEBUG) << BOLDBLUE << "Enabling SLVS output on CBCs by setting register to " << std::bitset<8>(cRegValue) << RESET;
        // else
        //     LOG(DEBUG) << BOLDBLUE << "Disabling SLVS output on CBCs by setting register to " << std::bitset<8>(cRegValue) << RESET;
        return WriteChipSingleReg(pCbc, "HIP&TestMode", cRegValue, pVerify);
    }
    else
    {
        if(dacValue > 255)
            LOG(ERROR) << "Error, DAC " << dacName << " for CBC3 can only be 8 bit max (255)!";
        else
            return WriteChipSingleReg(pCbc, dacName, dacValue, pVerify);
    }
    return false;
}

std::vector<std::pair<std::string, uint16_t>> CbcInterface::ReadChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& theRegisterList)
{
    setBoard(pChip->getBeBoardId());
    auto                     cRegMap = pChip->getRegMap();
    std::vector<ChipRegItem> cRegItemsPage0;
    std::vector<ChipRegItem> cRegItemsPage1;
    for(auto cReq: theRegisterList)
    {
        auto cIterator = cRegMap.find(cReq);
        if(cIterator == cRegMap.end())
        {
            LOG(ERROR) << BOLDRED << "CbcInterface::WriteChipMultReg trying to write to a register that doesn't exist in the map : " << cReq << RESET;
            abort();
        }

        ChipRegItem cItem = cIterator->second;
        if(cItem.fPage == 0)
            cRegItemsPage0.push_back(cItem);
        else
            cRegItemsPage1.push_back(cItem);
    }
    if(cRegItemsPage0.size() != 0)
    {
        ConfigurePage(pChip, 0);
        fBoardFW->MultiRegisterRead(pChip, cRegItemsPage0);
    }
    if(cRegItemsPage1.size() != 0)
    {
        ConfigurePage(pChip, 1);
        fBoardFW->MultiRegisterRead(pChip, cRegItemsPage1);
    }
    std::vector<std::pair<std::string, uint16_t>> theRegisterValues;
    // Now re order the read values as the order of the input vector theRegisterList
    for(auto cReq: theRegisterList)
    {
        auto        cIterator = cRegMap.find(cReq);
        ChipRegItem cItem     = cIterator->second;
        auto        matchItem = std::find(cRegItemsPage0.begin(), cRegItemsPage0.end(), cItem);
        if(matchItem != cRegItemsPage0.end())
            theRegisterValues.push_back(std::make_pair(cReq, matchItem->fValue));
        else
        {
            matchItem = std::find(cRegItemsPage1.begin(), cRegItemsPage1.end(), cItem);
            if(matchItem != cRegItemsPage1.end()) theRegisterValues.push_back(std::make_pair(cReq, matchItem->fValue));
        }
    }
    return theRegisterValues;
}

bool CbcInterface::ConfigurePage(Chip* pCbc, uint8_t pPage, bool pVerify)
{
    // only written for optical .. electrical readout the fw takes care of this
    bool cSuccess = !lpGBTFound();
    if(cSuccess) return cSuccess;

    cSuccess = true;
    setBoard(pCbc->getBeBoardId());
    std::string cRegName   = "FeCtrl&TrgLat2";
    ChipRegMap  cCbcRegMap = pCbc->getRegMap();
    // address in map depends on board id, hybrid id, chip id
    uint32_t cAddress = (pCbc->getBeBoardId() << 16) | (pCbc->getHybridId() << 8) | pCbc->getId();
    auto     cIter    = fPageMap.find(cAddress);
    if(cIter == fPageMap.end())
    {
        // auto        cValue = pCbc->getReg("FeCtrl&TrgLat2");
        ChipRegMask cMask;
        cMask.fBitShift      = 7;
        cMask.fNbits         = 1;
        uint8_t cDefaultPage = pCbc->getRegBits("FeCtrl&TrgLat2", cMask);
        // LOG(DEBUG) << BOLDYELLOW << "Default page on CBC" << +pCbc->getId() << " on hybrid " << +pCbc->getHybridId() << " is " << +cDefaultPage << " register value is 0x" << std::hex << +cValue
        //            << std::dec << RESET;
        fPageMap.insert(std::make_pair(cAddress, cDefaultPage));
        // LOG(DEBUG) << BOLDYELLOW << "Page was not explicitly selected on CBC#" << +pCbc->getId() << " setting to default value." << RESET;
        cSuccess = fBoardFW->SingleRegisterWrite(pCbc, cCbcRegMap.at(cRegName), pVerify);
    }
    if(!cSuccess)
    {
        LOG(ERROR) << BOLDRED << "Could not configure page register on CBC#" << +pCbc->getId() << RESET;
        return cSuccess;
    } // failed to write page register

    cIter         = fPageMap.find(cAddress);
    uint8_t cPage = cIter->second;
    if(cPage == pPage) { return true; } // don't need to do anything

    // switch page
    ChipRegItem cPageReg = pCbc->getRegItem("FeCtrl&TrgLat2");
    cPageReg.fValue      = (cPageReg.fValue & 0x7F) | (pPage << 7);
    // LOG(DEBUG) << BOLDBLUE << "Switching page on CBC#" << +pCbc->getId() << " on hybrid " << +pCbc->getHybridId() << " from page " << +cPage << " to page " << +pPage << "\t...Current page is "
    //            << cPage << " want to write to page " << +pPage << " need to update page register on the CBC" << RESET;
    // update page in map
    cIter->second = pPage;
    // write to page register in the CBC
    cSuccess = fBoardFW->SingleRegisterWrite(pCbc, cPageReg, pVerify);
    return cSuccess;
}
bool CbcInterface::WriteChipSingleReg(Chip* pCbc, const std::string& pRegNode, uint16_t pValue, bool pVerify)
{
    // first, identify the correct BeBoardFWInterface
    setBoard(pCbc->getBeBoardId());
    auto cRegMap                = pCbc->getRegMap();
    cRegMap.at(pRegNode).fValue = pValue;
    ConfigurePage(pCbc, cRegMap.at(pRegNode).fPage, pVerify);
    return fBoardFW->SingleRegisterWrite(pCbc, cRegMap.at(pRegNode), pVerify);
}
uint8_t CbcInterface::GetLastPage(Chip* pCbc)
{
    uint32_t cAddress = (pCbc->getBeBoardId() << 16) | (pCbc->getHybridId() << 8) | pCbc->getId();
    auto     cIter    = fPageMap.find(cAddress);
    if(cIter == fPageMap.end())
    {
        // auto        cValue = ReadChipSingleReg(pCbc, "FeCtrl&TrgLat2");
        ChipRegMask cMask;
        cMask.fBitShift      = 7;
        cMask.fNbits         = 1;
        uint8_t cDefaultPage = pCbc->getRegBits("FeCtrl&TrgLat2", cMask);
        // LOG(DEBUG) << BOLDMAGENTA << "\t...Default page on CBC" << +pCbc->getId() << " on hybrid " << +pCbc->getHybridId() << " is " << +cDefaultPage << " register value is 0x" << std::hex <<
        // +cValue
        //            << std::dec << RESET;
        return cDefaultPage;
    }
    else
        return cIter->second;
}
bool CbcInterface::WriteChipMultReg(Chip* pCbc, const std::vector<std::pair<std::string, uint16_t>>& pVecReq, bool pVerify)
{
    setBoard(pCbc->getBeBoardId());
    auto                     cRegMap = pCbc->getRegMap();
    std::vector<ChipRegItem> cRegItemsPg0;
    std::vector<ChipRegItem> cRegItemsPg1;
    for(auto cReq: pVecReq)
    {
        auto cIterator = cRegMap.find(cReq.first);
        if(cIterator == cRegMap.end())
        {
            LOG(ERROR) << BOLDRED << "CbcInterface::WriteChipMultReg trying to write to a register that doesn't exist in the map : " << cReq.first << RESET;
            continue;
        }
        if(cIterator->first.find("Fuse") != std::string::npos)
        {
            LOG(ERROR) << BOLDRED << "CbcInterface::WriteChipMultReg trtying to write to a FUSE register " << cReq.first << RESET;
            continue;
        }
        ChipRegItem cItem = cIterator->second;
        cItem.fValue      = cReq.second;
        if(cIterator->second.fPage == 0)
            cRegItemsPg0.push_back(cItem);
        else
            cRegItemsPg1.push_back(cItem);
    }
    if(cRegItemsPg0.size() > 0)
    {
        if(ConfigurePage(pCbc, 0, pVerify))
        {
            if(!fBoardFW->MultiRegisterWrite(pCbc, cRegItemsPg0, pVerify))
            {
                LOG(ERROR) << BOLDRED << "Coult not perform CbcInterface::WriteChipMultReg to Page0" << RESET;
                return false;
            }
        }
    }

    if(cRegItemsPg1.size() > 0)
    {
        if(ConfigurePage(pCbc, 1, pVerify))
        {
            if(!fBoardFW->MultiRegisterWrite(pCbc, cRegItemsPg1, pVerify))
            {
                LOG(ERROR) << BOLDRED << "Coult not perform CbcInterface::WriteChipMultReg to Page1" << RESET;
                return false;
            }
        }
    }

    return true;
}
bool CbcInterface::WriteChipAllLocalReg(ReadoutChip* pCbc, const std::string& dacName, const ChipContainer& localRegValues, bool pVerify)
{
    setBoard(pCbc->getBeBoardId());
    assert(localRegValues.size() == pCbc->getNumberOfChannels());
    std::string dacTemplate;
    bool        isMask = false;

    if(dacName == "ChannelOffset")
        dacTemplate = "Channel%03d";
    else if(dacName == "Mask")
        isMask = true;
    else
        LOG(ERROR) << "Error, DAC " << dacName << " is not a Local DAC";

    std::vector<std::pair<std::string, uint16_t>> cRegVec;
    ChannelGroup<1, NCHANNELS>                    channelToEnable;
    std::vector<uint32_t>                         cVec;
    cVec.clear();
    for(uint8_t iChannel = 0; iChannel < pCbc->getNumberOfChannels(); ++iChannel)
    {
        if(isMask)
        {
            if(localRegValues.getChannel<uint16_t>(0, iChannel)) { channelToEnable.enableChannel(0, iChannel); }
        }
        else
        {
            char dacName1[20];
            sprintf(dacName1, dacTemplate.c_str(), iChannel + 1);
            cRegVec.emplace_back(dacName1, localRegValues.getChannel<uint16_t>(0, iChannel));
        }
    }

    if(isMask) { return maskChannelGroup(pCbc, std::make_shared<ChannelGroup<1, NCHANNELS>>(std::move(channelToEnable)), pVerify); }
    else
        return WriteChipMultReg(pCbc, cRegVec, pVerify);
}
uint8_t CbcInterface::ReadChipSingleReg(Chip* pCbc, const std::string& pRegNode)
{
    setBoard(pCbc->getBeBoardId());
    ChipRegItem cRegItem = pCbc->getRegItem(pRegNode);
    ConfigurePage(pCbc, cRegItem.fPage);
    return fBoardFW->SingleRegisterRead(pCbc, cRegItem);
}
int32_t CbcInterface::ReadChipReg(Chip* pCbc, const std::string& pRegNode)
{
    ChipRegItem cRegItem;
    setBoard(pCbc->getBeBoardId());
    std::vector<uint32_t> cVecReq;
    if(pRegNode == "VCth" || pRegNode == "Threshold")
    {
        uint8_t  cReg0      = ReadChipSingleReg(pCbc, "VCth1");
        uint8_t  cReg1      = ReadChipSingleReg(pCbc, "VCth2");
        uint16_t cThreshold = ((cReg1 & 0x3) << 8) | cReg0;
        return cThreshold;
    }
    else if(pRegNode == "TestPulse" || pRegNode == "InjectedCharge")
    {
        uint8_t cRegValue = ReadChipSingleReg(pCbc, "MiscTestPulseCtrl&AnalogMux");
        return cRegValue;
    }
    else if(pRegNode == "HitLogic")
    {
        uint8_t cRegValue = ReadChipSingleReg(pCbc, "Pipe&StubInpSel&Ptwidth");
        return (cRegValue & 0xC0) >> 6;
    }
    else if(pRegNode == "StubLogic")
    {
        uint8_t cRegValue = ReadChipSingleReg(pCbc, "Pipe&StubInpSel&Ptwidth");
        return (cRegValue & 0x30) >> 4;
    }
    else if(pRegNode == "HitOr")
    {
        uint8_t cRegValue = ReadChipSingleReg(pCbc, "Pipe&StubInpSel&Ptwidth");
        return (cRegValue & 0x40) >> 6;
    }
    else if(pRegNode == "LayerSwap")
    {
        uint8_t cRegValue = ReadChipSingleReg(pCbc, "LayerSwap&CluWidth");
        return (cRegValue & 0x08) >> 3;
    }
    else if(pRegNode == "PtCut")
    {
        uint8_t cRegValue = ReadChipSingleReg(pCbc, "Pipe&StubInpSel&Ptwidth");
        return (cRegValue & 0x0F);
    }
    else if(pRegNode == "TriggerLatency")
    {
        auto     cRegValueFirst  = ReadChipSingleReg(pCbc, "FeCtrl&TrgLat2");
        auto     cRegValueSecond = ReadChipSingleReg(pCbc, "TriggerLatency1");
        uint16_t cLatency        = ((cRegValueFirst & 0x1) << 8) | cRegValueSecond;
        return cLatency;
    }
    else if(pRegNode == "TestPulse" || pRegNode == "InjectedCharge")
    {
        uint16_t cRegValue = ReadChipSingleReg(pCbc, "MiscTestPulseCtrl&AnalogMux");
        return (cRegValue & 0x40) >> 6;
    }
    else { return ReadChipSingleReg(pCbc, pRegNode) & 0xFF; }
}

void CbcInterface::produceL1phaseAlignmentPattern(ReadoutChip* pChip)
{
    // switch on HitOr
    WriteChipReg(pChip, "HitOr", 1);
    // set PtCut to maximum
    WriteChipReg(pChip, "PtCut", 14);
    // if I set this it doesn't work..   so no cluster cut
    WriteChipReg(pChip, "ClusterCut", 4);
    selectLogicMode(static_cast<ReadoutChip*>(pChip), "Sampled", true, true);
    WriteChipReg(pChip, "Threshold", 1023);

    auto cChannelMask = std::make_shared<ChannelGroup<1, NCHANNELS>>();
    cChannelMask->disableAllChannels();
    for(uint8_t cChannel = 0; cChannel < NCHANNELS; cChannel += 2) cChannelMask->enableChannel(0, cChannel); // generate a hit in every Nth channel
    this->maskChannelGroup(static_cast<ReadoutChip*>(pChip), cChannelMask);
}

void CbcInterface::produceStubLine0PhaseAlignmentPattern(ReadoutChip* pChip)
{
    // switch on HitOr
    WriteChipReg(pChip, "HitOr", 1);
    // set PtCut to maximum
    WriteChipReg(pChip, "PtCut", 14);
    // if I set this it doesn't work..   so no cluster cut
    WriteChipReg(pChip, "ClusterCut", 4);
    selectLogicMode(pChip, "Sampled", true, true);

    std::vector<std::pair<std::string, uint16_t>> theRegisterVector;
    theRegisterVector.push_back({"Bend7", 0x0A});              // forcing Bend7 to ouput the needed bend code for running the alignment
    theRegisterVector.push_back({"CoincWind&Offset12", 0x00}); // set stub window offset to 0
    theRegisterVector.push_back({"CoincWind&Offset34", 0x00}); // set stub window offset to 0
    WriteChipMultReg(pChip, theRegisterVector);
    // Also lines 1 and 2 are injected automatically
    // LOG(DEBUG) << BOLDBLUE << "Injecting on stub line 0 on CBC#" << +pChip->getId() << " on hybrid#" << +pChip->getHybridId() << RESET;
    std::vector<std::pair<uint8_t, int>> stubSeedAndBend{{0x55, 0}, {0xAA, 0}};
    injectStubs(pChip, stubSeedAndBend);
}

void CbcInterface::produceStubLines1To4PhaseAlignmentPattern(ReadoutChip* pChip)
{
    // switch on HitOr
    WriteChipReg(pChip, "HitOr", 1);
    // set PtCut to maximum
    WriteChipReg(pChip, "PtCut", 14);
    // if I set this it doesn't work..   so no cluster cut
    WriteChipReg(pChip, "ClusterCut", 4);
    selectLogicMode(pChip, "Sampled", true, true);

    std::vector<std::pair<std::string, uint16_t>> theRegisterVector;
    theRegisterVector.push_back({"Bend7", 0x0A});              // forcing Bend7 to ouput the needed bend code for running the alignment
    theRegisterVector.push_back({"CoincWind&Offset12", 0x00}); // set stub window offset to 0
    theRegisterVector.push_back({"CoincWind&Offset34", 0x00}); // set stub window offset to 0
    WriteChipMultReg(pChip, theRegisterVector);

    // LOG(DEBUG) << BOLDBLUE << "Injecting on stub lines 1,2,3 and 4 on CBC#" << +pChip->getId() << " on hybrid#" << +pChip->getHybridId() << RESET;
    std::vector<std::pair<uint8_t, int>> stubSeedAndBend{{0x2A, 0}, {0x55, 0}, {0xAA, 0}};
    injectStubs(pChip, stubSeedAndBend);
}

void CbcInterface::producePhaseAlignmentPattern(ReadoutChip* pChip, uint8_t pWait_ms)
{
    // LOG(DEBUG) << BOLDMAGENTA << "Producing phase alignment pattern on CBC#" << +pChip->getId() << RESET;
    // mask for L1A alignment
    auto cChannelMask = std::make_shared<ChannelGroup<1, NCHANNELS>>();
    cChannelMask->disableAllChannels();
    for(uint8_t cChannel = 0; cChannel < NCHANNELS; cChannel += 2) cChannelMask->enableChannel(0, cChannel); // generate a hit in every Nth channel

    // switch on HitOr
    WriteChipReg(pChip, "HitOr", 1);
    // set PtCut to maximum
    WriteChipReg(pChip, "PtCut", 14);
    // if I set this it doesn't work..   so no cluster cut
    WriteChipReg(pChip, "ClusterCut", 4);

    selectLogicMode(static_cast<ReadoutChip*>(pChip), "Sampled", true, true);

    uint8_t              cBendCode_phAlign = 0xa;
    std::vector<uint8_t> cBendLUT          = readLUT(static_cast<ReadoutChip*>(pChip));
    auto                 cIterator         = std::find(cBendLUT.begin(), cBendLUT.end(), cBendCode_phAlign);
    if(cIterator != cBendLUT.end())
    {
        int    cPosition    = std::distance(cBendLUT.begin(), cIterator);
        double cBend_strips = -7. + 0.5 * cPosition;
        // LOG(INFO) << BOLDBLUE << "Bend code of " << std::bitset<4>(cBendCode_phAlign) << " found for bend reg " << +cPosition << " which means " << cBend_strips << " strips." <<
        // RESET;

        // first pattern - stubs lines 0, 1 , 3
        // seeds on stub line 0 , stub line 1
        // bends on stub line 2
        // LOG(DEBUG) << BOLDBLUE << "Injecting on stub lines 0,1 and 2 on CBC#" << +pChip->getId() << " on hybrid#" << +pChip->getHybridId() << RESET;
        int                                  theBendValue = static_cast<int>(cBend_strips * 2);
        std::vector<std::pair<uint8_t, int>> stubBendAndValueFirstStep{{0x55, theBendValue}, {0xAA, theBendValue}};
        injectStubs(static_cast<ReadoutChip*>(pChip), stubBendAndValueFirstStep);
        std::this_thread::sleep_for(std::chrono::milliseconds(pWait_ms));

        // second pattern - 1, 2, 3 , 4
        // whatever on stub line 0
        // then alignment pattern on stub lines 1 + 2
        // LOG(DEBUG) << BOLDBLUE << "Injecting on stub lines 1,2,3 and 4 on CBC#" << +pChip->getId() << " on hybrid#" << +pChip->getHybridId() << RESET;
        std::vector<std::pair<uint8_t, int>> stubBendAndValueSecondStep{{42, theBendValue}, {0x55, theBendValue}, {0xAA, theBendValue}};
        injectStubs(static_cast<ReadoutChip*>(pChip), stubBendAndValueSecondStep);
        std::this_thread::sleep_for(std::chrono::milliseconds(pWait_ms));
    }
    else
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Bend code not available in the lookup table, aborting" << RESET;
        abort();
    }
    this->maskChannelGroup(static_cast<ReadoutChip*>(pChip), cChannelMask);
}
void CbcInterface::produceWordAlignmentPattern(ReadoutChip* pChip)
{
    // switch off HitOr
    WriteChipReg(pChip, "HitOr", 0);
    // set PtCut to maximum
    WriteChipReg(pChip, "PtCut", 14);
    // if I set this it doesn't work..   so no cluster cut
    WriteChipReg(pChip, "ClusterCut", 4);

    selectLogicMode(static_cast<ReadoutChip*>(pChip), "Sampled", true, true);

    std::vector<std::pair<std::string, uint16_t>> theRegisterVector;
    theRegisterVector.push_back({"Bend7", fWordAlignmentPatterns.at(3) & 0x0F});        // Set bend 0 to output of stub 1 required pattern
    theRegisterVector.push_back({"Bend8", (fWordAlignmentPatterns.at(3) & 0xF0) >> 4}); // Set bend 2 to output of stub 1 required pattern
    theRegisterVector.push_back({"Bend9", fWordAlignmentPatterns.at(4) & 0x0F});        // Set bend 4 to output of stub 1 required pattern
    theRegisterVector.push_back({"CoincWind&Offset12", 0x00});                          // set stub window offset to 0
    theRegisterVector.push_back({"CoincWind&Offset34", 0x00});                          // set stub window offset to 0

    WriteChipMultReg(pChip, theRegisterVector);
    std::vector<std::pair<uint8_t, int>> stubSeedAndBend{{fWordAlignmentPatterns.at(0), 0}, {fWordAlignmentPatterns.at(1), 2}, {fWordAlignmentPatterns.at(2), 4}};

    injectStubs(static_cast<ReadoutChip*>(pChip), stubSeedAndBend);
}

void CbcInterface::produceBX0AlignmentPattern(ReadoutChip* pChip)
{
    // switch off HitOr (OR254)
    WriteChipReg(pChip, "HitOr", 0);
    // Mask all channels (this way only the sync bit should be 1)
    //  MaskAllChannels(pChip, 1);

    // try stubs
    WriteChipReg(pChip, "PtCut", 14);
    WriteChipReg(pChip, "ClusterCut", 4);
    selectLogicMode(pChip, "OR", true, true);

    std::map<uint8_t, uint8_t>                    pBendingAndCode{{0, 0x9}, {2, 0xB}, {4, 0xF}};
    std::vector<std::pair<std::string, uint16_t>> theRegisterVector;
    theRegisterVector.push_back({"Bend7", pBendingAndCode.at(0)});
    theRegisterVector.push_back({"Bend8", pBendingAndCode.at(2)});
    theRegisterVector.push_back({"Bend9", pBendingAndCode.at(4)});
    theRegisterVector.push_back({"CoincWind&Offset12", 0x00}); // set stub window offset to 0
    theRegisterVector.push_back({"CoincWind&Offset34", 0x00}); // set stub window offset to 0
    WriteChipMultReg(pChip, theRegisterVector);
    // inject 3 stubs on CBC to CIC stub lines
    std::vector<std::pair<uint8_t, int>> stubSeedAndBend{{0x0A, 0}, {0x0A, 0}, {0x0A, 0}};
    injectStubs(pChip, stubSeedAndBend);
}
uint32_t CbcInterface::ReadChipFuseID(Chip* pCbc, uint8_t version)
{
    // make fuse read-able
    WriteChipReg(pCbc, "ChipIDFuse3", 8, false);
    uint8_t  IDa     = ReadChipReg(pCbc, "ChipIDFuse1");
    uint8_t  IDb     = ReadChipReg(pCbc, "ChipIDFuse2");
    uint8_t  IDc     = ReadChipReg(pCbc, "ChipIDFuse3");
    uint32_t IDeFuse = ((IDa) & 0x000000FF) + (((IDb) << 8) & 0x0000FF00) + (((IDc) << 16) & 0x000F0000);
    return IDeFuse;
}

} // namespace Ph2_HwInterface
