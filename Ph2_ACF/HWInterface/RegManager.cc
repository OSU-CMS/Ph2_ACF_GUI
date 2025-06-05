/*
  FileName :                    RegManager.cc
  Content :                     RegManager class, permit connection & r/w registers
  Programmer :                  Nicolas PIERRE
  Version :                     1.0
  Date of creation :            06/06/14
  Support :                     mail to : nico.pierre@icloud.com
*/

#include "HWInterface/RegManager.h"
#include "HWDescription/BeBoard.h"
#include "HWDescription/Definition.h"
#include "Utils/ConsoleColor.h"
#include "Utils/Utilities.h"
#include "Utils/easylogging++.h"
#include <thread>
#include <uhal/uhal.hpp>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#define DEV_FLAG 0

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
RegManager::RegManager(const std::string& puHalConfigFileName, uint32_t pBoardId, BeBoard* theBoard) : fUHalConfigFileName(puHalConfigFileName), fTheBoardPointer(theBoard)
{
    if(mode != Mode::Replay)
    {
        uhal::disableLogging();
        uhal::ConnectionManager cm(fUHalConfigFileName.c_str());
        char                    cBuff[7];
        sprintf(cBuff, "board%d", pBoardId);
        fBoard = new uhal::HwInterface(cm.getDevice((cBuff)));
        fBoard->setTimeoutPeriod(10000);
    }
}

RegManager::RegManager(const std::string& pId, const std::string& pUri, const std::string& pAddressTable, BeBoard* theBoard)
    : fBoard(nullptr), fUri(pUri), fAddressTable(pAddressTable), fId(pId), fTheBoardPointer(theBoard)
{
    if(mode != Mode::Replay)
    {
        uhal::disableLogging();
        fBoard = new uhal::HwInterface(uhal::ConnectionManager::getDevice(fId, fUri, fAddressTable));
        fBoard->setTimeoutPeriod(10000);
    }
}

RegManager::RegManager(RegManager&& theRegManager)
    : fUHalConfigFileName(std::move(theRegManager.fUHalConfigFileName))
    , fStackReg(std::move(theRegManager.fStackReg))
    , fUri(std::move(theRegManager.fUri))
    , fAddressTable(std::move(theRegManager.fAddressTable))
    , fId(std::move(theRegManager.fId))
{
    fBoard               = theRegManager.fBoard;
    theRegManager.fBoard = nullptr;
}

RegManager::~RegManager() { delete fBoard; }

bool RegManager::WriteReg(const std::string& pRegNode, const uint32_t& pVal)
{
    std::lock_guard<std::recursive_mutex> theGuard(fMutex);
    if(mode == Mode::Replay) return true;

    fBoard->getNode(pRegNode).write(pVal);
    if(exceptionCatchedBoardDispatch(__PRETTY_FUNCTION__) == false) WriteReg(pRegNode, pVal);
    fTheBoardPointer->setReg(pRegNode, pVal);

    // Verify if the writing is done correctly
    if(DEV_FLAG)
    {
        uhal::ValWord<uint32_t> reply = fBoard->getNode(pRegNode).read();
        fBoard->dispatch();

        uint32_t comp = reply.value();

        if(comp == pVal)
        {
            LOG(DEBUG) << "Values written correctly: " << pRegNode << "=" << pVal;
            return true;
        }

        LOG(DEBUG) << "\nERROR !!\nValues are not consistent:\nExpected : " << pVal << "\nActual: " << comp;
    }

    return false;
}

bool RegManager::WriteStackReg(const std::vector<std::pair<std::string, uint32_t>>& pVecReg)
{
    std::lock_guard<std::recursive_mutex> theGuard(fMutex);
    if(mode == Mode::Replay) return true;

    for(auto const& v: pVecReg)
    {
        fBoard->getNode(v.first).write(v.second);
        fTheBoardPointer->setReg(v.first, v.second);
    }

    if(exceptionCatchedBoardDispatch(__PRETTY_FUNCTION__) == false) WriteStackReg(pVecReg);

    if(DEV_FLAG)
    {
        int      cNbErrors = 0;
        uint32_t comp;

        for(auto const& v: pVecReg)
        {
            uhal::ValWord<uint32_t> reply = fBoard->getNode(v.first).read();
            fBoard->dispatch();

            comp = reply.value();

            if(comp == v.second) LOG(DEBUG) << "Values written correctly: " << v.first << "=" << v.second;
        }

        if(cNbErrors == 0)
        {
            LOG(DEBUG) << "All values written correctly";
            return true;
        }

        LOG(DEBUG) << "\nERROR !!\n" << cNbErrors << " have not been written correctly";
    }

    return false;
}

bool RegManager::WriteBlockReg(const std::string& pRegNode, const std::vector<uint32_t>& pValues)
{
    std::lock_guard<std::recursive_mutex> theGuard(fMutex);
    if(mode == Mode::Replay) return true;

    fBoard->getNode(pRegNode).writeBlock(pValues);
    if(exceptionCatchedBoardDispatch(__PRETTY_FUNCTION__) == false) WriteBlockReg(pRegNode, pValues);

    bool cWriteCorr = true;

    // Verifying block
    if(DEV_FLAG)
    {
        int cErrCount = 0;

        uhal::ValVector<uint32_t> cBlockRead = fBoard->getNode(pRegNode).readBlock(pValues.size());
        fBoard->dispatch();

        // Use size_t and not an iterator as op[] only works with size_t type
        for(std::size_t i = 0; i != cBlockRead.size(); i++)
        {
            if(cBlockRead[i] != pValues.at(i))
            {
                cWriteCorr = false;
                cErrCount++;
            }
        }

        LOG(DEBUG) << "Block Write finished !!\n" << cErrCount << " values failed to write";
    }

    return cWriteCorr;
}

bool RegManager::WriteBlockAtAddress(uint32_t uAddr, const std::vector<uint32_t>& pValues, bool bNonInc)
{
    std::lock_guard<std::recursive_mutex> theGuard(fMutex);
    if(mode == Mode::Replay) return true;

    fBoard->getClient().writeBlock(uAddr, pValues, bNonInc ? uhal::defs::NON_INCREMENTAL : uhal::defs::INCREMENTAL);
    if(exceptionCatchedBoardDispatch(__PRETTY_FUNCTION__) == false) WriteBlockAtAddress(uAddr, pValues, bNonInc);

    bool cWriteCorr = true;

    // Verifying block
    if(DEV_FLAG)
    {
        int cErrCount = 0;

        uhal::ValVector<uint32_t> cBlockRead = fBoard->getClient().readBlock(uAddr, pValues.size(), bNonInc ? uhal::defs::NON_INCREMENTAL : uhal::defs::INCREMENTAL);
        fBoard->dispatch();

        // Use size_t and not an iterator as op[] only works with size_t type
        for(std::size_t i = 0; i != cBlockRead.size(); i++)
        {
            if(cBlockRead[i] != pValues.at(i))
            {
                cWriteCorr = false;
                cErrCount++;
            }
        }

        LOG(DEBUG) << "BlockWriteAtAddress finished !!\n" << cErrCount << " values failed to write";
    }

    return cWriteCorr;
}

uint32_t RegManager::ReadReg(const std::string& pRegNode)
{
    std::lock_guard<std::recursive_mutex> theGuard(fMutex);
    if(mode == Mode::Replay) return replayRead();

    uhal::ValWord<uint32_t> cValRead = fBoard->getNode(pRegNode).read();
    if(exceptionCatchedBoardDispatch(__PRETTY_FUNCTION__) == false) ReadReg(pRegNode);

    if(DEV_FLAG)
    {
        uint32_t read = cValRead.value();
        LOG(DEBUG) << "Value in register ID " << pRegNode << " : " << read;
    }

    if(mode == Mode::Capture) captureRead(cValRead.value());

    fTheBoardPointer->setReg(pRegNode, cValRead.value());

    return cValRead.value();
}

uint32_t RegManager::ReadAtAddress(uint32_t uAddr, uint32_t uMask)
{
    std::lock_guard<std::recursive_mutex> theGuard(fMutex);
    if(mode == Mode::Replay) return replayRead();

    uhal::ValWord<uint32_t> cValRead = fBoard->getClient().read(uAddr, uMask);
    if(exceptionCatchedBoardDispatch(__PRETTY_FUNCTION__) == false) ReadAtAddress(uAddr, uMask);

    if(DEV_FLAG)
    {
        uint32_t read = cValRead.value();
        LOG(DEBUG) << "Value at address " << std::hex << uAddr << std::dec << " : " << read;
    }

    if(mode == Mode::Capture) captureRead(cValRead.value());

    return cValRead.value();
}

std::vector<uint32_t> RegManager::ReadBlockReg(const std::string& pRegNode, const uint32_t& pBlockSize)
{
    std::lock_guard<std::recursive_mutex> theGuard(fMutex);
    if(mode == Mode::Replay) return replayBlockRead(pBlockSize);

    uhal::ValVector<uint32_t> cBlockRead = fBoard->getNode(pRegNode).readBlock(pBlockSize);
    if(exceptionCatchedBoardDispatch(__PRETTY_FUNCTION__) == false) ReadBlockReg(pRegNode, pBlockSize);

    if(DEV_FLAG)
    {
        LOG(DEBUG) << "Values in register block " << pRegNode << " : ";

        for(std::size_t i = 0; i != cBlockRead.size(); i++)
        {
            uint32_t read = static_cast<uint32_t>(cBlockRead[i]);
            LOG(DEBUG) << " " << read << " ";
        }
    }

    if(mode == Mode::Capture) captureBlockRead(cBlockRead.value());

    return cBlockRead.value();
}

std::vector<uint32_t> RegManager::ReadBlockRegOffset(const std::string& pRegNode, const uint32_t& pBlocksize, const uint32_t& pBlockOffset)
{
    std::lock_guard<std::recursive_mutex> theGuard(fMutex);
    if(mode == Mode::Replay) return replayBlockRead(pBlocksize);

    uhal::ValVector<uint32_t> cBlockRead = fBoard->getNode(pRegNode).readBlockOffset(pBlocksize, pBlockOffset);
    if(exceptionCatchedBoardDispatch(__PRETTY_FUNCTION__) == false) ReadBlockRegOffset(pRegNode, pBlocksize, pBlockOffset);

    if(DEV_FLAG)
    {
        LOG(DEBUG) << "Values in register block " << pRegNode << " : ";

        for(std::size_t i = 0; i != cBlockRead.size(); i++)
        {
            uint32_t read = static_cast<uint32_t>(cBlockRead[i]);
            LOG(DEBUG) << " " << read << " ";
        }
    }

    if(mode == Mode::Capture) captureBlockRead(cBlockRead.value());

    return cBlockRead.value();
}

void RegManager::StackReg(const std::string& pRegNode, const uint32_t& pVal, bool pSend)
{
    for(std::vector<std::pair<std::string, uint32_t>>::iterator cIt = fStackReg.begin(); cIt != fStackReg.end(); cIt++)
        if(cIt->first == pRegNode) fStackReg.erase(cIt);

    std::pair<std::string, uint32_t> cPair(pRegNode, pVal);
    fStackReg.push_back(cPair);

    if(pSend || fStackReg.size() == 100)
    {
        WriteStackReg(fStackReg);
        fStackReg.clear();
    }
}

const uhal::Node& RegManager::getUhalNode(const std::string& pStrPath) { return fBoard->getNode(pStrPath); }

bool RegManager::pollRegister(const std::string& pRegisterName, uint32_t pValue, float pMaxWaitTime_s, bool pDebugOut)
{
    float cElapsedTime_s  = 0;
    auto  startTimeUTC_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    bool  cStopCondition  = false;
    while(!cStopCondition)
    {
        auto cRegValue = ReadReg(pRegisterName);
        cStopCondition = (pMaxWaitTime_s == 0) ? (cRegValue == pValue && cElapsedTime_s >= pMaxWaitTime_s) : (cRegValue >= pValue);
        if(cStopCondition) std::this_thread::sleep_for(std::chrono::microseconds(100));
        auto currentTimeUTC_us    = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        cElapsedTime_s            = (float)(currentTimeUTC_us - startTimeUTC_us) * 1e-6;
        int cElapedTime_nearest_s = std::floor(cElapsedTime_s);
        if(!pDebugOut) continue;

        int   barWidth   = 70;
        float percentage = (pMaxWaitTime_s == 0) ? (float)cRegValue / pValue : cElapedTime_nearest_s / pMaxWaitTime_s;
        int   pos        = barWidth * percentage;
        std::cout << BOLDCYAN << "Polling " << pRegisterName << " [ ";
        for(int i = 0; i < barWidth; ++i)
        {
            if(i < pos)
                std::cout << "=";
            else if(i == pos)
                std::cout << ">";
            else
                std::cout << " ";
        }
        if(pMaxWaitTime_s != 0)
            std::cout << " " << cElapedTime_nearest_s << "/" << pMaxWaitTime_s << " seconds]\r" << RESET;
        else
            std::cout << " " << cRegValue << "/" << pValue << " ]\r" << RESET;
        std::cout.flush();
    }
    return cStopCondition;
}

void RegManager::ResetRegManager()
{
    if(fBoard)
    {
        delete fBoard;
        fBoard = new uhal::HwInterface(uhal::ConnectionManager::getDevice(fId, fUri, fAddressTable));
    }
}

// ##############################################
// # Capure and replay data stream to/from FPGA #
// ##############################################
RegManager::Mode                    RegManager::mode = RegManager::Mode::Default;
boost::iostreams::filtering_ostream capture_file{};
boost::iostreams::filtering_istream replay_file{};

void RegManager::enableCapture(const std::string& filename)
{
    capture_file.push(boost::iostreams::gzip_compressor());
    capture_file.push(boost::iostreams::file_sink(filename));
    mode = Mode::Capture;
}

void RegManager::enableReplay(const std::string& filename)
{
    replay_file.push(boost::iostreams::gzip_decompressor());
    replay_file.push(boost::iostreams::file_source(filename));
    mode = Mode::Replay;
}

template <class S, class T>
void read_binary(S& stream, T& data)
{
    stream.read(reinterpret_cast<char*>(&data), sizeof(data));
}

template <class S, class T>
void write_binary(S& stream, const T& data)
{
    stream.write(reinterpret_cast<const char*>(&data), sizeof(data));
}

uint32_t RegManager::replayRead() { return replayBlockRead(1)[0]; }

std::vector<uint32_t> RegManager::replayBlockRead(size_t size)
{
    uint32_t read_size;
    read_binary(replay_file, read_size);

    if(read_size != size)
    {
        LOG(ERROR) << BOLDRED << "Binary data replay error" << RESET;
        throw;
    }

    std::vector<uint32_t> data(read_size);
    for(auto& d: data) read_binary(replay_file, d);

    return data;
}

void RegManager::captureRead(uint32_t value) { captureBlockRead({value}); }

void RegManager::captureBlockRead(std::vector<uint32_t> data)
{
    write_binary(capture_file, uint32_t(data.size()));
    for(const auto& d: data) write_binary(capture_file, d);
}

bool RegManager::exceptionCatchedBoardDispatch(const std::string& callingFunction)
{
    try
    {
        fBoard->dispatch();
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << BOLDRED << callingFunction << ": Exception caught from controlhub: " << BOLDYELLOW << e.what() << std::endl;

        if(fNumberOfErrors < fMaximumAcceptableNumberOfErrors)
        {
            LOG(ERROR) << BOLDRED << "Retrying, caught " << BOLDYELLOW << fNumberOfErrors << BOLDRED << " controlhub exception out of the " << BOLDYELLOW << fMaximumAcceptableNumberOfErrors << BOLDRED
                       << " acceptable" << RESET;
            ++fNumberOfErrors;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            return false;
        }
        else
        {
            LOG(ERROR) << BOLDRED << "Maximum number of acceptable errors (" << BOLDYELLOW << fMaximumAcceptableNumberOfErrors << BOLDRED << ") reached, throwing last exception" << RESET;
            LOG(ERROR) << BOLDRED << "Please contact Fabio Ravera" << RESET;
            fNumberOfErrors = 0; // In case it is decided to catch the exception and continue
            throw;
        }
    }

    fNumberOfErrors = 0;
    return true;
}

} // namespace Ph2_HwInterface
