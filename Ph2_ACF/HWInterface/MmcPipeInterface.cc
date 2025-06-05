#include <arpa/inet.h>
// FC7 Headers
#include "HWInterface/MmcPipeInterface.h"

// uHal Headers
#include "uhal/log/log.hpp"

namespace fc7
{
UHAL_REGISTER_DERIVED_NODE(MmcPipeInterface)

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// PUBLIC METHODS

MmcPipeInterface::MmcPipeInterface(const uhal::Node& node) : uhal::Node(node) {}

MmcPipeInterface::~MmcPipeInterface() {}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// PRIVATE METHODS

void MmcPipeInterface::Send(const uint32_t& aHeader)
{
    std::vector<uint32_t> lVector;
    lVector.push_back(aHeader);
    lVector.push_back(0);
    UpdateCounters();

    while(FPGAtoMMCSpaceAvailable() < lVector.size())
    {
        usleep(1000); // Otherwise we get serious bus contention
        UpdateCounters();
    }

    this->getNode("FIFO").writeBlock(lVector);
    this->getClient().dispatch();
}

void MmcPipeInterface::Send(const uint32_t& aHeader, const uint32_t& aSizeInWords, const uint32_t* aPayload)
{
    std::vector<uint32_t> lVector;
    lVector.push_back(aHeader);
    lVector.push_back(aSizeInWords);

    for(uint32_t i = 0; i != aSizeInWords; ++i) { lVector.push_back(*aPayload++); }

    UpdateCounters();

    while(FPGAtoMMCSpaceAvailable() < lVector.size())
    {
        usleep(1000); // Otherwise we get serious bus contention
        UpdateCounters();
    }

    this->getNode("FIFO").writeBlock(lVector);
    this->getClient().dispatch();
}

void MmcPipeInterface::Send(const uint32_t& aHeader, const uint32_t& aSizeInBytes, const char* aPayload)
{
    uint32_t              lSize((uint32_t)(ceil(aSizeInBytes / 4.0)));
    std::vector<uint32_t> lVector;
    lVector.push_back(aHeader);
    lVector.push_back(lSize);
    uint32_t* lPayload((uint32_t*)aPayload);

    for(uint32_t i = 0; i != lSize; ++i) { lVector.push_back(htonl(*lPayload++)); }

    UpdateCounters();

    while(FPGAtoMMCSpaceAvailable() < lVector.size())
    {
        usleep(1000); // Otherwise we get serious bus contention
        UpdateCounters();
    }

    this->getNode("FIFO").writeBlock(lVector);
    this->getClient().dispatch();
}

std::vector<uint32_t> MmcPipeInterface::Receive()
{
    std::vector<uint32_t> lRet;
    UpdateCounters();

    while(MMCtoFPGADataAvailable() < 2)
    {
        usleep(1000); // Otherwise we get serious bus contention
        UpdateCounters();
    }

    auto lHeader = this->getNode("FIFO").readBlock(2);
    this->getClient().dispatch();

    if(lHeader[1])
    {
        while(MMCtoFPGADataAvailable() < lHeader[1])
        {
            usleep(1000); // Otherwise we get serious bus contention
            UpdateCounters();
        }

        auto lPayload = this->getNode("FIFO").readBlock(lHeader[1]);
        this->getClient().dispatch();
        lRet = lPayload.value();
    }

    if(lHeader[0])
    {
        uhal::exception::ReplyIndicatesError lExc;
        uhal::log(lExc, ConvertString(lRet.begin(), lRet.end()));
        throw lExc;
    }

    return lRet;
}

std::string MmcPipeInterface::ConvertString(std::vector<uint32_t>::const_iterator aStart, const std::vector<uint32_t>::const_iterator& aEnd)
{
    std::string lRet;
    lRet.reserve((aEnd - aStart) * 4);

    for(; aStart != aEnd; ++aStart)
    {
        uint32_t lTemp(ntohl(*aStart));
        char*    lPtr = (char*)(&lTemp);

        for(int i = 0; i != 4; ++i, ++lPtr)
        {
            if(!*lPtr) { break; }

            lRet.push_back(*lPtr);
        }
    }

    return lRet;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// PRIVATE METHODS

void MmcPipeInterface::EnterSecureMode(const std::string& aPassword)
{
    if(aPassword.size() > 31) { throw uhal::exception::TextExceedsSpaceAvailable(); }

    if(aPassword.size())
    {
        // Result of c_str remains valid unless a non-const member function is called on the string object
        Send(0x00000000, aPassword.size() + 1, aPassword.c_str()); // Size in bytes
    }
    else { Send(0x00000001, 0, (const char*)(NULL)); }

    Receive();
}

void MmcPipeInterface::SetTextSpace(const std::string& aStr)
{
    if(aStr.size() > 31) { throw uhal::exception::TextExceedsSpaceAvailable(); }

    if(aStr.size())
    {
        // Result of c_str remains valid unless a non-const member function is called on the string object
        Send(0x00000001, aStr.size() + 1, aStr.c_str()); // Size in bytes
    }
    else { Send(0x00000001, 0, (const char*)(NULL)); }

    Receive();
}

std::string MmcPipeInterface::GetTextSpace()
{
    Send(0x00000022);

    std::vector<uint32_t>                 lVector(Receive());
    std::vector<uint32_t>::const_iterator lStart(lVector.begin());
    std::string                           lRet = ConvertString(lStart, lStart + 8);

    return lRet;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// PUBLIC METHODS

void MmcPipeInterface::SetDummySensor(const uint8_t& aValue)
{
    uint32_t lValue(aValue);
    Send(0x00000002, 1, &lValue); // Size in words
    Receive();
}

void MmcPipeInterface::FileToSD(const std::string& aFilename, Firmware& aFirmware, uint32_t* pProgress, std::string* pProgressStr)
{
    SetTextSpace(aFilename);
    //     if( aFilename == "GoldenImage.bin" )
    //     {
    //       throw uhal::exception::GoldenImageIsInvolateError();
    //     }
    uint32_t lTotalSize(40000 * 512 / 4); // 40000 sectors of size 512 bytes, converted to 32-bit words
    // Make some space for preparing the data
    std::vector<uint32_t> lVector;
    lVector.reserve(512); // FIFO is of length 512 words
    // Send the header for the transfer
    lVector.push_back(0x00000010);
    lVector.push_back(lTotalSize);
    this->getNode("FIFO").writeBlock(lVector);
    this->getClient().dispatch();

    // Firmware needs to be bitswapped on the SD card
    if(!aFirmware.isBitSwapped()) { aFirmware.BitSwap(); }

    // Cast the data to 32bit form
    std::vector<uint32_t>                lSrcData(lTotalSize, 0xFFFFFFFF);
    std::vector<uint32_t>::iterator      lWriteIt(lSrcData.begin());
    std::vector<uint8_t>::const_iterator lIt(aFirmware.Bitstream().begin());

    for(; lIt != aFirmware.Bitstream().end(); lIt += 4, ++lWriteIt) { *lWriteIt = htonl(*(uint32_t*)(&(*lIt))); }

    // Send the data in chunks
    if(pProgressStr) pProgressStr->assign("Loading firmware image");
    uint32_t i(0);

    for(std::vector<uint32_t>::iterator lBegin(lSrcData.begin()), lEnd(lSrcData.begin()); lEnd != lSrcData.end(); lBegin = lEnd)
    {
        UpdateCounters();

        if(MMCtoFPGADataAvailable()) { break; }

        if(FPGAtoMMCSpaceAvailable())
        {
            if(lBegin + FPGAtoMMCSpaceAvailable() < lSrcData.end()) { lEnd = lBegin + FPGAtoMMCSpaceAvailable(); }
            else { lEnd = lSrcData.end(); }

            // std::cout<<"begin: "<<std::dec<<distance(lSrcData.begin(),lBegin)<<", end:
            // "<<distance(lSrcData.begin(),lEnd)<<" ("<<distance(lBegin, lEnd)<<")"<<std::endl;
            lVector.assign(lBegin, lEnd);
            this->getNode("FIFO").writeBlock(lVector);
            this->getClient().dispatch();

            if(!(i++ % 500) && pProgress) { *pProgress = 100 - (lSrcData.end() - lEnd) * 100 / lTotalSize; }
        }
        //       else{
        //         usleep ( 1000 ); //Otherwise we get serious bus contention
        //       }
    }

    if(pProgressStr) pProgressStr->assign("Done loading firmware image");
    Receive();
    // std::cout<<"Out of Receive()"<<std::endl;
}

XilinxBitStream MmcPipeInterface::FileFromSD(const std::string& aFilename, uint32_t* pProgress, uint32_t uOffset)
{
    SetTextSpace(aFilename);
    Send(0x00000009);

    XilinxBitStream lFirmware;

    while(MMCtoFPGADataAvailable() < 2)
    {
        usleep(1000); // Otherwise we get serious bus contention
        UpdateCounters();
    }

    auto lHeader = this->getNode("FIFO").readBlock(2);
    this->getClient().dispatch();

    std::vector<uint32_t> lRet;
    lRet.reserve(5120000);
    uint32_t lWordCount = lHeader[1], lTot = lWordCount;

    uint32_t i(0);

    while(lWordCount)
    {
        UpdateCounters();

        if(MMCtoFPGADataAvailable())
        {
            size_t wordCount;
            if(lWordCount < MMCtoFPGADataAvailable())
            {
                wordCount  = lWordCount;
                lWordCount = 0;
            }
            else
            {
                wordCount = MMCtoFPGADataAvailable();
                lWordCount -= MMCtoFPGADataAvailable();
            }
            auto lPayload = this->getNode("FIFO").readBlock(wordCount);

            this->getClient().dispatch();

            lRet.insert(lRet.end(), lPayload.begin(), lPayload.end());

            if(!(i++ % 500) && pProgress) { *pProgress = 33 - lWordCount * 33 / lTot + uOffset; }
        }
        else
        {
            usleep(1000); // Otherwise we get serious bus contention
        }
    }

    if(lHeader[0])
    {
        uhal::exception::ReplyIndicatesError lExc;
        uhal::log(lExc, ConvertString(lRet.begin(), lRet.end()));
        throw lExc;
    }

    lFirmware.BigEndianAppend(lRet.begin(), lRet.end());

    return lFirmware;
}

void MmcPipeInterface::RebootFPGA(const std::string& aFilename, const std::string& aPassword)
{
    SetTextSpace(aFilename);
    EnterSecureMode(aPassword);
    Send(0x00000011);

    try
    {
        Receive();
    }
    catch(std::exception& aExc)
    {
        uhal::log(uhal::Info, "Exception seen as expected");
    }

    while(true)
    {
        try
        {
            UpdateCounters();
            break;
        }
        catch(std::exception& aExc)
        {
        }
    }

    if(GetTextSpace() != aFilename) { uhal::log(uhal::Error, "Failed to configure FPGA with image - defaulted to GoldenImage.bin"); }
}

void MmcPipeInterface::BoardHardReset(const std::string& aPassword)
{
    EnterSecureMode(aPassword);
    Send(0x00000021);

    try
    {
        Receive();
    }
    catch(std::exception& aExc)
    {
        uhal::log(uhal::Info, "Exception seen as expected");
    }

    while(true)
    {
        try
        {
            UpdateCounters();
            break;
        }
        catch(std::exception& aExc)
        {
        }
    }
}

void MmcPipeInterface::DeleteFromSD(const std::string& aFilename, const std::string& aPassword)
{
    //     if( aFilename == "GoldenImage.bin" )
    //     {
    //       throw uhal::exception::GoldenImageIsInvolateError();
    //     }
    SetTextSpace(aFilename);
    EnterSecureMode(aPassword);
    Send(0x00000012);
    Receive();
}

std::vector<std::string> MmcPipeInterface::ListFilesOnSD()
{
    Send(0x00000013);
    std::vector<uint32_t>                 lVector(Receive());
    std::vector<std::string>              lRet;
    std::vector<uint32_t>::const_iterator lStart(lVector.begin());

    for(uint32_t i(0); i != lVector.size() >> 3; ++i)
    {
        lRet.push_back(ConvertString(lStart, lStart + 8));
        lStart += 8;
    }

    return lRet;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// PUBLIC METHODS

void MmcPipeInterface::UpdateCounters()
{
    uhal::ValWord<uint32_t> lFPGAtoMMCcounters = this->getNode("FPGAtoMMCcounters").read();
    uhal::ValWord<uint32_t> lMMCtoFPGAcounters = this->getNode("MMCtoFPGAcounters").read();
    this->getClient().dispatch();
    mFPGAtoMMCDataAvailable  = (((lFPGAtoMMCcounters >> 16) & 0x0000FFFF) - ((lFPGAtoMMCcounters >> 1) & 0x00007FFF) + 1) % 512;
    mFPGAtoMMCSpaceAvailable = 511 - mFPGAtoMMCDataAvailable;
    mMMCtoFPGADataAvailable  = (((lMMCtoFPGAcounters >> 1) & 0x00007FFF) - ((lMMCtoFPGAcounters >> 16) & 0x0000FFFF) + 1) % 512;
    mMMCtoFPGASpaceAvailable = 511 - mMMCtoFPGADataAvailable;
    // std::cout << std::dec << "mFPGAtoMMCDataAvailable:" << mFPGAtoMMCDataAvailable << "\tmFPGAtoMMCSpaceAvailable:"
    // << mFPGAtoMMCSpaceAvailable << "\tmMMCtoFPGADataAvailable:" << mMMCtoFPGADataAvailable <<
    // "\tmMMCtoFPGASpaceAvailable:" << mMMCtoFPGASpaceAvailable <<std::endl;//"\tlFpgaToMmc:"<< std::hex <<
    // lFPGAtoMMCcounters<<"\tlMmcToFpga:"<<  lMMCtoFPGAcounters<<std::endl;
}

const uint16_t& MmcPipeInterface::FPGAtoMMCDataAvailable() { return mFPGAtoMMCDataAvailable; }

const uint16_t& MmcPipeInterface::FPGAtoMMCSpaceAvailable() { return mFPGAtoMMCSpaceAvailable; }

const uint16_t& MmcPipeInterface::MMCtoFPGADataAvailable() { return mMMCtoFPGADataAvailable; }

const uint16_t& MmcPipeInterface::MMCtoFPGASpaceAvailable() { return mMMCtoFPGASpaceAvailable; }

} // namespace fc7
