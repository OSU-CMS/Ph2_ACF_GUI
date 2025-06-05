/*
        FileName :                     PSInterface.cc
        Content :                      User Interface to the PSs
        Programmer :                   K. nash, M. Haranko, D. Ceresa
        Version :                      1.0
        Date of creation :             5/01/18
 */

#include "HWInterface/PSInterface.h"

#include "Utils/ConsoleColor.h"
#include <typeinfo>

#define DEV_FLAG 0

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
PSInterface::PSInterface(const BeBoardFWMap& pBoardMap) : ReadoutChipInterface(pBoardMap)
{
    fTheSSA2Interface                                                   = static_cast<SSA2Interface*>(new SSA2Interface(pBoardMap));
    fTheMPA2Interface                                                   = static_cast<MPA2Interface*>(new MPA2Interface(pBoardMap));
    const std::map<FrontEndType, ReadoutChipInterface*> CHIP_INTERFACE1 = {{FrontEndType::SSA2, fTheSSA2Interface}, {FrontEndType::MPA2, fTheMPA2Interface}};
    CHIP_INTERFACE                                                      = CHIP_INTERFACE1;
}
PSInterface::~PSInterface() {}

ReadoutChipInterface* PSInterface::getInterface(Chip* pPS)
{
    if(pPS->getFrontEndType() == FrontEndType::SSA2)
        return static_cast<SSA2Interface*>((CHIP_INTERFACE.find(pPS->getFrontEndType()))->second);
    else if(pPS->getFrontEndType() == FrontEndType::MPA2)
        return static_cast<MPA2Interface*>((CHIP_INTERFACE.find(pPS->getFrontEndType()))->second);
    else
    {
        std::string errorstring = "Unknown Interface Type " + std::to_string((int)pPS->getFrontEndType());
        throw Exception(errorstring.c_str());
        exit(EXIT_FAILURE);
    }

    // return (CHIP_INTERFACE.find(pPS->getFrontEndType()))->second;
}

std::vector<uint8_t> PSInterface::readLUT(ReadoutChip* pPS, uint8_t pMode)
{
    std::vector<uint8_t> cLUT(0);
    if(pPS->getFrontEndType() == FrontEndType::MPA2) { cLUT = fTheMPA2Interface->readLUT(pPS, pMode); }
    return cLUT;
}
bool PSInterface::setInjectionSchema(ReadoutChip* pPS, const std::shared_ptr<ChannelGroupBase> group, bool pVerifLoop) { return getInterface(pPS)->setInjectionSchema(pPS, group, pVerifLoop); }
bool PSInterface::maskChannelsAndSetInjectionSchema(ReadoutChip* pPS, const std::shared_ptr<ChannelGroupBase> group, bool mask, bool inject, bool pVerifLoop)
{
    return getInterface(pPS)->maskChannelsAndSetInjectionSchema(pPS, group, mask, inject, pVerifLoop);
}
bool PSInterface::maskChannelGroup(ReadoutChip* pPS, const std::shared_ptr<ChannelGroupBase> group, bool pVerifLoop) { return getInterface(pPS)->maskChannelGroup(pPS, group, pVerifLoop); }

bool PSInterface::ConfigureChipOriginalMask(ReadoutChip* pChip, bool pVerifLoop, uint32_t pBlockSize) { return getInterface(pChip)->ConfigureChipOriginalMask(pChip, pVerifLoop, pBlockSize); }

// To generalize
int32_t  PSInterface::ReadChipReg(Chip* pPS, const std::string& pRegName) { return getInterface(pPS)->ReadChipReg(pPS, pRegName); }
uint32_t PSInterface::ReadChipFuseID(Chip* pPS, uint8_t version) { return getInterface(pPS)->ReadChipFuseID(pPS); }

std::vector<std::pair<std::string, uint16_t>> PSInterface::ReadChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& theRegisterList)
{
    return getInterface(pChip)->ReadChipMultReg(pChip, theRegisterList);
}

// To generalize
bool PSInterface::WriteChipReg(Chip* pPS, const std::string& pRegName, uint16_t pValue, bool pVerifLoop)
{
    // LOG(DEBUG) << BOLDMAGENTA << " PSInterface::WriteChipReg writing to " << pRegName << RESET;
    return getInterface(pPS)->WriteChipReg(pPS, pRegName, pValue, pVerifLoop);
}

bool PSInterface::WriteChipMultReg(Chip* pPS, const std::vector<std::pair<std::string, uint16_t>>& pVecReq, bool pVerifLoop) { return getInterface(pPS)->WriteChipMultReg(pPS, pVecReq, pVerifLoop); }

// To generalize
bool PSInterface::WriteChipAllLocalReg(ReadoutChip* pPS, const std::string& dacName, const ChipContainer& localRegValues, bool pVerifLoop)
{
    return getInterface(pPS)->WriteChipAllLocalReg(pPS, dacName, localRegValues, pVerifLoop);
}

// To generalize
bool PSInterface::ConfigureChip(Chip* pPS, bool pVerifLoop, uint32_t pBlockSize) { return getInterface(pPS)->ConfigureChip(pPS, pVerifLoop, pBlockSize); }

void PSInterface::producePhaseAlignmentPattern(ReadoutChip* pChip, uint8_t pWait_ms)
{
    if(pChip->getFrontEndType() == FrontEndType::MPA2) { fTheMPA2Interface->producePhaseAlignmentPattern(pChip, pWait_ms); }
    else { LOG(INFO) << BOLDMAGENTA << "No need to generate phase alignment pattern on SSA#" << +pChip->getId() << " when on a PS module" << RESET; }
}
void PSInterface::produceWordAlignmentPattern(ReadoutChip* pChip)
{
    if(pChip->getFrontEndType() == FrontEndType::MPA2) { fTheMPA2Interface->produceWordAlignmentPattern(pChip); }
    else { LOG(INFO) << BOLDMAGENTA << "No need to generate word alignment pattern on SSA#" << +pChip->getId() << " when on a PS module" << RESET; }
}

void PSInterface::produceBX0AlignmentPattern(ReadoutChip* pChip)
{
    if(pChip->getFrontEndType() == FrontEndType::MPA2) { fTheMPA2Interface->produceBX0AlignmentPattern(pChip); }
    else if(pChip->getFrontEndType() == FrontEndType::SSA2) { LOG(INFO) << BOLDMAGENTA << "No need to generate word alignment pattern on SSA#" << +pChip->getId() << " when on a PS module" << RESET; }
}

bool PSInterface::enableInjection(ReadoutChip* pPS, bool inject, bool pVerifLoop)
{
    if(pPS->getFrontEndType() == FrontEndType::MPA2) { return fTheMPA2Interface->enableInjection(pPS, inject, pVerifLoop); }
    else if(pPS->getFrontEndType() == FrontEndType::SSA2) { return fTheSSA2Interface->enableInjection(pPS, inject, pVerifLoop); }
    else
        LOG(ERROR) << "Bad chip for PS interface";
    return false;
}

//
std::vector<int> PSInterface::decodeBendCode(ReadoutChip* pChip, uint8_t pBendCode)
{
    if(pChip->getFrontEndType() == FrontEndType::MPA2) { return fTheMPA2Interface->decodeBendCode(pChip, pBendCode); }
    return std::vector<int>(0);
}
//
void PSInterface::digiInjection(ReadoutChip* pChip, std::vector<Injection> pInjections, uint8_t pPattern)
{
    if(pChip->getFrontEndType() == FrontEndType::MPA2) { fTheMPA2Interface->digiInjection(pChip, pInjections, pPattern); }
    else
    {
        LOG(ERROR) << BOLDRED << "No digiInjection implemented for SSA for some reason " << RESET;
        throw std::runtime_error(std::string("No digiInjection implemented for SSA for some reason "));
    }
}

bool PSInterface::disableTestPadsOutput(ReadoutChip* pPS) { return getInterface(pPS)->disableTestPadsOutput(pPS); }

// bool PSInterface::selectTestPadsOutput(ReadoutChip* pPS, std::string theRegisterName)
// {
//     return getInterface(pPS)->selectTestPadsOutput(pPS, theRegisterName);

// }

uint32_t PSInterface::readADCGround(ReadoutChip* pPS) { return getInterface(pPS)->readADCGround(pPS); }

uint32_t PSInterface::readADC(ReadoutChip* pPS, std::string theADCName, uint16_t numberOfRead) { return getInterface(pPS)->readADC(pPS, theADCName, numberOfRead); }

uint32_t PSInterface::readADCVref(ReadoutChip* pPS) { return getInterface(pPS)->readADCVref(pPS); }

uint32_t PSInterface::readVrefRegister(ReadoutChip* pPS) { return getInterface(pPS)->readVrefRegister(pPS); }

uint32_t PSInterface::readADCBandGap(ReadoutChip* pPS) { return getInterface(pPS)->readADCBandGap(pPS); }

bool PSInterface::setVref(ReadoutChip* pPS, uint16_t theVrefRegisterValue) { return getInterface(pPS)->setVref(pPS, theVrefRegisterValue); }

bool PSInterface::setVrefFromFuseID(ReadoutChip* pPS) { return getInterface(pPS)->setVrefFromFuseID(pPS); }

const std::map<std::string, std::pair<uint8_t, float>> PSInterface::getBiasStructureDefaultTable(ReadoutChip* pPS) { return getInterface(pPS)->getBiasStructureDefaultTable(pPS); }

float PSInterface::calculateADCLSB(ReadoutChip* pPS, float theVrefValue) { return getInterface(pPS)->calculateADCLSB(pPS, theVrefValue); }

float PSInterface::getBandGapExpectedValue(ReadoutChip* pPS) { return getInterface(pPS)->getBandGapExpectedValue(pPS); }

float PSInterface::getVrefExpectedValue(ReadoutChip* pPS) { return getInterface(pPS)->getVrefExpectedValue(pPS); }

float PSInterface::getVrefPrecision(ReadoutChip* pPS) { return getInterface(pPS)->getVrefPrecision(pPS); }

float PSInterface::getVrefMinValue(ReadoutChip* pPS) { return getInterface(pPS)->getVrefMinValue(pPS); }
float PSInterface::getVrefMaxValue(ReadoutChip* pPS) { return getInterface(pPS)->getVrefMaxValue(pPS); }

bool PSInterface::injectNoiseClusters(ReadoutChip* pPS, std::vector<Cluster> theClusterList)
{
    if(pPS->getFrontEndType() == FrontEndType::MPA2) { return fTheMPA2Interface->injectNoiseClusters(pPS, theClusterList); }
    else { return fTheSSA2Interface->injectNoiseClusters(pPS, theClusterList); }
}

bool PSInterface::injectNoiseStubs(ReadoutChip* pMPA, ReadoutChip* pSSA, std::vector<std::tuple<uint8_t, uint8_t, int>> theStubVector)
{
    if(pMPA->getFrontEndType() != FrontEndType::MPA2 || pSSA->getFrontEndType() != FrontEndType::SSA2)
    {
        std::cerr << __PRETTY_FUNCTION__ << " MPA2 and SSA2 must be provided in the correct order! Aborting..." << std::endl;
        abort();
    }
    std::vector<Cluster> pixelClusterList;
    std::vector<Cluster> stripClusterList;

    for(const auto& theStub: theStubVector)
    {
        uint8_t seedRow         = std::get<0>(theStub);
        uint8_t seedCol         = std::get<1>(theStub) / 2;
        uint8_t seedClusterSize = 1 + std::get<1>(theStub) % 2;

        uint8_t correlationHit         = std::get<1>(theStub) + std::get<2>(theStub);
        uint8_t correlationCol         = correlationHit / 2;
        uint8_t correlationClusterSize = 1 + correlationHit % 2;

        pixelClusterList.push_back(Cluster(seedRow, seedCol, seedClusterSize));
        stripClusterList.push_back(Cluster(0, correlationCol, correlationClusterSize));
    }

    return fTheMPA2Interface->injectNoiseClusters(pMPA, pixelClusterList) && fTheSSA2Interface->injectNoiseClusters(pSSA, stripClusterList);
}

// One should first tune Vref using the BandGap as reference to tune it and then tune the different bias registers.
uint8_t PSInterface::TuneDAC(ReadoutChip* theChip, float theSlope, float theExpectedValue, std::string theDACtoTuneName, uint8_t theDACValue, bool isVref)
{
    LOG(INFO) << CYAN << "Register being tuned: " << theDACtoTuneName << RESET;

    uint32_t theGroundADCValue = this->readADCGround(theChip);

    // write DAC (ie one of the registers) with value 0 (minimum)
    uint8_t theDACMinValue = 0;
    if(isVref)
        this->setVref(theChip, theDACMinValue);
    else
        this->WriteChipReg(theChip, theDACtoTuneName, theDACMinValue);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    uint32_t theOffsetValue = isVref == 0 ? this->readADC(theChip, theDACtoTuneName, 1) : this->readADCBandGap(theChip);
    LOG(DEBUG) << MAGENTA << "The register for " << theDACtoTuneName << " at " << +theDACMinValue << "  gives theOffsetValue " << theOffsetValue << " [ADC]" << RESET;

    // now set the DAC value to its max value
    uint8_t theDACMaxValue = 0x1F;
    if(isVref)
        this->setVref(theChip, theDACMaxValue);
    else
        this->WriteChipReg(theChip, theDACtoTuneName, theDACMaxValue);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    uint32_t theMaxValue = isVref == 0 ? this->readADC(theChip, theDACtoTuneName, 1) : this->readADCBandGap(theChip);
    LOG(DEBUG) << MAGENTA << "The register for " << theDACtoTuneName << " at " << +theDACMaxValue << " gives theMaxValue " << theMaxValue << " [ADC]" << RESET;

    float theLSB = abs(float(theMaxValue) - float(theOffsetValue)) / float(theDACMaxValue);
    LOG(DEBUG) << BOLDMAGENTA << " abs(float(theMaxValue) - float(theOffsetValue)) " << abs(float(theMaxValue) - float(theOffsetValue)) << " float(theDACMaxValue) " << float(theDACMaxValue) << RESET;
    LOG(DEBUG) << BLUE << theDACtoTuneName << " LSB " << theLSB << RESET;

    float theADCDExpectedValue = 0.0;
    theADCDExpectedValue       = theExpectedValue / theSlope + theGroundADCValue; // converted from volts to ADC
    LOG(DEBUG) << MAGENTA << "The register for " << theDACtoTuneName << " expected value in ADC " << theADCDExpectedValue << " [ADC]" << RESET;

    // now set the DAC value to its default value and get the value at the default value
    if(isVref)
        this->setVref(theChip, theDACValue);
    else
        this->WriteChipReg(theChip, theDACtoTuneName, theDACValue);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    uint32_t theCurrentValue = isVref == 0 ? this->readADC(theChip, theDACtoTuneName, 1) : this->readADCBandGap(theChip);
    LOG(INFO) << MAGENTA << "The register to tune at nominal value " << +theDACValue << " gives theCurrentValue " << theCurrentValue << " [ADC]" << RESET;

    int theStepSign = 0;
    if(theADCDExpectedValue < theCurrentValue)
        theStepSign = (isVref) ? 1 : -1;
    else
        theStepSign = (isVref) ? -1 : 1;

    uint8_t theSteps       = uint8_t(std::round(abs(float(theADCDExpectedValue) - float(theCurrentValue)) / float(theLSB)));
    uint8_t theDACNewValue = 0;
    if(float(theDACValue + theStepSign * theSteps) > theDACMaxValue)
        theDACNewValue = theDACMaxValue;
    else if((float(theDACValue + theStepSign * theSteps) < theDACMinValue))
        theDACNewValue = theDACMinValue;
    else
        theDACNewValue = theDACValue + theStepSign * theSteps;

    theDACValue = theDACNewValue;

    LOG(DEBUG) << MAGENTA << "Predicted number of register steps to get the expected value " << +theSteps << " giving the new register value of " << +theDACNewValue << RESET;

    // now writing the DAC to the new value estimated above
    if(isVref)
        this->setVref(theChip, theDACNewValue);
    else
        this->WriteChipReg(theChip, theDACtoTuneName, theDACNewValue);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    uint32_t theNewValue = isVref == 0 ? this->readADC(theChip, theDACtoTuneName, 1) : this->readADCBandGap(theChip);
    LOG(DEBUG) << MAGENTA << "After changing value for DAC " << theDACtoTuneName << " to " << +theDACValue << " the ADC value is " << theNewValue << " [ADC]" << RESET;

    // Now checking if we can go even closer to the expected value
    bool     isSearching         = true;
    uint32_t theCurrentIteration = 0;
    while(isSearching)
    {
        LOG(DEBUG) << YELLOW << "Checking if we can go closer to the expected value. Iteration " << theCurrentIteration << RESET;
        LOG(DEBUG) << MAGENTA << " theDACNewValue - 1 " << +theDACNewValue - 1 << RESET;
        uint8_t theDACDownValue = std::max(theDACMinValue, uint8_t(theDACNewValue - 1));
        if(isVref)
            this->setVref(theChip, theDACDownValue);
        else
            this->WriteChipReg(theChip, theDACtoTuneName, theDACDownValue);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        uint32_t theNewValueDown = isVref == 0 ? this->readADC(theChip, theDACtoTuneName, 1) : this->readADCBandGap(theChip);

        uint8_t theDACUpValue = std::min(uint8_t(theDACMaxValue), uint8_t(theDACNewValue + 1));
        LOG(DEBUG) << MAGENTA << "theDACUpValue " << +theDACUpValue << RESET;
        if(isVref)
            this->setVref(theChip, theDACUpValue);
        else
            this->WriteChipReg(theChip, theDACtoTuneName, theDACUpValue);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        uint32_t theNewValueUp = isVref == 0 ? this->readADC(theChip, theDACtoTuneName, 1) : this->readADCBandGap(theChip);

        float theExpectedDifference     = std::fabs(theADCDExpectedValue - theNewValue);
        float theExpectedDifferenceDown = std::fabs(theADCDExpectedValue - theNewValueDown);
        float theExpectedDifferenceUp   = std::fabs(theADCDExpectedValue - theNewValueUp);

        if((theExpectedDifferenceDown < theExpectedDifference) || (theExpectedDifferenceUp < theExpectedDifference))
        {
            LOG(DEBUG) << BOLDRED << "Not precise extrapolation in OTPSADCCalibration: theExpectedDifferenceDown:" << theExpectedDifferenceDown
                       << ", theExpectedDifferenceUp:" << theExpectedDifferenceUp << ", theExpectedDifference:" << theExpectedDifference << RESET;
            if((theExpectedDifferenceDown < theExpectedDifference))
            {
                theDACValue    = theDACDownValue;
                theDACNewValue = theDACNewValue - 1;
            }
            if((theExpectedDifferenceUp < theExpectedDifference))
            {
                theDACValue    = theDACUpValue;
                theDACNewValue = std::min(uint8_t(theDACMaxValue), uint8_t(theDACNewValue + 1));
            }
        }
        else
        {
            LOG(DEBUG) << BOLDGREEN << "Good extrapolation in OTPSADCCalibration for register value " << +theDACValue << RESET;
            if(isVref)
                this->setVref(theChip, theDACValue);
            else
                this->WriteChipReg(theChip, theDACtoTuneName, theDACValue);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));

            uint32_t theCheckValue = isVref == 0 ? this->readADC(theChip, theDACtoTuneName, 1) : this->readADCBandGap(theChip);
            LOG(DEBUG) << BOLDGREEN << "Register " << theDACtoTuneName << " gives ADC " << theCheckValue << RESET;
            isSearching = false;
        }
        LOG(DEBUG) << BOLDMAGENTA << "Writing DAC val " << +theDACValue << RESET;
        if(isVref)
            this->setVref(theChip, theDACValue);
        else
            this->WriteChipReg(theChip, theDACtoTuneName, theDACValue);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

        theNewValue = isVref == 0 ? this->readADC(theChip, theDACtoTuneName, 1) : this->readADCBandGap(theChip);

        theCurrentIteration += 1;
    }

    LOG(INFO) << BOLDGREEN << "Register: " << theDACtoTuneName << " -> New tuned value: " << theNewValue << " Expected value: " << theADCDExpectedValue << "+/-" << theLSB << RESET;

    return theDACValue;
}

float PSInterface::readADCVoltage(Ph2_HwDescription::ReadoutChip* pPS, std::string theADCName)
{
    float adcValue            = readADC(pPS, theADCName, 1);
    float theConversionFactor = 1;                                             // without the conversion factor the voltages are not visible
    if(theADCName == "AVDD" || theADCName == "DVDD") theConversionFactor *= 2; // keep into account a voltage divider
    return (adcValue * pPS->getADCCalibrationValue("ADC_SLOPE") + pPS->getADCCalibrationValue("ADC_OFFSET")) * theConversionFactor;
}

float PSInterface::measureTemperature(Ph2_HwDescription::ReadoutChip* pPS)
{
    return (readADCVoltage(pPS, "temperature") - pPS->getADCCalibrationValue("TEMP_OFFSET")) / pPS->getADCCalibrationValue("TEMP_SLOPE") + 25;
}

bool PSInterface::MaskAllChannels(Ph2_HwDescription::ReadoutChip* pPS, bool mask, bool pVerifLoop)
{
    if(pPS->getFrontEndType() == FrontEndType::MPA2) { return fTheMPA2Interface->MaskAllChannels(pPS, mask, false); }
    else { return fTheSSA2Interface->MaskAllChannels(pPS, mask, false); }
}
} // namespace Ph2_HwInterface
