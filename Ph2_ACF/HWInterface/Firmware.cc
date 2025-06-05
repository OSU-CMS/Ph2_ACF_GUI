#include "HWInterface/Firmware.h"

#include <arpa/inet.h>
#include <fstream>
#include <sstream>

#include "uhal/log/log.hpp"

#include <boost/date_time/local_time/local_time.hpp>

bool operator==(const fc7::Firmware& aFirmware1, const fc7::Firmware& aFirmware2) { return (aFirmware1.mBitStream == aFirmware2.mBitStream); }

bool operator!=(const fc7::Firmware& aFirmware1, const fc7::Firmware& aFirmware2) { return (aFirmware1.mBitStream != aFirmware2.mBitStream); }

std::ostream& operator<<(std::ostream& aStream, const fc7::Firmware& aFirmware)
{
    aStream << std::dec << "File              : " << aFirmware.mFileName << '\n'
            << "Bit-stream Length : " << aFirmware.mBitStream.size() << " bytes" << '\n'
            << "Bit-swapped       : " << (aFirmware.mBitSwapped ? "True" : "False") << std::endl;
    return aStream;
}

std::ostream& operator<<(std::ostream& aStream, const fc7::XilinxBitFile& aBitFile)
{
    aStream << std::dec << "File              : " << aBitFile.mFileName << '\n'
            << "Design Name       : " << aBitFile.mDesignName << '\n'
            << "Device Name       : " << aBitFile.mDeviceName << '\n'
            << "Build time        : " << aBitFile.mTimeStamp << '\n'
            << "Bit-stream Length : " << aBitFile.mBitStream.size() << " bytes" << '\n'
            << "Bit-swapped       : " << (aBitFile.mBitSwapped ? "True" : "False") << std::endl;
    return aStream;
}

namespace fc7
{
//! Default Target-specified Constructor
Firmware::Firmware(const std::string& aFileName) : mFileName(aFileName) {}

//! Default Destructor
Firmware::~Firmware() {}

const std::vector<uint8_t>& Firmware::Bitstream() const { return mBitStream; }

const std::string& Firmware::FileName() const { return mFileName; }

const bool& Firmware::isBitSwapped() const { return mBitSwapped; }

void Firmware::BitSwap()
{
    for(std::vector<uint8_t>::iterator lIt = mBitStream.begin(); lIt != mBitStream.end(); ++lIt) { *lIt = mLUT[*lIt]; }

    mBitSwapped = !mBitSwapped;
}

const uint8_t Firmware::mLUT[] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff};

//! Default Target-specified Constructor
XilinxBitStream::XilinxBitStream() : Firmware("A Bit Stream") {}

void XilinxBitStream::BigEndianAppend(std::vector<uint32_t>::const_iterator aStart, const std::vector<uint32_t>::const_iterator& aEnd)
{
    for(; aStart != aEnd; ++aStart)
    {
        uint32_t lTemp(ntohl(*aStart));
        uint8_t* lPtr = (uint8_t*)(&lTemp);

        for(int i = 0; i != 4; ++i, ++lPtr) { mBitStream.push_back(*lPtr); }
    }
}

//! Default Destructor
XilinxBitStream::~XilinxBitStream() {}

//! Default Target-specified Constructor
XilinxBinFile::XilinxBinFile(const std::string& aFileName) : Firmware(aFileName)
{
    using namespace uhal;
    std::string lExtension = mFileName.substr(mFileName.size() - 3);
    std::transform(lExtension.begin(), lExtension.end(), lExtension.begin(), ::tolower);

    if(lExtension != "bin")
    {
        log(Error(), Quote(mFileName), " is not a .bin file");
        throw WrongFileExtension();
    }

    std::size_t          lSize;
    std::vector<uint8_t> lFileMemory;
    std::ifstream        lFileStr(mFileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    if(lFileStr.is_open())
    {
        lSize = lFileStr.tellg();
        lFileMemory.resize(lSize);
        lFileStr.seekg(0, std::ios::beg);
        lFileStr.read((char*)(&lFileMemory.front()), lSize);
        lFileStr.close();
    }
    else
    {
        log(Error(), "File ", Quote(mFileName), " not found.");
        throw FileNotFound();
    }

    // skip through the padding
    uint32_t                       lUint(0);
    std::vector<uint8_t>::iterator lIt;

    for(lIt = lFileMemory.begin(); lIt != lFileMemory.end(); ++lIt)
    {
        // lUint = (  ( *lIt ) <<24 ) | (  ( * ( lIt+1 ) ) <<16 ) | (  ( * ( lIt+2 ) ) <<8 ) | (  ( * ( lIt+3 ) ) );
        lUint = (lUint << 8) | (*lIt);
        // std::cout << std::hex << std::setfill('0') << std::setw(8) << lUint << std::endl;

        if((lUint == 0xaa995566) || (lUint == 0x5599aa66)) { break; }
    }

    if(lIt == lFileMemory.end())
    {
        log(Error(), "Corrupted .bin file");
        throw CorruptedFile();
    }

    mBitStream = lFileMemory;

    if(lUint == 0x5599aa66) { mBitSwapped = true; }
    else { mBitSwapped = false; }
}

//! Default Destructor
XilinxBinFile::~XilinxBinFile() {}

//! Default Target-specified Constructor
XilinxBitFile::XilinxBitFile::XilinxBitFile(const std::string& aFileName) : Firmware(aFileName)
{
    using namespace uhal;
    std::string lExtension = mFileName.substr(mFileName.size() - 3);
    std::transform(lExtension.begin(), lExtension.end(), lExtension.begin(), ::tolower);

    if(lExtension != "bit")
    {
        log(Error(), Quote(mFileName), " is not a .bit file");
        throw WrongFileExtension();
    }

    std::vector<uint8_t> lFileMemory;
    std::ifstream        lFileStr(mFileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    if(lFileStr.is_open())
    {
        std::size_t lSize = lFileStr.tellg();
        lFileMemory.resize(lSize);
        lFileStr.seekg(0, std::ios::beg);
        lFileStr.read((char*)(&lFileMemory.front()), lSize);
        lFileStr.close();
    }
    else
    {
        log(Error(), "File ", Quote(mFileName), " not found.");
        throw FileNotFound();
    }

    std::vector<uint8_t>::iterator lIt = lFileMemory.begin();
    uint16_t                       lByteCount;
    std::string                    lTempStr;
    std::string                    lDate, lTime;
    uint32_t                       lBitStreamLength;
    // random xilinx header
    parse(lIt, lByteCount, lTempStr);
    // design name
    parse(lIt, 'a', lByteCount, mDesignName);
    // second key + device name
    parse(lIt, 'b', lByteCount, mDeviceName);
    // third key + build date
    parse(lIt, 'c', lByteCount, lDate);
    // fourth key + build time
    parse(lIt, 'd', lByteCount, lTime);
    // fifth key + bitstream length
    parse(lIt, 'e', lByteCount, lBitStreamLength);
    // convert timestrings to time stamp
    std::stringstream ss;
    ss << lDate << " " << lTime;
    ss.imbue(std::locale(std::locale::classic(), new boost::local_time::local_time_input_facet("%Y/%m/%d %H:%M:%S")));
    ss.exceptions(std::ios::failbit);
    ss >> mTimeStamp;
    mBitStream  = std::vector<uint8_t>(lIt, lIt + lBitStreamLength);
    mBitSwapped = false;
}

//! Default Destructor

XilinxBitFile::~XilinxBitFile() {}

const std::string& XilinxBitFile::DesignName() const { return mDesignName; }

const std::string& XilinxBitFile::DeviceName() const { return mDeviceName; }

const boost::posix_time::ptime& XilinxBitFile::TimeStamp() const { return mTimeStamp; }

std::string XilinxBitFile::StandardizedFileName() const { return std::string(boost::posix_time::to_iso_string(mTimeStamp) + ".bin"); }

void XilinxBitFile::parse(std::vector<uint8_t>::iterator& aIt, uint16_t& aByteCount, std::string& aString)
{
    aByteCount = ((*aIt++) << 8) | (*aIt++);
    aString    = std::string((char*)(&(*aIt)), aByteCount);
    aIt += aByteCount;
    aIt += 2; // take into account the next byte count
}

void XilinxBitFile::parse(std::vector<uint8_t>::iterator& aIt, const char& aExpectedDelimeter, uint16_t& aByteCount, std::string& aString)
{
    using namespace uhal;

    if(*aIt++ != aExpectedDelimeter)
    {
        log(Error(), "Corrupted .bit file");
        throw CorruptedFile();
    }

    aByteCount = ((*aIt++) << 8) | *aIt++;
    aString    = std::string((char*)(&(*aIt)), aByteCount - 1);
    aIt += aByteCount;
}

void XilinxBitFile::parse(std::vector<uint8_t>::iterator& aIt, const char& aExpectedDelimeter, uint16_t& aByteCount, uint32_t& aUint)
{
    using namespace uhal;

    if(*aIt++ != aExpectedDelimeter)
    {
        log(Error(), "Corrupted .bit file");
        throw CorruptedFile();
    }

    aByteCount = 4;
    aUint      = ((*aIt++) << 24) | ((*aIt++) << 16) | ((*aIt++) << 8) | ((*aIt++));
}
} // namespace fc7
