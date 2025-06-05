#include "HWInterface/D19cBERTinterface.h"
#include "HWInterface/ExceptionHandler.h"
#include "HWInterface/RegManager.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/DataContainer.h"
#include "Utils/GenericDataArray.h"
#include <cmath>
#include <iostream>
#include <thread>

using namespace Ph2_HwInterface;

bool                               BitErrorTestControl::fCurrentCheckMode     = false;
bool                               BitErrorTestControl::fIsDebugModeActivated = false;
BitErrorTestControl::Mode          BitErrorTestControl::fCurrentMode          = BitErrorTestControl::Mode::None0;
BitErrorTestControl::CounterSelect BitErrorTestControl::fCurrentCounterSelect = BitErrorTestControl::CounterSelect::FrameCounterLSB;

uint32_t BitErrorTestControl::encodeCommand() const
{
    uint32_t theCommand = 0;

    theCommand |= ((fHybridId & 0x1F) << 27);
    theCommand |= ((fChipId & 0x7) << 24);
    theCommand |= ((fLineId & 0xF) << 20);
    theCommand |= ((static_cast<uint8_t>(fCommand) & 0xF) << 16);

    switch(fCommand)
    {
    case Command::Configure:
        if(fHybridId == 0x1F || fChipId == 0x7 || fLineId == 0xF)
        {
            theCommand |= ((fDebugMode ? 1 : 0) << 8);
            theCommand |= ((fCheckMode ? 1 : 0) << 7);
            fIsDebugModeActivated = fDebugMode;
        }
        theCommand |= ((fLineSelect & 0xF) << 12);
        theCommand |= ((fPatternWordIndex & 0x7) << 9);
        theCommand |= ((fIsMask ? 1 : 0) << 6);
        theCommand |= ((static_cast<uint8_t>(fCounterSelect) & 0x3) << 4);
        theCommand |= ((static_cast<uint8_t>(fMode) & 0x3) << 2);
        theCommand |= ((fCheckEnable ? 1 : 0) << 1);
        theCommand |= ((fReceiveEnable ? 1 : 0) << 0);
        fCurrentMode          = fMode;
        fCurrentCounterSelect = fCounterSelect;
        fCurrentCheckMode     = fCheckMode;
        break;

    case Command::SetFirstPattern: theCommand |= ((fFirstPattern & 0xFFFF) << 0); break;

    case Command::ErrorInject:
        theCommand |= ((fErrorInjection ? 1 : 0) << 7);
        theCommand |= ((fDataLoad ? 1 : 0) << 2);
        break;

    case Command::ReadBERTfirstData:
        if(fIsDebugModeActivated) { theCommand |= ((fPattern & 0xFFFF) << 0); }
        break;

    case Command::ReadBERTsampledData:
        // if(fIsDebugModeActivated) { theCommand |= ((fPackagePatternMSB & 0xFF) << 0); }
        break;

    default: break;
    }

    return theCommand;
}

void BitErrorTestControl::getLine(const BitErrorTestControl& theBitErrorTestReply)
{
    fHybridId = theBitErrorTestReply.fHybridId;
    fChipId   = theBitErrorTestReply.fChipId;
    fLineId   = theBitErrorTestReply.fLineId;
}

void BitErrorTestControl::resetCommandBits()
{
    fCommand        = Command::ReturnConfig;
    fDebugMode      = false;
    fCheckMode      = false;
    fCounterSelect  = CounterSelect::FrameCounterLSB;
    fMode           = Mode::None0;
    fCheckEnable    = false;
    fReceiveEnable  = false;
    fFirstPattern   = 0;
    fErrorInjection = false;
    fDataLoad       = false;
}

void BitErrorTestReply::decodeReply(uint32_t reply, const BitErrorTestControl& theBitErrorTestControl)
{
    fHybridId = theBitErrorTestControl.fHybridId;
    fChipId   = theBitErrorTestControl.fChipId;
    fLineId   = theBitErrorTestControl.fLineId;

    auto checkAddress = [this](uint32_t reply, const std::string& caseName)
    {
        uint8_t theHybridId = (reply >> 27) & 0x1F;
        uint8_t theChipId   = (reply >> 24) & 0x7;
        uint8_t theLineId   = (reply >> 20) & 0xF;
        if(theHybridId != fHybridId || theChipId != fChipId || theLineId != fLineId)
        {
            std::string errorMessage = std::string(__PRETTY_FUNCTION__) + " case " + caseName + " requesting info for hybrid " + std::to_string(this->fHybridId) + " chip " +
                                       std::to_string(this->fChipId) + " line " + std::to_string(this->fLineId) + " but received hybrid " + std::to_string(theHybridId) + " chip " +
                                       std::to_string(theChipId) + " line " + std::to_string(theLineId);
            std::cerr << errorMessage << std::endl;
            throw std::runtime_error(errorMessage);
        }
    };

    switch(theBitErrorTestControl.fCommand)
    {
    case BitErrorTestControl::Command::ReturnConfig:
    {
        checkAddress(reply, "ReturnConfig");

        bool isFlagSet                    = ((reply >> 15) & 0x1) > 0;
        fMode                             = static_cast<BitErrorTestControl::Mode>((reply >> 2) & 0x3);
        BitErrorTestControl::fCurrentMode = fMode;
        switch(BitErrorTestControl::fCurrentMode)
        {
        case BitErrorTestControl::Mode::PRBS: fPRBScounterOverflow = isFlagSet; break;

        case BitErrorTestControl::Mode::LSFR: fLFSRcounterOverflow = isFlagSet; break;

        default: break;
        }
        fPRBScheckStateMachineStatus = (reply >> 12) & 0x3;
        fCheckMode                   = ((reply >> 7) & 0x1) > 0;
        fCounterReset                = ((reply >> 6) & 0x1) > 0;
        fCounterSelect               = (reply >> 4) & 0x3;
        fCheckEnable                 = ((reply >> 1) & 0x1) > 0;
        fReceiveEnable               = ((reply >> 0) & 0x1) > 0;
        break;
    }

    case BitErrorTestControl::Command::ReturnFirstPattern:
        checkAddress(reply, "ReturnFirstfFirstPattern");
        fFirstPattern = reply & 0xFF;
        break;

    case BitErrorTestControl::Command::ReadCounterData:
        switch(BitErrorTestControl::fCurrentMode)
        {
        case BitErrorTestControl::Mode::PRBS:
            if(BitErrorTestControl::fCurrentCheckMode)
            {
                switch(BitErrorTestControl::fCurrentCounterSelect)
                {
                case BitErrorTestControl::CounterSelect::FrameCounterLSB: fFrameCounterLSB = reply; break;
                case BitErrorTestControl::CounterSelect::FrameCounterMSB: fFrameCounterMSB = reply; break;
                case BitErrorTestControl::CounterSelect::FrameErrorCounter: fPRBSframeCounterValueEmulator = reply; break;
                case BitErrorTestControl::CounterSelect::BitErrorCounter: fPRBSbitCounterValueEmulator = reply; break;
                default: break;
                }
            }
            else
            {
                switch(BitErrorTestControl::fCurrentCounterSelect)
                {
                case BitErrorTestControl::CounterSelect::FrameCounterLSB: fFrameCounterLSB = reply; break;
                case BitErrorTestControl::CounterSelect::FrameCounterMSB: fFrameCounterMSB = reply; break;
                case BitErrorTestControl::CounterSelect::FrameErrorCounter: fPRBSframeCounterValuePredictNext = reply; break;
                case BitErrorTestControl::CounterSelect::BitErrorCounter: fPRBSbitCounterValuePredictNext = reply; break;
                default: break;
                }
            }
            break;

        case BitErrorTestControl::Mode::LSFR:
            // not handled by the FW at the moment
            break;

        case BitErrorTestControl::Mode::Pattern:
            switch(BitErrorTestControl::fCurrentCounterSelect)
            {
            case BitErrorTestControl::CounterSelect::FrameCounterLSB: fFrameCounterLSB = reply; break;
            case BitErrorTestControl::CounterSelect::FrameCounterMSB: fFrameCounterMSB = reply; break;
            case BitErrorTestControl::CounterSelect::FrameErrorCounter: fPRBSframeCounterValueEmulator = reply; break;
            case BitErrorTestControl::CounterSelect::BitErrorCounter: fPRBSbitCounterValueEmulator = reply; break;
            default: break;
            }
            break;

        default: break;
        }
        break;

    case BitErrorTestControl::Command::ReadBERTfirstData:
        if(!BitErrorTestControl::fIsDebugModeActivated)
        {
            switch(BitErrorTestControl::fCurrentMode)
            {
            case BitErrorTestControl::Mode::PRBS: fPRBSfirstData = reply; break;
            case BitErrorTestControl::Mode::LSFR: fLFSRfirstData = reply; break;
            case BitErrorTestControl::Mode::Pattern: fPRBSfirstData = reply; break;

            default: break;
            }
        }
        break;

    case BitErrorTestControl::Command::ReadBERTsampledData:
        if(BitErrorTestControl::fIsDebugModeActivated)
        {
            switch(BitErrorTestControl::fCurrentMode)
            {
            case BitErrorTestControl::Mode::PRBS: fPRBSdata = reply; break;

            case BitErrorTestControl::Mode::LSFR: fLFSRdata = reply; break;

            default: break;
            }
        }
        break;

    default: break;
    }
}

D19cBERTinterface::D19cBERTinterface(RegManager* theRegManager) : fTheRegManager(theRegManager) {}

D19cBERTinterface::~D19cBERTinterface() {}

void D19cBERTinterface::writeCommand(BitErrorTestControl theBitErrorTestControl)
{
    uint32_t theCommand = theBitErrorTestControl.encodeCommand();

    size_t maxNumberOfIterations = 10;
    size_t iterationNumber       = 0;
    while(iterationNumber < maxNumberOfIterations)
    {
        fTheRegManager->WriteReg("fc7_daq_ctrl.physical_interface_block.bert_control", theCommand);
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        uint32_t reply = fTheRegManager->ReadReg("fc7_daq_stat.physical_interface_block.bert_readback");
        if(theCommand == reply) break;
        LOG(DEBUG) << WARNING_FORMAT << "D19cBERTinterface::writeCommandWithCheck: Failed to write command 0x" << std::hex << theCommand << " read back 0x" << reply << std::dec << RESET;
        ++iterationNumber;
    }
    if(iterationNumber >= maxNumberOfIterations)
    {
        LOG(ERROR) << ERROR_FORMAT << "D19cBERTinterface::writeCommandWithCheck: Failed to write command 0x" << std::hex << theCommand << std::dec << " after " << iterationNumber << " iterations"
                   << RESET;
    }
}

void D19cBERTinterface::loadSampleData(uint16_t hybridId, uint8_t lineId)
{
    BitErrorTestControl theBitErrorTestControl;
    theBitErrorTestControl.setHybridId(hybridId);
    theBitErrorTestControl.setChipId(lineId == 0xF ? 0x7 : 0x0);
    theBitErrorTestControl.setLineId(lineId);
    theBitErrorTestControl.setCommand(BitErrorTestControl::Command::ErrorInject);
    theBitErrorTestControl.setDataLoad(true);
    writeCommand(theBitErrorTestControl);
}

void D19cBERTinterface::readSampleData(uint16_t hybridId, uint8_t lineId)
{
    BitErrorTestControl theBitErrorTestControl;
    theBitErrorTestControl.setHybridId(hybridId);
    theBitErrorTestControl.setChipId(lineId == 0xF ? 0x7 : 0x0);
    theBitErrorTestControl.setLineId(lineId);
    theBitErrorTestControl.setCommand(BitErrorTestControl::Command::ReadBERTsampledData);

    readReplay(theBitErrorTestControl);
}

void D19cBERTinterface::loadAlignmentPattern(uint16_t hybridId, uint8_t lineId)
{
    BitErrorTestControl selectSyncMaskCommand;
    selectSyncMaskCommand.setHybridId(hybridId);
    selectSyncMaskCommand.setChipId(lineId == 0xF ? 0x7 : 0x0);
    selectSyncMaskCommand.setLineId(lineId);
    selectSyncMaskCommand.resetCommandBits();
    selectSyncMaskCommand.setCommand(BitErrorTestControl::Command::Configure);
    selectSyncMaskCommand.setIsMask(false);
    selectSyncMaskCommand.setLineSelect(fLineSelect);
    writeCommand(selectSyncMaskCommand);

    BitErrorTestControl theBitErrorTestSetAlignmentPattern;
    theBitErrorTestSetAlignmentPattern.getLine(selectSyncMaskCommand);
    theBitErrorTestSetAlignmentPattern.resetCommandBits();
    theBitErrorTestSetAlignmentPattern.setCommand(BitErrorTestControl::Command::SetFirstPattern);
    if(fUsePRBS)
        theBitErrorTestSetAlignmentPattern.setFirstPattern(BERT_ALIGNMENT_PATTERN);
    else { theBitErrorTestSetAlignmentPattern.setFirstPattern(fCheckedPatternMap.at(hybridId).at(0) >> 16); }
    writeCommand(theBitErrorTestSetAlignmentPattern);

    BitErrorTestControl setDebugModeCommand;
    setDebugModeCommand.getLine(selectSyncMaskCommand);
    setDebugModeCommand.setCommand(BitErrorTestControl::Command::Configure);
    setDebugModeCommand.setIsMask(true);
    setDebugModeCommand.setLineSelect(fLineSelect);
    writeCommand(setDebugModeCommand);

    BitErrorTestControl setSyncPatternMask;
    setSyncPatternMask.getLine(selectSyncMaskCommand);
    setSyncPatternMask.setCommand(BitErrorTestControl::Command::SetFirstPattern);

    if(fUsePRBS)
        setSyncPatternMask.setFirstPattern(0xffff);
    else { setSyncPatternMask.setFirstPattern(fCheckedPatternMaskMap.at(hybridId).at(0) >> 16); }
    writeCommand(setSyncPatternMask);
}

void D19cBERTinterface::startPatternSyncronization(uint8_t hybridId, uint8_t lineId)
{
    BitErrorTestControl theBitErrorTestConfigure;
    theBitErrorTestConfigure.setHybridId(hybridId);
    theBitErrorTestConfigure.setChipId(lineId == 0xF ? 0x7 : 0x0);
    theBitErrorTestConfigure.setLineId(lineId);
    theBitErrorTestConfigure.setLineSelect(fLineSelect);
    theBitErrorTestConfigure.setCommand(BitErrorTestControl::Command::Configure);
    theBitErrorTestConfigure.setCounterSelect(BitErrorTestControl::CounterSelect::BitErrorCounter);
    theBitErrorTestConfigure.setMode(fUsePRBS ? BitErrorTestControl::Mode::PRBS : BitErrorTestControl::Mode::Pattern);
    theBitErrorTestConfigure.setReceiveEnable(true);
    theBitErrorTestConfigure.setCheckMode(true);
    theBitErrorTestConfigure.setLineSelect(fLineSelect);
    writeCommand(theBitErrorTestConfigure);
}

void D19cBERTinterface::startBitErrorRateTest(uint8_t hybridId, uint8_t lineId)
{
    BitErrorTestControl theBitErrorTestConfigure;
    theBitErrorTestConfigure.setHybridId(hybridId);
    theBitErrorTestConfigure.setChipId(lineId == 0xF ? 0x7 : 0x0);
    theBitErrorTestConfigure.setLineId(lineId);
    theBitErrorTestConfigure.setLineSelect(fLineSelect);

    theBitErrorTestConfigure.setCommand(BitErrorTestControl::Command::Configure);
    theBitErrorTestConfigure.setCounterSelect(BitErrorTestControl::CounterSelect::BitErrorCounter);
    theBitErrorTestConfigure.setMode(fUsePRBS ? BitErrorTestControl::Mode::PRBS : BitErrorTestControl::Mode::Pattern);
    theBitErrorTestConfigure.setReceiveEnable(true);
    theBitErrorTestConfigure.setCheckMode(true);
    theBitErrorTestConfigure.setCheckEnable(true);
    writeCommand(theBitErrorTestConfigure);
}

void D19cBERTinterface::stopBitErrorRateTest(uint8_t hybridId, uint8_t lineId)
{
    BitErrorTestControl theBitErrorTestControl;
    theBitErrorTestControl.setHybridId(hybridId);
    theBitErrorTestControl.setChipId(lineId == 0xF ? 0x7 : 0x0);
    theBitErrorTestControl.setLineId(lineId);
    theBitErrorTestControl.setLineSelect(fLineSelect);

    theBitErrorTestControl.setCommand(BitErrorTestControl::Command::Configure);
    theBitErrorTestControl.setCounterSelect(BitErrorTestControl::CounterSelect::BitErrorCounter);
    theBitErrorTestControl.setMode(fUsePRBS ? BitErrorTestControl::Mode::PRBS : BitErrorTestControl::Mode::Pattern);
    theBitErrorTestControl.setReceiveEnable(true);
    theBitErrorTestControl.setCheckMode(true);
    writeCommand(theBitErrorTestControl);
}

void D19cBERTinterface::haltBitErrorRateTest(uint8_t hybridId, uint8_t lineId)
{
    BitErrorTestControl theBitErrorTestControl;
    theBitErrorTestControl.setHybridId(hybridId);
    theBitErrorTestControl.setChipId(lineId == 0xF ? 0x7 : 0x0);
    theBitErrorTestControl.setLineId(lineId);
    theBitErrorTestControl.setLineSelect(fLineSelect);

    theBitErrorTestControl.setCommand(BitErrorTestControl::Command::Configure);
    theBitErrorTestControl.setCounterSelect(BitErrorTestControl::CounterSelect::BitErrorCounter);
    theBitErrorTestControl.setMode(fUsePRBS ? BitErrorTestControl::Mode::PRBS : BitErrorTestControl::Mode::Pattern);
    theBitErrorTestControl.setCheckMode(true);
    writeCommand(theBitErrorTestControl);
}

uint32_t D19cBERTinterface::getBitErrorCounters(uint8_t hybridId, uint8_t lineId)
{
    BitErrorTestControl theBitErrorTestControl;
    theBitErrorTestControl.setHybridId(hybridId);
    theBitErrorTestControl.setChipId(0);
    theBitErrorTestControl.setLineId(lineId);
    theBitErrorTestControl.setCommand(BitErrorTestControl::Command::ReadCounterData);
    size_t maxNumberOfIterations = 10;
    size_t iterationNumber       = 0;
    while(iterationNumber < maxNumberOfIterations)
    {
        size_t                numberOfRead = 3;
        std::vector<uint32_t> readValues(numberOfRead);
        for(size_t readIt = 0; readIt < numberOfRead; ++readIt)
        {
            BitErrorTestReply theBitErrorTestCounter = readReplay(theBitErrorTestControl);
            readValues.at(readIt)                    = theBitErrorTestCounter.getPRBSbitCounterValueEmulator();
        }
        if(std::adjacent_find(readValues.begin(), readValues.end(), std::not_equal_to<>()) == readValues.end())
            return readValues.at(0);
        else
            ++iterationNumber;
    }
    return 0xFFFFFFFF;
}

void D19cBERTinterface::selectFrameCounters(bool isMSB)
{
    BitErrorTestControl theSetFrameCounterControl;
    theSetFrameCounterControl.setHybridId(0x1F);
    theSetFrameCounterControl.setChipId(0x7);
    theSetFrameCounterControl.setLineId(0xF);
    theSetFrameCounterControl.setLineSelect(fLineSelect);

    theSetFrameCounterControl.setCommand(BitErrorTestControl::Command::Configure);
    theSetFrameCounterControl.setCounterSelect(BitErrorTestControl::CounterSelect::BitErrorCounter);
    theSetFrameCounterControl.setMode(fUsePRBS ? BitErrorTestControl::Mode::PRBS : BitErrorTestControl::Mode::Pattern);
    theSetFrameCounterControl.setReceiveEnable(true);
    theSetFrameCounterControl.setCheckMode(true);
    if(isMSB)
        theSetFrameCounterControl.setCounterSelect(BitErrorTestControl::CounterSelect::FrameCounterMSB);
    else
        theSetFrameCounterControl.setCounterSelect(BitErrorTestControl::CounterSelect::FrameCounterLSB);
    writeCommand(theSetFrameCounterControl);
}

uint64_t D19cBERTinterface::getFrameCounters(uint8_t hybridId, uint8_t lineId, bool isMSB)
{
    BitErrorTestControl theReadDataControl;
    theReadDataControl.setHybridId(hybridId);
    theReadDataControl.setChipId(0);
    theReadDataControl.setLineId(lineId);
    theReadDataControl.setCommand(BitErrorTestControl::Command::ReadCounterData);
    size_t maxNumberOfIterations = 10;
    size_t iterationNumber       = 0;
    while(iterationNumber < maxNumberOfIterations)
    {
        size_t                numberOfRead = 3;
        std::vector<uint32_t> readValues(numberOfRead);
        for(size_t readIt = 0; readIt < numberOfRead; ++readIt)
        {
            selectFrameCounters(isMSB);
            BitErrorTestReply theFrameLSBcounter = readReplay(theReadDataControl);
            readValues.at(readIt)                = isMSB ? theFrameLSBcounter.getFrameCounterMSB() : theFrameLSBcounter.getFrameCounterLSB();
        }
        if(std::adjacent_find(readValues.begin(), readValues.end(), std::not_equal_to<>()) == readValues.end()) { return readValues.at(0); }
        else
            ++iterationNumber;
    }
    return 0xFFFFFFFFFFFFFFFF;
}

uint8_t D19cBERTinterface::getCheckerFSMstatus(uint8_t hybridId, uint8_t lineId)
{
    BitErrorTestControl theReadConfig;
    theReadConfig.setHybridId(hybridId);
    theReadConfig.setChipId(0);
    theReadConfig.setLineId(lineId);
    theReadConfig.setCommand(BitErrorTestControl::Command::ReturnConfig);
    BitErrorTestReply theConfig = readReplay(theReadConfig);

    return theConfig.getPRBScheckStateMachineStatus();
}

uint32_t D19cBERTinterface::getFirstData(uint8_t hybridId, uint8_t lineId)
{
    BitErrorTestControl theBitErrorTestControl;
    theBitErrorTestControl.setHybridId(hybridId);
    theBitErrorTestControl.setChipId(0);
    theBitErrorTestControl.setLineId(lineId);

    theBitErrorTestControl.resetCommandBits();
    theBitErrorTestControl.setCommand(BitErrorTestControl::Command::ReadBERTfirstData);
    BitErrorTestReply theBitErrorTestCounter = readReplay(theBitErrorTestControl);

    return theBitErrorTestCounter.getPRBSfirstData();
}

BitErrorTestReply D19cBERTinterface::readReplay(const BitErrorTestControl& theBitErrorTestControl)
{
    uint16_t          iteration     = 0;
    uint16_t          maxIterations = 10;
    BitErrorTestReply theBitErrorTestReply;
    while(iteration < maxIterations)
    {
        writeCommand(theBitErrorTestControl);

        uint32_t reply = fTheRegManager->ReadReg("fc7_daq_stat.physical_interface_block.bert_stat");
        // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] fc7_daq_stat.physical_interface_block.bert_stat = 0x"  << std::hex << reply << std::dec << std::endl;

        try
        {
            theBitErrorTestReply.decodeReply(reply, theBitErrorTestControl);
            break;
        }
        catch(const std::exception& e)
        {
            ++iteration;
        }
    }

    return theBitErrorTestReply;
}

void D19cBERTinterface::injectError(uint8_t hybridId, uint8_t lineId)
{
    BitErrorTestControl theBitErrorTestControl;
    theBitErrorTestControl.setHybridId(hybridId);
    theBitErrorTestControl.setChipId(lineId == 0xF ? 0x7 : 0x0);
    theBitErrorTestControl.setLineId(lineId);
    theBitErrorTestControl.setCommand(BitErrorTestControl::Command::ErrorInject);
    theBitErrorTestControl.setErrorInjection(true);
    writeCommand(theBitErrorTestControl);
}

void D19cBERTinterface::waitForNeededBits(bool is10Gmodule, float numberOfMatchedBits)
{
    float dataRate = 3.2E5;
    if(is10Gmodule) dataRate *= 2.;
    if(!fUsePRBS)
    {
        float theMinimumNumber = 1e10;
        bool  minimumFound     = false;
        for(const auto& theNumberOfCheckedBits: fNumberOfCheckedBitsMap)
        {
            if(theNumberOfCheckedBits.second == 0) continue;
            if(theNumberOfCheckedBits.second < theMinimumNumber)
            {
                theMinimumNumber = theNumberOfCheckedBits.second;
                minimumFound     = true;
            }
        }
        if(!minimumFound)
        {
            std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] theMinimumNumber = 0, impossible to measure meaninless pattern error bits, aborting" << std::endl;
            abort();
        }
        dataRate *= theMinimumNumber / (32. * 4);
    }

    float    extimatedWaitInMilliseconds = numberOfMatchedBits / dataRate + 1;
    uint32_t waitInMilliSeconds          = ceil(extimatedWaitInMilliseconds);

    uint32_t sleepingStepMilliSeconds = 5000;
    while(waitInMilliSeconds >= sleepingStepMilliSeconds)
    {
        LOG(INFO) << BOLDMAGENTA << "Sleeping for other " << waitInMilliSeconds / 1000 << " seconds" << RESET;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepingStepMilliSeconds));
        waitInMilliSeconds -= sleepingStepMilliSeconds;
    }
    if(waitInMilliSeconds > 0) { std::this_thread::sleep_for(std::chrono::milliseconds(waitInMilliSeconds)); }
}

std::tuple<bool, bool, std::map<uint16_t, bool>> D19cBERTinterface::isStartPatternFound(BoardContainer* theBoardContainer, uint8_t lineNumber)
{
    std::tuple<bool, bool, std::map<uint16_t, bool>> startPatternFoundResults;
    std::get<0>(startPatternFoundResults) = true;
    std::get<1>(startPatternFoundResults) = false;
    for(auto theOpticalGroup: *theBoardContainer)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            std::get<2>(startPatternFoundResults)[theHybrid->getId()] = false;
            uint16_t iteration                                        = 0;
            uint16_t maxIterations                                    = 3;
            uint32_t firstData;
            while(iteration < maxIterations)
            {
                firstData = getFirstData(theHybrid->getId(), lineNumber);

                if(firstData != 0xfedececa)
                {
                    if(fUsePRBS)
                    {
                        if((firstData >> 16) == BERT_ALIGNMENT_PATTERN)
                        {
                            std::get<2>(startPatternFoundResults)[theHybrid->getId()] = true;
                            std::get<1>(startPatternFoundResults)                     = true;
                            break;
                        }
                    }
                    else
                    {
                        if(((firstData && fCheckedPatternMaskMap.at(theHybrid->getId()).at(0)) >> 16) ==
                           ((fCheckedPatternMap.at(theHybrid->getId()).at(0) && fCheckedPatternMaskMap.at(theHybrid->getId()).at(0)) >> 16))
                        {
                            std::get<2>(startPatternFoundResults)[theHybrid->getId()] = true;
                            std::get<1>(startPatternFoundResults)                     = true;
                            break;
                        }
                    }
                }
                ++iteration;
            }
            if(iteration >= maxIterations)
            {
                if(!fSuppressErrorPrintout)
                    LOG(WARNING) << WARNING_FORMAT << "Failed to find BERT start pattern on OpticalGroup " << theOpticalGroup->getId() << " Hybrid " << theHybrid->getId() << " line " << +lineNumber
                                 << " after " << maxIterations << " iterations. Last word read = 0x" << std::hex << firstData << std::dec << RESET;
                std::get<0>(startPatternFoundResults) = false;
            }
        }
    }

    return startPatternFoundResults;
}

bool D19cBERTinterface::isStateMachineStarted(BoardContainer* theBoardContainer, uint8_t lineNumber)
{
    bool allLineStarted = true;
    for(auto theOpticalGroup: *theBoardContainer)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            uint16_t iteration     = 0;
            uint16_t maxIterations = 1;
            uint8_t  checkerFSMstatus;
            while(iteration < maxIterations)
            {
                checkerFSMstatus = getCheckerFSMstatus(theHybrid->getId(), lineNumber);

                if(checkerFSMstatus == 0x2) break;
                ++iteration;
            }
            if(iteration >= maxIterations)
            {
                if(!fSuppressErrorPrintout)
                    LOG(WARNING) << WARNING_FORMAT << "Failed to start BERT checker on OpticalGroup " << theOpticalGroup->getId() << " Hybrid " << theHybrid->getId() << " line " << +lineNumber
                                 << " after " << maxIterations << " iterations. FSM status = 0x" << std::hex << +checkerFSMstatus << std::dec << RESET;
                allLineStarted = false;
            }
        }
    }

    return allLineStarted;
}

uint64_t D19cBERTinterface::readNumberOfTestedBit(uint16_t hybridId, uint8_t lineId, bool is10Gmodule)
{
    uint64_t theBitCounterCounter = (getFrameCounters(hybridId, lineId, true) << 32);

    theBitCounterCounter |= getFrameCounters(hybridId, lineId, false);

    float numberOfCheckedBits = (is10Gmodule ? 16. : 8.);
    if(!fUsePRBS) { numberOfCheckedBits *= fNumberOfCheckedBitsMap.at(hybridId) / (32. * fCheckedPatternMaskMap.at(hybridId).size()); }

    theBitCounterCounter *= numberOfCheckedBits;
    return theBitCounterCounter;
}

bool D19cBERTinterface::retrieveBitTestedCounterLine(BoardDataContainer*      theBoardContainer,
                                                     uint8_t                  lineNumber,
                                                     bool                     is10Gmodule,
                                                     float                    numberOfMatchedBits,
                                                     std::map<uint16_t, bool> startPatternFoundHybridMap)
{
    bool correctFrameFound = true;
    for(auto theOpticalGroup: *theBoardContainer)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            auto& theCounterVector     = theHybrid->getSummary<GenericDataArray<uint64_t, 2>>();
            auto& theBitCounterCounter = theCounterVector.at(0);
            if(!startPatternFoundHybridMap.at(theHybrid->getId()))
                theBitCounterCounter = 0;
            else
                theBitCounterCounter = readNumberOfTestedBit(theHybrid->getId(), lineNumber, is10Gmodule);
            if(theBitCounterCounter < numberOfMatchedBits)
            {
                correctFrameFound = false;
                if(!fSuppressErrorPrintout)
                    LOG(ERROR) << ERROR_FORMAT << "Number of checked bits 0x" << std::hex << theBitCounterCounter << std::dec << " is less then the expected number " << numberOfMatchedBits
                               << " for OpticalGroup " << theHybrid->getId() / 2 << " Hybrid " << theHybrid->getId() << " line " << +lineNumber << RESET;
            }
            if(theBitCounterCounter > numberOfMatchedBits * 100 && numberOfMatchedBits > 9e5)
            {
                correctFrameFound = false;
                if(!fSuppressErrorPrintout)
                    LOG(ERROR) << ERROR_FORMAT << "Number of checked bits 0x" << std::hex << theBitCounterCounter << std::dec << " is too large than the expected number " << numberOfMatchedBits
                               << " for OpticalGroup " << theHybrid->getId() / 2 << " Hybrid " << theHybrid->getId() << " line " << +lineNumber << RESET;
            }
        }
    }

    return correctFrameFound;
}

void D19cBERTinterface::retrieveErrorCounter(OpticalGroupDataContainer* theOpticalGroupContainer, uint8_t numberOfLines)
{
    for(auto theHybrid: *theOpticalGroupContainer)
    {
        auto& theCounterVector = theHybrid->getSummary<std::vector<GenericDataArray<uint64_t, 2>>>();
        for(uint8_t line = 0; line < numberOfLines; ++line) { theCounterVector.at(line).at(1) = getBitErrorCounters(theHybrid->getId(), line); }
    }
}

void D19cBERTinterface::retrieveErrorCounterLine(BoardDataContainer* theBoardContainer, uint8_t lineNumber)
{
    for(auto theOpticalGroup: *theBoardContainer)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            auto& theCounter = theHybrid->getSummary<GenericDataArray<uint64_t, 2>>();
            theCounter.at(1) = getBitErrorCounters(theHybrid->getId(), lineNumber);
        }
    }
}

BoardDataContainer D19cBERTinterface::runBERTonSingleLine(BoardContainer* theBoardContainer, uint8_t lineNumber, bool is10Gmodule, float numberOfMatchedBits)
{
    fLineSelect = lineNumber;

    uint8_t hybridId = 0x1F;
    uint8_t lineId   = 0xF;

    BoardDataContainer theBoardBERTcounterResult;
    ContainerFactory::copyAndInitHybrid<GenericDataArray<uint64_t, 2>>(*theBoardContainer, theBoardBERTcounterResult);

    std::tuple<bool, bool, std::map<uint16_t, bool>> isStartFound;
    size_t                                           maxNumberOfIteration = 1;
    size_t                                           iterationNumber      = 0;
    while(iterationNumber < maxNumberOfIteration)
    {
        ++iterationNumber;
        loadAllCheckedPatternsInBoard(theBoardContainer);

        startPatternSyncronization(hybridId, lineId);

        isStartFound = isStartPatternFound(theBoardContainer, lineNumber);
        if(std::get<0>(isStartFound) || (iterationNumber >= maxNumberOfIteration && std::get<1>(isStartFound)))
        {
            startBitErrorRateTest(hybridId, lineId);
            if(isStateMachineStarted(theBoardContainer, lineNumber)) { break; }
        }
        if(iterationNumber >= maxNumberOfIteration) break;
        stopBitErrorRateTest(hybridId, lineId);
        haltBitErrorRateTest(hybridId, lineId);
    }

    if(iterationNumber >= maxNumberOfIteration && !fSuppressErrorPrintout)
    {
        LOG(ERROR) << ERROR_FORMAT << "Failed to properly start BERT on line " << +lineNumber << " after " << maxNumberOfIteration << " trials" << RESET;
    }

    if(std::get<1>(isStartFound))
    {
        waitForNeededBits(is10Gmodule, numberOfMatchedBits);

        stopBitErrorRateTest(hybridId, lineId);

        retrieveErrorCounterLine(&theBoardBERTcounterResult, lineNumber);
        retrieveBitTestedCounterLine(&theBoardBERTcounterResult, lineNumber, is10Gmodule, numberOfMatchedBits, std::get<2>(isStartFound));

        haltBitErrorRateTest(hybridId, lineId);
    }

    return theBoardBERTcounterResult;
}

void D19cBERTinterface::setCheckedPattern(uint8_t hybridId, const std::vector<uint32_t>& theCheckedPattern)
{
    if(theCheckedPattern.size() != 4)
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] size of Pattern " << theCheckedPattern.size() << " does not match expected size 4. Aborting..." << std::endl;
        abort();
    }
    fCheckedPatternMap[hybridId] = theCheckedPattern;
}

void D19cBERTinterface::setCheckedPatternMask(uint8_t hybridId, const std::vector<uint32_t>& theCheckedPatternMask)
{
    if(theCheckedPatternMask.size() != 4)
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] size of Pattern Mask " << theCheckedPatternMask.size() << " does not match expected size 4. Aborting..." << std::endl;
        abort();
    }
    fNumberOfCheckedBitsMap[hybridId] = 0;
    for(size_t index = 0; index < theCheckedPatternMask.size(); ++index) { fNumberOfCheckedBitsMap[hybridId] += std::bitset<32>(theCheckedPatternMask.at(index)).count(); }
    fCheckedPatternMaskMap[hybridId] = theCheckedPatternMask;
}

void D19cBERTinterface::loadCheckedPattern(uint8_t hybridId, uint8_t lineId)
{
    BitErrorTestControl setPatternWordIndex;
    setPatternWordIndex.setHybridId(0x1f);
    setPatternWordIndex.setChipId(0x7);
    setPatternWordIndex.setLineId(0xf);
    setPatternWordIndex.setCommand(BitErrorTestControl::Command::Configure);
    setPatternWordIndex.setDebugMode(true);

    BitErrorTestControl setPatternCommand;
    setPatternCommand.setHybridId(hybridId);
    setPatternCommand.setChipId(0);
    setPatternCommand.setLineId(lineId);
    setPatternCommand.setCommand(BitErrorTestControl::Command::ReadBERTfirstData);

    auto theCheckedPattern     = fCheckedPatternMap.at(hybridId);
    auto theCheckedPatternMask = fCheckedPatternMaskMap.at(hybridId);

    for(size_t index = 0; index < theCheckedPattern.size(); ++index)
    {
        for(size_t subPattern = 0; subPattern < 2; ++subPattern)
        {
            uint16_t currentPattern     = (theCheckedPattern.at(theCheckedPattern.size() - index - 1) >> (16 * subPattern)) & 0xFFFF;
            uint16_t currentPatternMask = (theCheckedPatternMask.at(theCheckedPatternMask.size() - index - 1) >> (16 * subPattern)) & 0xFFFF;

            setPatternWordIndex.setPatternWordIndex(index * 2 + subPattern);

            setPatternWordIndex.setIsMask(false);
            setPatternWordIndex.setDebugMode(true);
            writeCommand(setPatternWordIndex);
            setPatternCommand.setPattern(currentPattern);

            writeCommand(setPatternCommand);

            setPatternWordIndex.setIsMask(true);
            setPatternWordIndex.setDebugMode(true);
            writeCommand(setPatternWordIndex);
            setPatternCommand.setPattern(currentPatternMask);
            writeCommand(setPatternCommand);
        }
    }

    setPatternWordIndex.setIsMask(false);
    setPatternWordIndex.setDebugMode(false);
    writeCommand(setPatternWordIndex);
}

void D19cBERTinterface::loadAllCheckedPatternsInBoard(BoardContainer* theBoardContainer)
{
    if(!fUsePRBS)
    {
        for(auto theOpticalGroup: *theBoardContainer)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                loadAlignmentPattern(theHybrid->getId(), fLineSelect);
                loadCheckedPattern(theHybrid->getId(), fLineSelect);
            }
        }
    }
    else { loadAlignmentPattern(0x1F, 0xF); }
}
