/*!

        \file                                            SSA2Interface.cc
        \brief                                           User Interface to the SSA2s
        \author                                          Marc Osherson
        \version                                         1.0
        \date                        Jan 2021
        Support :                    mail to : oshersonmarc@gmail.com

 */

#include "HWInterface/SSA2Interface.h"
#include "HWDescription/SSA2.h"
#include "Utils/ChannelGroupHandler.h"
#include "Utils/ConsoleColor.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/DataContainer.h"
#include <bitset>

using namespace Ph2_HwDescription;

#define DEV_FLAG 0
namespace Ph2_HwInterface
{ // start namespace
SSA2Interface::SSA2Interface(const BeBoardFWMap& pBoardMap) : ReadoutChipInterface(pBoardMap) {}
SSA2Interface::~SSA2Interface() {}
//	// CONFIGURE CHIP
//////////
void SSA2Interface::DumpConfiguration(Chip* pSSA2, std::string filename)
{
    std::ofstream myfile;
    myfile.open(filename);
    for(auto& cRegInMap: pSSA2->getRegMap())
    {
        uint16_t val = this->ReadChipReg(pSSA2, cRegInMap.first);
        LOG(INFO) << BLUE << cRegInMap.first << "    @0x" << std::hex << cRegInMap.second.fAddress << "    0x" << val << std::dec << RESET;
        myfile << cRegInMap.first << "    @0x" << std::hex << cRegInMap.second.fAddress << "    0x" << val << std::dec << "\n";
    }
    myfile.close();
}
bool SSA2Interface::ConfigureChip(Chip* pSSA2, bool pVerify, uint32_t pBlockSize)
{
    bool              cConfigLocalRegs = true;
    ChipRegMap        cSSA2RegMap      = pSSA2->getRegMap();
    std::stringstream cOutput;
    setBoard(pSSA2->getBeBoardId());
    pSSA2->printChipType(cOutput);
    LOG(INFO) << BOLDBLUE << cOutput.str() << "...Configuring chip with Id[" << +pSSA2->getId() << "] oh Hybrid" << +pSSA2->getHybridId() << RESET;

    // write mask registers
    std::vector<std::string> cMaskRegs{"peri_A", "peri_D", "strip"};
    std::vector<ChipRegItem> cRegItems;
    cRegItems.clear();
    for(auto cName: cMaskRegs)
    {
        auto cItem   = cSSA2RegMap.at("mask_" + cName);
        cItem.fValue = 0xFF;
        cRegItems.push_back(cItem);
    }
    fBoardFW->MultiRegisterWrite(pSSA2, cRegItems, false);

    cRegItems.clear();
    // need to split between control and enable registers
    // don't read back enable registers
    std::vector<ChipRegItem> cCntrlRegItems;

    ChipDataContainer theEnableFlagContainer;
    ContainerFactory::copyAndInitChannel<uint16_t>(*static_cast<ReadoutChip*>(pSSA2), theEnableFlagContainer);
    ChipDataContainer theStripControl2Container;
    ContainerFactory::copyAndInitChannel<uint16_t>(*static_cast<ReadoutChip*>(pSSA2), theStripControl2Container);
    ChipDataContainer theThTrimmingContainer;
    ContainerFactory::copyAndInitChannel<uint16_t>(*static_cast<ReadoutChip*>(pSSA2), theThTrimmingContainer);
    ChipDataContainer theDigCalibPatternLSBContainer;
    ContainerFactory::copyAndInitChannel<uint16_t>(*static_cast<ReadoutChip*>(pSSA2), theDigCalibPatternLSBContainer);
    ChipDataContainer theDigCalibPatternMSBContainer;
    ContainerFactory::copyAndInitChannel<uint16_t>(*static_cast<ReadoutChip*>(pSSA2), theDigCalibPatternMSBContainer);

    auto isLocalRegister = [](const std::string& theRegisterName) -> bool
    {
        std::vector<std::string> localRegisterMatches;
        localRegisterMatches.push_back("ENFLAGS_S");
        localRegisterMatches.push_back("StripControl2_S");
        localRegisterMatches.push_back("THTRIMMING_S");
        localRegisterMatches.push_back("DigCalibPattern_L_S");
        localRegisterMatches.push_back("DigCalibPattern_H_S");
        for(const auto& templ: localRegisterMatches)
            if(theRegisterName.find(templ) != std::string::npos) return true;
        return false;
    };

    auto extractStrip = [](const std::string& registerName) -> uint16_t
    {
        size_t posS = registerName.find("_S");

        if(posS == std::string::npos) { throw std::invalid_argument("Invalid format: missing '_S'"); }
        uint16_t strip = std::stoi(registerName.substr(posS + 2)) - 1;
        return strip;
    };

    cCntrlRegItems.clear();
    auto theListOfFreeRegisters = pSSA2->getFreeRegisters();
    for(auto cMapItem: cSSA2RegMap)
    {
        bool isFreeRegister = false;
        for(const auto& freeRegister: theListOfFreeRegisters)
        {
            isFreeRegister = std::regex_match(cMapItem.first, freeRegister.first);
            if(isFreeRegister) break;
        }
        if(isFreeRegister) continue; // skipping readonly registers

        if(cMapItem.second.fControlReg)
        {
            cCntrlRegItems.push_back(cMapItem.second);
            // cCntrlRegItemsNames.push_back(cMapItem.first);
        }
        else if(isLocalRegister(cMapItem.first))
        {
            uint16_t strip = extractStrip(cMapItem.first);
            if(cMapItem.first.find("ENFLAGS") != std::string::npos)
                theEnableFlagContainer.getChannel<uint16_t>(0, strip) = cMapItem.second.fValue;
            else if(cMapItem.first.find("StripControl2") != std::string::npos)
                theStripControl2Container.getChannel<uint16_t>(0, strip) = cMapItem.second.fValue;
            else if(cMapItem.first.find("THTRIMMING") != std::string::npos)
                theThTrimmingContainer.getChannel<uint16_t>(0, strip) = cMapItem.second.fValue;
            else if(cMapItem.first.find("DigCalibPattern_L") != std::string::npos)
                theDigCalibPatternLSBContainer.getChannel<uint16_t>(0, strip) = cMapItem.second.fValue;
            else if(cMapItem.first.find("DigCalibPattern_H") != std::string::npos)
                theDigCalibPatternMSBContainer.getChannel<uint16_t>(0, strip) = cMapItem.second.fValue;
            else
            {
                LOG(ERROR) << ERROR_FORMAT << "SSA2Interface::ConfigureChip - Local register " << cMapItem.first << " not recognized, throwing exception" << RESET;
                std::runtime_error("SSA2Interface::ConfigureChip - Local register not recognized");
            }
        }
        else { cRegItems.push_back(cMapItem.second); }
    }
    bool cSuccess = fBoardFW->MultiRegisterWrite(pSSA2, cCntrlRegItems, false);
    if(cSuccess) LOG(INFO) << BOLDGREEN << "Wrote " << cCntrlRegItems.size() << " control registers in SSA#" << +pSSA2->getId() << RESET;

    cSuccess &= fBoardFW->MultiRegisterWrite(pSSA2, cRegItems, pVerify);
    if(cSuccess) LOG(INFO) << BOLDGREEN << "Wrote " << cRegItems.size() << " global R/W registers in SSA#" << +pSSA2->getId() << RESET;

    if(cConfigLocalRegs)
    {
        std::vector<std::pair<std::string, uint16_t>> globalSettings;
        std::vector<std::pair<std::string, uint16_t>> localSettings;

        auto enflagsRegisterVector = packLocalRegisters(static_cast<ReadoutChip*>(pSSA2), "ENFLAGS", theEnableFlagContainer);
        globalSettings.push_back(enflagsRegisterVector.first);
        localSettings.insert(localSettings.end(), enflagsRegisterVector.second.begin(), enflagsRegisterVector.second.end());

        auto stripControl2RegisterVector = packLocalRegisters(static_cast<ReadoutChip*>(pSSA2), "StripControl2", theStripControl2Container);
        globalSettings.push_back(stripControl2RegisterVector.first);
        localSettings.insert(localSettings.end(), stripControl2RegisterVector.second.begin(), stripControl2RegisterVector.second.end());

        auto thTRimmingRegisterVector = packLocalRegisters(static_cast<ReadoutChip*>(pSSA2), "THTRIMMING", theThTrimmingContainer);
        globalSettings.push_back(thTRimmingRegisterVector.first);
        localSettings.insert(localSettings.end(), thTRimmingRegisterVector.second.begin(), thTRimmingRegisterVector.second.end());

        auto digCalibPatternLSBRegisterVector = packLocalRegisters(static_cast<ReadoutChip*>(pSSA2), "DigCalibPattern_L", theDigCalibPatternLSBContainer);
        globalSettings.push_back(digCalibPatternLSBRegisterVector.first);
        localSettings.insert(localSettings.end(), digCalibPatternLSBRegisterVector.second.begin(), digCalibPatternLSBRegisterVector.second.end());

        auto digCalibPatternMSBRegisterVector = packLocalRegisters(static_cast<ReadoutChip*>(pSSA2), "DigCalibPattern_H", theDigCalibPatternMSBContainer);
        globalSettings.push_back(digCalibPatternMSBRegisterVector.first);
        localSettings.insert(localSettings.end(), digCalibPatternMSBRegisterVector.second.begin(), digCalibPatternMSBRegisterVector.second.end());

        cSuccess &= WriteChipMultReg(pSSA2, globalSettings, false);
        cSuccess &= WriteChipMultReg(pSSA2, localSettings);
    }

    return cSuccess;
}

uint32_t SSA2Interface::ReadChipFuseID(Chip* pSSA2, uint8_t version)
{
    this->WriteChipReg(pSSA2, "Fuse_Mode", 0x0);
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    this->WriteChipReg(pSSA2, "Fuse_Mode", 0xF);
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    this->WriteChipReg(pSSA2, "Fuse_Mode", 0x0);

    uint32_t val = (this->ReadChipReg(pSSA2, "Fuse_Value_b3") << 24) | (this->ReadChipReg(pSSA2, "Fuse_Value_b2") << 16) | (this->ReadChipReg(pSSA2, "Fuse_Value_b1") << 8) |
                   (this->ReadChipReg(pSSA2, "Fuse_Value_b0") << 0);
    pSSA2->pChipFuseID.SetId(val);

    LOG(INFO) << GREEN << "FuseID from SSA2#" << +pSSA2->getId() << " Pos " << +pSSA2->pChipFuseID.Pos() << " Wafer " << +pSSA2->pChipFuseID.Wafer() << " Lot " << +pSSA2->pChipFuseID.Lot()
              << " Status " << +pSSA2->pChipFuseID.Status() << " Process " << +pSSA2->pChipFuseID.Process() << " ADCRef " << +pSSA2->pChipFuseID.ADCRef() << RESET;
    return val;
}

uint32_t SSA2Interface::readADC(Ph2_HwDescription::ReadoutChip* pChip, std::string pRegName, uint16_t numberOfRead)
{
    auto theRegister = SSA2_ADC_CONTROL_TABLE.find(pRegName);
    if(theRegister == SSA2_ADC_CONTROL_TABLE.end())
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " " << pRegName << " not found for this chip type - aborting." << RESET;
        abort();
    }
    // LOG(DEBUG) << BOLDMAGENTA << "For SSA " << +pChip->getId() << " converting " << pRegName << " to " << +theRegister->second << RESET;
    return SSA2Interface::ReadADC(pChip, theRegister->second);
}

uint32_t SSA2Interface::ReadADC(ReadoutChip* pChip, uint8_t pInput)
{
    setBoard(pChip->getBeBoardId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);

    std::vector<std::pair<std::string, uint16_t>> writeRegisters{{"ADC_control", 0xE0 | (pInput & 0x1F)}, {"ADC_control", 0xC0 | (pInput & 0x1F)}};
    WriteChipMultReg(pChip, writeRegisters, false);

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    std::vector<std::string> readRegisters{"ADC_out_H", "ADC_out_L"};
    auto                     readResults = ReadChipMultReg(pChip, readRegisters);

    return (readResults.at(0).second << 8) | (readResults.at(1).second);
}

uint32_t SSA2Interface::readADCGround(ReadoutChip* pSSA2)
{
    // LOG(DEBUG) << BOLDMAGENTA << "GND  " << +this->readADC(static_cast<ReadoutChip*>(pSSA2), "GND") << RESET;
    return this->readADC(static_cast<ReadoutChip*>(pSSA2), "GND");
}

uint32_t SSA2Interface::readADCBandGap(ReadoutChip* pSSA2)
{
    auto theBandGap = this->readADC(static_cast<ReadoutChip*>(pSSA2), "VBG");
    // LOG(DEBUG) << BOLDMAGENTA << "VBG  " << theBandGap << RESET;
    return theBandGap;
}

uint32_t SSA2Interface::readADCVref(ReadoutChip* pSSA2)
{
    uint8_t theVrefADC = readADC(pSSA2, "ADC_VREF");
    // LOG(DEBUG) << BOLDMAGENTA << "ADC_VREF  " << +theVrefADC << RESET;
    return theVrefADC;
}

uint32_t SSA2Interface::readVrefRegister(ReadoutChip* pSSA2)
{
    uint8_t theVrefADC = ReadChipReg(pSSA2, "ADC_VREF");
    LOG(INFO) << BOLDMAGENTA << "ADC_VREF  " << +theVrefADC << RESET;
    return theVrefADC;
}

bool SSA2Interface::setVrefFromFuseID(ReadoutChip* pSSA2)
{
    LOG(INFO) << BOLDMAGENTA << "Set Vref from value stored in fuse id " << +pSSA2->pChipFuseID.ADCRef() << RESET;
    return this->WriteChipReg(pSSA2, "ADC_VREF", pSSA2->pChipFuseID.ADCRef());
}
bool SSA2Interface::setVref(ReadoutChip* pSSA2, uint16_t theVrefRegisterValue)
{
    LOG(DEBUG) << BOLDMAGENTA << "Set Vref to desired value " << +theVrefRegisterValue << RESET;
    return this->WriteChipReg(pSSA2, "ADC_VREF", theVrefRegisterValue);
}

const std::map<std::string, std::pair<uint8_t, float>> SSA2Interface::getBiasStructureDefaultTable(Ph2_HwDescription::ReadoutChip* pSSA2) { return SSA2_BIAS_STRUCTURE_DEFAULT; }

// FIXME At the moment we are setting the exepected values
//  of bandgap and ADC_VREF to the default nominal value.
//  This will be updated once we have the real values for each chip
float SSA2Interface::getBandGapExpectedValue(Ph2_HwDescription::ReadoutChip* pMPA2) { return SSA2_VBG_EXPECTED; }
// FIXME At the moment we are setting the exepected values
//  of bandgap and ADC_VREF to the default nominal value.
//  This will be updated once we have the real values for each chip
float SSA2Interface::getVrefExpectedValue(Ph2_HwDescription::ReadoutChip* pSSA2) { return SSA2_VREF_EXPECTED; }
// FIXME At the moment we are setting the exepected values
//  of bandgap and ADC_VREF to the default nominal value.
//  This will be updated once we have the real values for each chip
float SSA2Interface::getVrefPrecision(Ph2_HwDescription::ReadoutChip* pSSA2) { return SSA2_ADC_PRECISION; }
// FIXME At the moment we are setting the exepected values
//  of bandgap and ADC_VREF to the default nominal value.
//  This will be updated once we have the real values for each chip
float SSA2Interface::getVrefMinValue(Ph2_HwDescription::ReadoutChip* pSSA2) { return SSA2_VREF_MIN; }
// FIXME At the moment we are setting the exepected values
//  of bandgap and ADC_VREF to the default nominal value.
//  This will be updated once we have the real values for each chip
float SSA2Interface::getVrefMaxValue(Ph2_HwDescription::ReadoutChip* pSSA2) { return SSA2_VREF_MAX; }

bool SSA2Interface::disableTestPadsOutput(ReadoutChip* pSSA2)
{
    LOG(DEBUG) << BOLDMAGENTA << "Disable all SSA test pads outputs... " << RESET;
    bool success = this->WriteChipReg(pSSA2, "Bias_TEST_lsb", 0x0);
    return success & this->WriteChipReg(pSSA2, "Bias_TEST_msb", 0x0);
}

float SSA2Interface::calculateADCLSB(ReadoutChip* pSSA2, float theVrefValue)
{
    float offset = this->readADCGround(pSSA2);

    LOG(DEBUG) << BOLDMAGENTA << "ADCLSB " << theVrefValue / (4095.0 - offset) << RESET;
    return theVrefValue / (4095.0 - offset);
}

// READ REGISTER ON CHIP:
int32_t SSA2Interface::ReadChipReg(Chip* pSSA2, const std::string& pRegNode)
{
    setBoard(pSSA2->getBeBoardId());
    auto                     cRegMap = pSSA2->getRegMap();
    std::vector<ChipRegItem> cRegItems;
    ChipRegItem              cRegItem;
    if(pRegNode.find("CounterStrip") != std::string::npos)
    {
        int cChannel = 0;
        sscanf(pRegNode.c_str(), "CounterStrip%d", &cChannel);
        cRegItem.fPage    = 0x00;
        cRegItem.fAddress = 0x500 + cChannel; // MSB
        cRegItem.fValue   = 0;
        cRegItems.push_back(cRegItem);
        cRegItem.fAddress = 0x600 + cChannel;
        cRegItems.push_back(cRegItem); // LSB
    }
    else if(pRegNode == "ChipId") { return this->ReadChipId(pSSA2); }
    else if(pRegNode == "Threshold" || pRegNode == "Bias_THDAC")
    {
        // LOG(DEBUG) << BOLDYELLOW << "Adding thrshld register to multi-reg read..." << RESET;
        cRegItem = cRegMap.at("Bias_THDAC");
        cRegItems.push_back(cRegItem);
    }
    else if(pRegNode == "ThresholdHigh" || pRegNode == "Bias_THDACHIGH")
    {
        // LOG(DEBUG) << BOLDYELLOW << "Adding thrshld high register to multi-reg read..." << RESET;
        cRegItem = cRegMap.at("Bias_THDACHIGH");
        cRegItems.push_back(cRegItem);
    }
    else
    {
        cRegItem = cRegMap.at(pRegNode);
        cRegItems.push_back(cRegItem);
    }

    // THE REGISTERS BELOW NEED TO BE READ BEFORE MultiRegisterRead OTHERWISE THEY ARE GETTING MESSED UP BECAUSE YOU CAN'T READ THEM DIRECTLY
    if(pRegNode == "SamplingMode_ALL")
    {
        uint16_t cMode = this->ReadChipReg(pSSA2, "ENFLAGS_S1");
        cMode          = (cMode & 0x60) >> 5;
        return cMode;
    }
    else if(pRegNode == "ENFLAGS")
    {
        // LOG(DEBUG) << BOLDGREEN << __LINE__ << "] ReadChipReg(): " << pRegNode << RESET;
        uint16_t cMode = this->ReadChipReg(pSSA2, "ENFLAGS_S1");
        return cMode;
    }
    else if(pRegNode == "StripControl2")
    {
        // LOG(DEBUG) << BOLDGREEN << __LINE__ << "] ReadChipReg(): " << pRegNode << RESET;
        uint16_t cMode = this->ReadChipReg(pSSA2, "StripControl2_S1");
        return cMode;
    }
    else if(pRegNode == "InjectedCharge" || pRegNode == "Bias_CALDAC")
    {
        auto cRegItem = cRegMap.at("Bias_CALDAC");
        return fBoardFW->SingleRegisterRead(pSSA2, cRegItem);
    }

    auto cValues = fBoardFW->MultiRegisterRead(pSSA2, cRegItems);
    if(pRegNode.find("CounterStrip") != std::string::npos) { return (cValues.at(0) << 8) | cValues.at(1); }
    else if(pRegNode == "TriggerLatency")
    {
        uint8_t cLatencyReg1 = this->ReadChipReg(pSSA2, "control_1");
        cLatencyReg1         = cLatencyReg1 & (0x10); // fourth bit is latency for SSA2
        uint8_t cLatencyReg2 = this->ReadChipReg(pSSA2, "control_3");
        return +((cLatencyReg1 << 4) | cLatencyReg2);
    }
    else
        return cValues.at(0);
}
// READ CHIP ID:
// FIX-ME
uint8_t SSA2Interface::ReadChipId(Chip* pChip)
{
    // set board
    setBoard(pChip->getBeBoardId());
    auto cRegMap = pChip->getRegMap();
    // ask SSA team how to read chip id
    auto cItem   = cRegMap.at("Fuse_Mode");
    cItem.fValue = 0x0F;
    return cItem.fValue;
}

std::pair<std::pair<std::string, uint16_t>, std::vector<std::pair<std::string, uint16_t>>>
SSA2Interface::packLocalRegisters(ReadoutChip* pChip, const std::string& dacName, const ChipContainer& localRegValues)
{
    std::string localDacName = dacName;
    if(dacName == "ThresholdTrim") localDacName = "THTRIMMING";

    // check that you are actually configuring all local registers
    assert(localRegValues.size() == pChip->getNumberOfChannels());

    uint16_t theMostFrequentValue = getMostFrequentLocalRegisterValue(localRegValues);

    std::pair<std::pair<std::string, uint16_t>, std::vector<std::pair<std::string, uint16_t>>> theListOfLocalRegisters;
    theListOfLocalRegisters.first = {localDacName, theMostFrequentValue};
    for(size_t strip = 0; strip < pChip->getNumberOfCols(); ++strip)
    {
        uint16_t theValue = localRegValues.getChannel<uint16_t>(0, strip);
        if(theValue == theMostFrequentValue) continue;
        theListOfLocalRegisters.second.push_back({static_cast<SSA2*>(pChip)->getStripRegisterName(localDacName, strip), theValue});
    }

    return theListOfLocalRegisters;
}

bool SSA2Interface::WriteChipAllLocalReg(ReadoutChip* pChip, const std::string& dacName, const ChipContainer& localRegValues, bool pVerify)
{
    auto theListOfLocalRegisters = packLocalRegisters(pChip, dacName, localRegValues);
    bool success                 = WriteChipReg(pChip, theListOfLocalRegisters.first.first, theListOfLocalRegisters.first.second, false);
    success &= WriteChipMultReg(pChip, theListOfLocalRegisters.second, pVerify);
    return success;
}

bool SSA2Interface::WriteChipMultReg(Chip* pSSA2, const std::vector<std::pair<std::string, uint16_t>>& pVecReq, bool pVerify)
{
    setBoard(pSSA2->getBeBoardId());
    auto                     cRegMap = pSSA2->getRegMap();
    std::vector<ChipRegItem> cRegItems;
    for(auto cReq: pVecReq)
    {
        auto cIterator = cRegMap.find(cReq.first);
        if(cIterator == cRegMap.end())
        {
            LOG(ERROR) << BOLDRED << "SSA2Interaface::WriteChipMultReg trying to write to a register that doesn't exist in the map : " << cReq.first << RESET;
            continue;
        }

        ChipRegItem cItem = cIterator->second;
        cItem.fValue      = cReq.second;
        cRegItems.push_back(cItem);
    }
    return fBoardFW->MultiRegisterWrite(pSSA2, cRegItems, pVerify);
}

bool SSA2Interface::WriteChipRegBits(Chip* theSSA, const std::string& pRegNode, uint16_t pValue, const std::string& pMaskReg, uint8_t mask, bool pVerify)
{
    setBoard(theSSA->getBeBoardId());
    uint16_t registerValue = theSSA->getReg(pRegNode);

    // Preserve the original register values changing only the needed bits
    registerValue = (registerValue & ~mask) + pValue;

    std::vector<std::pair<std::string, uint16_t>> registerVector;
    registerVector.push_back({pMaskReg, mask});
    registerVector.push_back({pRegNode, pValue});
    registerVector.push_back({pMaskReg, 0xFF});
    bool cSuccess = WriteChipMultReg(theSSA, registerVector, pVerify);

    theSSA->setReg(pRegNode, registerValue);

    return cSuccess;
}

bool SSA2Interface::WriteChipRegBitsLocal(Chip* pSSA2, const std::string& pRegNode, uint16_t pValue, const std::string& pMaskReg, uint8_t mask, bool pVerify)
{
    setBoard(pSSA2->getBeBoardId());

    uint16_t registerValue = pSSA2->getReg(pRegNode);
    unsigned posOfFirstOne = 0;
    // ASSUMING THAT MASK BITS ARE ALWAYS CONSECUTIVE. CANNOT BE MASK 0b101 BUT ONLY WORKS FOR 0b11000
    for(; posOfFirstOne < 8; posOfFirstOne++) // 8bits
    {
        if((mask & (1 << posOfFirstOne))) break;
    }
    // Preserve the original register values changing only the needed bits
    registerValue = (registerValue & ~mask) + (pValue << posOfFirstOne);

    std::vector<std::pair<std::string, uint16_t>> registerVector;
    registerVector.push_back({pMaskReg, mask});
    registerVector.push_back({pRegNode, (pValue << posOfFirstOne)});
    registerVector.push_back({pMaskReg, 0xFF});
    bool cSuccess = WriteChipMultReg(pSSA2, registerVector, pVerify);

    pSSA2->setReg(pRegNode, registerValue);

    return cSuccess;
}
//	// WRITE REGISTER (SINGLE CHIP REG <<main interface for writing>>):
//////////
bool SSA2Interface::WriteChipReg(Chip* pSSA2, const std::string& pRegName, uint16_t pValue, bool pVerify)
{
    setBoard(pSSA2->getBeBoardId());
    // LOG (DEBUG) << BOLDYELLOW << "SSA2Interface::WriteChipReg  writing to " << pRegName << RESET;
    auto        cRegMap = pSSA2->getRegMap();
    ChipRegItem cRegItem;

    std::string pRegNameMod = pRegName;
    if(pRegName.find("_ALL") != std::string::npos) { pRegNameMod.erase(pRegName.length() - 4); }

    if(pRegNameMod == "CountingMode")
    {
        cRegItem        = cRegMap.at("ENFLAGS");
        cRegItem.fValue = (pValue << 2) | (1 << 0);
        return fBoardFW->SingleRegisterWrite(pSSA2, cRegItem, pVerify);
    }
    // else if(pRegNameMod == "AmuxHigh")
    // {
    //     LOG(ERROR) << BOLDRED << "SSA2 Register " << BOLDYELLOW << pRegNameMod << BOLDRED << " has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal " << RESET;
    //     throw Exception("SSA2 Register has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal");

    //     return this->ConfigureAmux(pSSA2, "HighZ");
    // }
    // need to re-name threshold here..
    // else if(fAmuxMap.find(pRegNameMod) != fAmuxMap.end())
    // {
    //     return this->ConfigureAmux(pSSA2, pRegNameMod);
    // }
    // else if(pRegNameMod == "MonitorBandgap")
    // {
    //     LOG(ERROR) << BOLDRED << "SSA2 Register " << BOLDYELLOW << pRegNameMod << BOLDRED << " has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal " << RESET;
    //     throw Exception("SSA2 Register has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal");

    //     return this->ConfigureAmux(pSSA2, "Bandgap");
    // }
    // else if(pRegNameMod == "MonitorGround") { return this->ConfigureAmux(pSSA2, "GND"); }
    else if(pRegNameMod == "ReadoutMode") // AT THE TOP OF THIS METHOD _ALL IS REMOVED
    {
        return this->WriteChipRegBitsLocal(pSSA2, "control_1", pValue & 0x07, "mask_peri_D", 0x07);
    }
    else if(pRegNameMod == "SamplingMode") // AT THE TOP OF THIS METHOD _ALL IS REMOVED
    {
        bool retVal = this->WriteChipRegBitsLocal(pSSA2, "ENFLAGS", pValue, "mask_strip", 0x60);
        return retVal;
    }
    else if(pRegNameMod == "TriggerLatency")
    {
        bool cSuccess;
        cSuccess = this->WriteChipRegBitsLocal(pSSA2, "control_1", ((pValue & 0x100) >> 8), "mask_peri_D", 0x10);
        cSuccess &= this->WriteChipRegBitsLocal(pSSA2, "control_3", pValue & 0xFF, "mask_peri_D", 0xFF);
        return cSuccess;
    }
    else if(pRegName == "SamplePhaseShift")
    {
        uint8_t cBitShift = 0;
        uint8_t cRegMask  = (0xF << cBitShift); //

        LOG(ERROR) << BOLDRED << "SSA2 Register " << BOLDYELLOW << pRegNameMod << BOLDRED << " has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal " << RESET;
        throw Exception("SSA2 Register has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal");

        return this->WriteChipRegBitsLocal(pSSA2, "ClockDeskewing_fine", (pValue << cBitShift), "mask_peri_D", cRegMask);
    }
    else if(pRegName == "PhaseShift")
    {
        uint8_t cBitShift = 0;
        uint8_t cRegMask  = (0x7 << cBitShift); //

        LOG(ERROR) << BOLDRED << "SSA2 Register " << BOLDYELLOW << pRegNameMod << BOLDRED << " has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal " << RESET;
        throw Exception("SSA2 Register has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal");

        return this->WriteChipRegBitsLocal(pSSA2, "ClockDeskewing_coarse", (pValue << cBitShift), "mask_peri_D", cRegMask);
    }
    else if(pRegNameMod == "AsyncDelay")
    {
        uint8_t                                       cLSB = pValue & 0xFF;
        uint8_t                                       cMSB = (pValue >> 8);
        std::vector<std::pair<std::string, uint16_t>> registerVector;
        registerVector.push_back({"AsyncRead_StartDel_LSB", cLSB});
        registerVector.push_back({"AsyncRead_StartDel_MSB", cMSB});
        return WriteChipMultReg(pSSA2, registerVector, pVerify);
    }
    else if(pRegNameMod == "AnalogueSync")
    {
        uint8_t cReadoutMode = 0x0;
        // readout mode
        bool cSuccess = this->WriteChipRegBitsLocal(pSSA2, "control_1", cReadoutMode, "mask_peri_D", 0x7, pVerify);
        // duration
        uint8_t cDuration = 0x8;
        cSuccess          = cSuccess && this->WriteChipRegBitsLocal(pSSA2, "control_2", cDuration, "mask_peri_D", (0xF << 4));

        // sampling mode
        // sampling mode
        uint8_t cSamplingMode = 0;
        cSuccess              = cSuccess && this->WriteChipRegBitsLocal(pSSA2, "ENFLAGS", (cSamplingMode << 5), "mask_strip", (0x3 << 5));
        cRegItem              = cRegMap.at("ENFLAGS_S1");
        // auto cRegValue        = fBoardFW->SingleRegisterRead(pSSA2, cRegItem);
        // LOG(DEBUG) << BOLDBLUE << "[post-set sampling] StripControl1 set to 0x" << std::hex << cRegValue << std::dec << RESET;
        // configure for injection with the strip register
        uint8_t cMask         = 1;
        uint8_t cPolarity     = 0;
        uint8_t cHitCounter   = 0;
        uint8_t cDigitalCalib = 0;
        uint8_t cAnalogCalib  = pValue;
        uint8_t cEnFlags      = (cAnalogCalib << 4 | cDigitalCalib << 3 | cHitCounter << 2 | cPolarity << 1 | cMask);
        cSuccess              = cSuccess && this->WriteChipRegBitsLocal(pSSA2, "ENFLAGS", cEnFlags, "mask_strip", 0x1F);
        // cRegValue             = fBoardFW->SingleRegisterRead(pSSA2, cRegItem);
        // LOG(DEBUG) << BOLDBLUE << "StripControl1 set to 0x" << std::hex << cRegValue << std::dec << RESET;
        return cSuccess;
    }
    else if(pRegNameMod == "AnalogueAsync")
    {
        uint8_t cReadoutMode = 0x1;
        // readout mode
        bool cSuccess = this->WriteChipRegBitsLocal(pSSA2, "control_1", cReadoutMode, "mask_peri_D", 0x07);
        // duration
        uint8_t cDuration = 0x8;
        cSuccess          = cSuccess && this->WriteChipRegBitsLocal(pSSA2, "control_2", cDuration, "mask_peri_D", 0xF0);

        // configure for injection with the strip register
        uint8_t cMask         = 1;
        uint8_t cPolarity     = 0;
        uint8_t cHitCounter   = 1;
        uint8_t cDigitalCalib = 0;
        uint8_t cAnalogCalib  = pValue;
        uint8_t cEnFlags      = (cAnalogCalib << 4 | cDigitalCalib << 3 | cHitCounter << 2 | cPolarity << 1 | cMask);
        cSuccess              = cSuccess && this->WriteChipRegBitsLocal(pSSA2, "ENFLAGS", cEnFlags, "mask_strip", 0x1F);
        return cSuccess;
    }
    else if(pRegNameMod == "Sync")
    {
        uint8_t cMask         = 1;
        uint8_t cPolarity     = 0;
        uint8_t cHitCounter   = 0;
        uint8_t cDigitalCalib = 1;
        uint8_t cAnalogCalib  = 1;
        uint8_t cEnFlags      = (cAnalogCalib << 4 | cDigitalCalib << 3 | cHitCounter << 2 | cPolarity << 1 | cMask);
        bool    cSuccess      = this->WriteChipRegBitsLocal(pSSA2, "ENFLAGS", cEnFlags, "mask_strip", 0x1F);
        LOG(ERROR) << BOLDRED << "SSA2 Register " << BOLDYELLOW << pRegNameMod << BOLDRED << " has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal " << RESET;
        throw Exception("SSA2 Register has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal");

        return cSuccess && this->WriteChipRegBitsLocal(pSSA2, "control_1", (1 - pValue), "mask_peri_D", 0x7);
    }
    else if(pRegNameMod == "DigitalAsync")
    {
        uint8_t cReadoutMode = 0x1;
        uint8_t cEdgeSel_T1  = 0x0;
        // readout mode
        bool cSuccess = this->WriteChipRegBitsLocal(pSSA2, "control_1", cReadoutMode, "mask_peri_D", 0x7);
        // edge select
        cSuccess = cSuccess && this->WriteChipRegBitsLocal(pSSA2, "control_1", cEdgeSel_T1, "mask_peri_D", (0x1 << 3));
        // duration
        uint8_t cDuration = 0x8;
        cSuccess          = cSuccess && this->WriteChipRegBitsLocal(pSSA2, "control_2", cDuration, "mask_peri_D", (0xF << 4));

        // sampling mode
        // sampling mode
        uint8_t cSamplingMode = 0;
        cSuccess              = cSuccess && this->WriteChipRegBitsLocal(pSSA2, "ENFLAGS", (cSamplingMode << 5), "mask_strip", (0x3 << 5));
        cRegItem              = cRegMap.at("ENFLAGS_S1");
        // auto cRegValue        = fBoardFW->SingleRegisterRead(pSSA2, cRegItem);
        // LOG(DEBUG) << BOLDBLUE << "[post-set sampling] StripControl1 set to 0x" << std::hex << cRegValue << std::dec << RESET;
        // configure for injection with the strip register
        uint8_t cMask         = 0;
        uint8_t cPolarity     = 0;
        uint8_t cHitCounter   = 0;
        uint8_t cDigitalCalib = 1;
        uint8_t cAnalogCalib  = pValue;
        uint8_t cEnFlags      = (cAnalogCalib << 4 | cDigitalCalib << 3 | cHitCounter << 2 | cPolarity << 1 | cMask);
        LOG(ERROR) << BOLDRED << "SSA2 Register " << BOLDYELLOW << pRegNameMod << BOLDRED << " has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal " << RESET;
        throw Exception("SSA2 Register has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal");

        cSuccess = cSuccess && this->WriteChipRegBitsLocal(pSSA2, "ENFLAGS", cEnFlags, "mask_strip", 0x1F);

        return cSuccess;
    }
    else if(pRegNameMod == "DigitalSync")
    {
        this->WriteChipReg(pSSA2, "ReadoutMode", 0);
        // configure for injection with the strip register
        uint8_t cMask         = 0;
        uint8_t cPolarity     = 0;
        uint8_t cHitCounter   = 0;
        uint8_t cDigitalCalib = pValue;
        uint8_t cAnalogCalib  = 0;
        uint8_t cEnFlags      = (cAnalogCalib << 4 | cDigitalCalib << 3 | cHitCounter << 2 | cPolarity << 1 | cMask);

        LOG(ERROR) << BOLDRED << "SSA2 Register " << BOLDYELLOW << pRegNameMod << BOLDRED << " has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal " << RESET;
        throw Exception("SSA2 Register has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal");

        bool cSuccess  = this->WriteChipRegBitsLocal(pSSA2, "ENFLAGS", cEnFlags, "mask_strip", 0x1F);
        auto cRegItem  = cRegMap.at("ENFLAGS_S1");
        auto cRegValue = fBoardFW->SingleRegisterRead(pSSA2, cRegItem);
        LOG(INFO) << BOLDYELLOW << "ENFLAGS_S1 set to 0x" << std::hex << +cRegValue << std::dec << RESET;

        return cSuccess;
    }
    else if(pRegNameMod == "PulseDuration" || pRegNameMod == "DigitalDuration")
    {
        // edge select
        return this->WriteChipRegBitsLocal(pSSA2, "control_2", pValue, "mask_peri_D", 0xF0);
        // return this->WriteChipRegBitsLocal(pSSA2, "control_2", (pValue << 4), "mask_peri_D", (0xF << 4));
    }
    else if(pRegNameMod == "EdgeSel")
    {
        // edge select
        return this->WriteChipRegBitsLocal(pSSA2, "control_1", pValue, "mask_peri_D", 0x8);
        // return this->WriteChipRegBitsLocal(pSSA2, "control_1", (pValue << 3), "mask_peri_D", (0x1 << 3));
    }
    else if(pRegNameMod.find("DigitalSync") != std::string::npos)
    {
        this->WriteChipReg(pSSA2, "ReadoutMode", 0);

        int               cStripId = 0;
        std::stringstream cRegName;
        std::sscanf(pRegNameMod.c_str(), "DigitalSync_S%d", &cStripId);
        cRegName << "ENFLAGS_S" << (cStripId);
        // LOG (INFO) << BOLDYELLOW << "Digital injection on Strip#" << +cStripId << "\t" << cRegName.str() << RESET;

        // configure for injection with the strip register
        uint8_t cMask         = 0;
        uint8_t cPolarity     = 0;
        uint8_t cHitCounter   = 0;
        uint8_t cDigitalCalib = pValue;
        uint8_t cAnalogCalib  = 0;
        uint8_t cEnFlags      = (cAnalogCalib << 4 | cDigitalCalib << 3 | cHitCounter << 2 | cPolarity << 1 | cMask);
        bool    cSuccess      = this->WriteChipRegBitsLocal(pSSA2, cRegName.str(), cEnFlags, "mask_strip", 0x1F);

        auto cRegItem  = pSSA2->getRegItem(cRegName.str());
        auto cRegValue = fBoardFW->SingleRegisterRead(pSSA2, cRegItem);
        LOG(INFO) << BOLDYELLOW << cRegName.str() << " set to 0x" << std::hex << +cRegValue << std::dec << RESET;

        LOG(ERROR) << BOLDRED << "SSA2 Register " << BOLDYELLOW << pRegNameMod << BOLDRED << " has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal " << RESET;
        throw Exception("SSA2 Register has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal");

        return cSuccess;
    }
    else if(pRegNameMod == "EnableSLVSTestOutput")
    {
        LOG(INFO) << BOLDBLUE << "Enabling SLVS test output on SSA2#" << +pSSA2->getId() << RESET;
        // return this->WriteChipRegBitsLocal(pSSA2, "control_1", pValue << 1, "mask_peri_D", 0x2);
        return this->WriteChipRegBitsLocal(pSSA2, "control_1", pValue, "mask_peri_D", 0x2);
    }
    else if(pRegNameMod.find("OutPatternStubLine") != std::string::npos) // Stub Lines
    {
        int cLine;
        std::sscanf(pRegNameMod.c_str(), "OutPatternStubLine%d", &cLine);
        std::vector<std::string> cRegNames{"Shift_pattern_st_0",
                                           "Shift_pattern_st_1",
                                           "Shift_pattern_st_2",
                                           "Shift_pattern_st_3",
                                           "Shift_pattern_st_4_st_5",
                                           "Shift_pattern_st_4_st_5",
                                           "Shift_pattern_st_6_st_7",
                                           "Shift_pattern_st_6_st_7"};
        cRegItem        = cRegMap.at(cRegNames.at(cLine));
        cRegItem.fValue = pValue;
        return fBoardFW->SingleRegisterWrite(pSSA2, cRegItem, pVerify);
    }
    else if(pRegNameMod.find("OutPatternL1Line") != std::string::npos) // Stub Lines
    {
        cRegItem        = cRegMap.at("Shift_pattern_L1");
        cRegItem.fValue = pValue;
        return fBoardFW->SingleRegisterWrite(pSSA2, cRegItem, pVerify);
        // return this->WriteChipSingleReg(pSSA2, "Shift_pattern_L1", pValue, pVerify);
    }
    else if(pRegNameMod == "CalibrationPattern")
    {
        // configure for injection with the strip register
        uint8_t cMask         = 0;
        uint8_t cPolarity     = 0;
        uint8_t cHitCounter   = 0;
        uint8_t cDigitalCalib = 1;
        uint8_t cAnalogCalib  = pValue;
        uint8_t cEnFlags      = (cAnalogCalib << 4 | cDigitalCalib << 3 | cHitCounter << 2 | cPolarity << 1 | cMask);

        LOG(ERROR) << BOLDRED << "SSA2 Register " << BOLDYELLOW << pRegNameMod << BOLDRED << " has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal " << RESET;
        throw Exception("SSA2 Register has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal");

        bool cSuccess = this->WriteChipRegBitsLocal(pSSA2, "ENFLAGS", cEnFlags, "mask_strip", 0x1F);
        // uint8_t cRegValue = fBoardFW->SingleRegisterRead(pSSA2, cRegItem);
        // LOG(DEBUG) << BOLDYELLOW << "Strip register set to 0x" << std::hex << +cRegValue << std::dec << RESET;

        cRegItem        = cRegMap.at("DigCalibPattern_L");
        cRegItem.fValue = pValue;
        return cSuccess && fBoardFW->SingleRegisterWrite(pSSA2, cRegItem, pVerify);
    }
    else if(pRegNameMod.find("CalibrationPattern") != std::string::npos)
    {
        int cChannel;
        std::sscanf(pRegNameMod.c_str(), "CalibrationPatternS%d", &cChannel);
        // uint16_t cAddress = 0x0300 + cChannel;
        // LOG(DEBUG) << BOLDBLUE << "Configuring register 0x" << std::hex << cAddress << std::dec << " to 0x" << std::hex << pValue << std::dec << " for channel " << +cChannel << RESET;

        // configure for injection with the strip register
        uint8_t cMask         = 0;
        uint8_t cPolarity     = 0;
        uint8_t cHitCounter   = 0;
        uint8_t cDigitalCalib = 1;
        uint8_t cAnalogCalib  = pValue;
        uint8_t cEnFlags      = (cAnalogCalib << 4 | cDigitalCalib << 3 | cHitCounter << 2 | cPolarity << 1 | cMask);

        LOG(ERROR) << BOLDRED << "SSA2 Register " << BOLDYELLOW << pRegNameMod << BOLDRED << " has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal " << RESET;
        throw Exception("SSA2 Register has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal");

        bool cSuccess = this->WriteChipRegBitsLocal(pSSA2, "ENFLAGS", cEnFlags, "mask_strip", 0x1F);
        // auto cRegValue = fBoardFW->SingleRegisterRead(pSSA2, cRegItem);
        // LOG(DEBUG) << BOLDYELLOW << "Strip register set to 0x" << std::hex << +cRegValue << std::dec << RESET;

        cRegItem.fAddress = 0x0300 + cChannel;
        cRegItem.fValue   = pValue;
        return cSuccess && fBoardFW->SingleRegisterWrite(pSSA2, cRegItem, pVerify);
    }
    else if(pRegNameMod == "InjectedCharge" || pRegNameMod == "Bias_CALDAC")
    {
        // LOG(DEBUG) << BOLDBLUE << "Setting "
        //            << " bias calDac to " << +pValue << " on SSA2#" << +pSSA2->getId() << RESET;
        cRegItem        = cRegMap.at("Bias_CALDAC");
        cRegItem.fValue = pValue;
        return fBoardFW->SingleRegisterWrite(pSSA2, cRegItem, pVerify);
    }
    else if(pRegNameMod == "Threshold" || pRegNameMod == "Bias_THDAC")
    {
        // LOG(DEBUG) << BOLDYELLOW << "!!! Writing to " << pRegNameMod << "," << pValue << RESET;
        return this->WriteChipRegBitsLocal(pSSA2, "Bias_THDAC", pValue, "mask_peri_A", 0xFF);
    }
    else if(pRegNameMod == "ThresholdHigh" || pRegNameMod == "Bias_THDACHIGH")
    {
        // LOG(DEBUG) << BOLDYELLOW << "!!! Writing to " << pRegNameMod << "," << pValue << RESET;
        return this->WriteChipRegBitsLocal(pSSA2, "Bias_THDACHIGH", pValue, "mask_peri_A", 0xFF);
    }
    else if(pRegNameMod == "CalPulse_duration" || pRegNameMod == "CalPulse_lenght")
    {
        // Duration of the Calibration pulse distributed to the Strip channels, in multiples of 25ns. - bits [7:4] of control_2
        // LOG(DEBUG) << BOLDYELLOW << "!!! Writing to " << pRegNameMod << "," << pValue << RESET;
        return this->WriteChipRegBitsLocal(pSSA2, "control_2", pValue, "mask_peri_D", 0xF0);
    }
    else if(pRegNameMod == "LateralRX_L_PhaseData")
    {
        LOG(ERROR) << BOLDRED << "SSA2 Register " << BOLDYELLOW << pRegNameMod << BOLDRED << " has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal " << RESET;
        throw Exception("SSA2 Register has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal");
        // you probably only need to change (pValue << 4) to pValue but I could not test it

        return this->WriteChipRegBitsLocal(pSSA2, "LateralRX_sampling", pValue, "mask_peri_A", 0x07);
    }
    else if(pRegNameMod == "LateralRX_R_PhaseData")
    {
        LOG(ERROR) << BOLDRED << "SSA2 Register " << BOLDYELLOW << pRegNameMod << BOLDRED << " has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal " << RESET;
        throw Exception("SSA2 Register has not been checked after changes in SSA2Interface::WriteChipRegBitsLocal");
        // you probably only need to change (pValue << 4) to pValue but I could not test it
        return this->WriteChipRegBitsLocal(pSSA2, "LateralRX_sampling", (pValue << 4), "mask_peri_A", (0x7 << 4));
    }
    else
    {
        if(cRegMap.find(pRegNameMod) == cRegMap.end()) { LOG(ERROR) << BOLDYELLOW << __PRETTY_FUNCTION__ << "Cannot find register: " << pRegNameMod << RESET; }
        cRegItem        = cRegMap.at(pRegNameMod);
        cRegItem.fValue = pValue;
        return fBoardFW->SingleRegisterWrite(pSSA2, cRegItem, pVerify);
    }
    return true;
}

std::vector<std::pair<std::string, uint16_t>> SSA2Interface::ReadChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& theRegisterList)
{
    setBoard(pChip->getBeBoardId());
    auto                     cRegMap = pChip->getRegMap();
    std::vector<ChipRegItem> cRegItems;
    for(auto cReq: theRegisterList)
    {
        auto cIterator = cRegMap.find(cReq);
        if(cIterator == cRegMap.end())
        {
            LOG(ERROR) << BOLDRED << "SSA2Interface::WriteChipMultReg trying to write to a register that doesn't exist in the map : " << cReq << RESET;
            abort();
        }

        ChipRegItem cItem = cIterator->second;
        cRegItems.push_back(cItem);
    }

    fBoardFW->MultiRegisterRead(pChip, cRegItems);

    std::vector<std::pair<std::string, uint16_t>> theRegisterValues;
    for(size_t i = 0; i < theRegisterList.size(); ++i) theRegisterValues.push_back(std::make_pair(theRegisterList.at(i), cRegItems.at(i).fValue));
    return theRegisterValues;
}

bool SSA2Interface::enableInjection(ReadoutChip* pChip, bool inject, bool pVerify) { return this->WriteChipReg(pChip, "AnalogueAsync", 1); }
bool SSA2Interface::setInjectionAmplitude(ReadoutChip* pChip, uint8_t injectionAmplitude, bool pVerify) { return this->WriteChipReg(pChip, "InjectedCharge", injectionAmplitude, pVerify); }

bool SSA2Interface::setInjectionSchema(ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool pVerify)
{
    auto cOriginalMask = std::static_pointer_cast<const ChannelGroup<1, NSSACHANNELS>>(pChip->getChipOriginalMask());
    auto groupToMask   = std::static_pointer_cast<const ChannelGroup<1, NSSACHANNELS>>(group);
    auto cBitset       = std::bitset<NSSACHANNELS>(groupToMask->getBitset() & cOriginalMask->getBitset());
    LOG(DEBUG) << BOLDYELLOW << "\t... Applying mask to SSA" << +pChip->getId() << " with " << group->getNumberOfEnabledChannels() << " desired mask \t... : " << cBitset
               << " original mask  \t... : " << cOriginalMask << " enabled channels "
               << " original bitset was be \t... " << groupToMask->getBitset() << RESET;

    auto                     cRegMap = pChip->getRegMap();
    std::vector<ChipRegItem> cRegItems;
    for(uint32_t cIndx = 0; cIndx < pChip->size(); cIndx++)
    {
        std::stringstream cRegName;
        cRegName << "ENFLAGS_S" << (cIndx + 1);
        cRegItems.push_back(cRegMap.at(cRegName.str()));
    }
    auto   cRegValues = fBoardFW->MultiRegisterRead(pChip, cRegItems);
    size_t cIndx      = 0;
    for(auto& cReg: cRegItems)
    {
        uint16_t regval  = cReg.fValue;
        auto     shifted = std::bitset<NSSACHANNELS>(0x1) << cIndx;
        bool     bitval  = bool(((cBitset & shifted) >> cIndx).to_ulong());
        regval           = (regval & 0xEF) | (bitval << 4); // enable injection bit
        cReg.fValue      = regval;
        // LOG(DEBUG) << BOLDYELLOW << "SSA2 - setting mask on channel#"
        //            << "," << cIndx << "," << bitval << "," << std::hex << regval << std::dec << RESET;
        cIndx++;
    }
    return fBoardFW->MultiRegisterWrite(pChip, cRegItems, true);
}
bool SSA2Interface::maskChannelGroup(ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool pVerify)
{
    auto cOriginalMask = std::static_pointer_cast<const ChannelGroup<1, NSSACHANNELS>>(pChip->getChipOriginalMask());
    auto groupToMask   = std::static_pointer_cast<const ChannelGroup<1, NSSACHANNELS>>(group);
    auto cBitset       = std::bitset<NSSACHANNELS>(groupToMask->getBitset() & cOriginalMask->getBitset());

    // LOG(DEBUG) << BOLDYELLOW << "\t... Applying mask to SSA" << +pChip->getId() << " with " << group->getNumberOfEnabledChannels() << " desired mask \t... : " << cBitset
    //            << " original mask  \t... : " << cOriginalMask << " enabled channels "
    //            << " original bitset was be \t... " << groupToMask->getBitset() << RESET;

    std::vector<std::pair<std::string, uint16_t>> pVecReq;
    pVecReq.clear();

    auto                     cRegMap = pChip->getRegMap();
    std::vector<ChipRegItem> cRegItems;
    for(uint32_t cIndx = 0; cIndx < pChip->size(); cIndx++)
    {
        std::stringstream cRegName;
        cRegName << "ENFLAGS_S" << (cIndx + 1);
        cRegItems.push_back(cRegMap.at(cRegName.str()));
    }
    auto   cRegValues = fBoardFW->MultiRegisterRead(pChip, cRegItems);
    size_t cIndx      = 0;
    for(auto& cReg: cRegItems)
    {
        uint16_t regval  = cReg.fValue;
        auto     shifted = std::bitset<NSSACHANNELS>(0x1) << cIndx;
        bool     bitval  = bool(((cBitset & shifted) >> cIndx).to_ulong());
        regval           = (regval & 0xFE) | (bitval);
        cReg.fValue      = regval;
        // LOG(DEBUG) << BOLDYELLOW << "SSA2 - setting mask on channel#"
        //            << "," << cIndx << "," << bitval << "," << regval << RESET;
        cIndx++;
    }
    return fBoardFW->MultiRegisterWrite(pChip, cRegItems, true);
}
bool SSA2Interface::maskChannelsAndSetInjectionSchema(ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool mask, bool inject, bool pVerify)
{
    bool success = true;
    if(mask) success &= maskChannelGroup(pChip, group, pVerify);
    if(inject) success &= setInjectionSchema(pChip, group, pVerify);

    return success;
}
bool SSA2Interface::ConfigureChipOriginalMask(ReadoutChip* pSSA2, bool pVerify, uint32_t pBlockSize)
{
    auto allChannelEnabledGroup = std::make_shared<ChannelGroup<1, NSSACHANNELS>>();
    return maskChannelGroup(pSSA2, allChannelEnabledGroup, pVerify);
}

bool SSA2Interface::MaskAllChannels(ReadoutChip* pSSA2, bool mask, bool pVerify) { return WriteChipRegBitsLocal(pSSA2, "ENFLAGS", mask ? 0 : 1, "mask_strip", 0x01, pVerify); }

bool SSA2Interface::injectNoiseClusters(ReadoutChip* pSSA2, std::vector<Cluster> theClusterList)
{
    WriteChipReg(pSSA2, "ENFLAGS", 0x20);       // masking all MPA and setting readout mode to OR
    WriteChipReg(pSSA2, "StripControl2", 0x07); // disable HIP cut
    // it looks like the trick of masking and invert polarity does not work
    std::vector<std::pair<std::string, uint16_t>> listOfRegisters;

    // This only works with synchronous counters by construction
    listOfRegisters.push_back({"Bias_THDAC", 0xFF});     // set threshold to the maximum
    listOfRegisters.push_back({"Bias_THDACHIGH", 0xFF}); // set hip threshold to the maximum
    listOfRegisters.push_back({"control_1", 0x00});      // normal readout mode
    listOfRegisters.push_back({"control_2", 0x0F});      // maximize cluster cut

    for(const auto& theCluster: theClusterList)
    {
        for(uint8_t stripIndex = 0; stripIndex < theCluster.fColWidth; ++stripIndex)
        {
            std::string registerName = SSA2::getStripRegisterName("ENFLAGS", theCluster.fFirstCol + stripIndex);
            listOfRegisters.push_back({registerName, 0x23}); // Enabling the channel and inverting polarity
        }
    }
    // listOfRegisters.push_back({"mask_strip", 0xFF});

    return WriteChipMultReg(pSSA2, listOfRegisters);
}

} // namespace Ph2_HwInterface
