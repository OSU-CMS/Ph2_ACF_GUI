/**
   @file
   @author Andrew W. Rose
   @date 2011
*/

#ifndef Firmware_h
#define Firmware_h

#include <iostream>
#include <string>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "uhal/log/exception.hpp"

namespace fc7
{
class Firmware;
class XilinxBitFile;
} // namespace fc7

bool          operator==(const fc7::Firmware& aFirmware1, const fc7::Firmware& aFirmware2);
bool          operator!=(const fc7::Firmware& aFirmware1, const fc7::Firmware& aFirmware2);
std::ostream& operator<<(std::ostream& aStream, const fc7::Firmware& aFirmware);
std::ostream& operator<<(std::ostream& aStream, const fc7::XilinxBitFile& aBitFile);

namespace fc7
{
UHAL_DEFINE_EXCEPTION_CLASS(WrongFileExtension, "File has the wrong file-extension for the class trying to open it")
UHAL_DEFINE_EXCEPTION_CLASS(FileNotFound, "File was not found")
UHAL_DEFINE_EXCEPTION_CLASS(CorruptedFile, "File was corrupted")

/**
   Firmware is

   @author Andrew W. Rose
   @date 2011
*/
class Firmware
{
    friend std::ostream&(::operator<<)(std::ostream& aStream, const Firmware& aFirmware);
    friend bool(::operator==)(const Firmware& aFirmware1, const Firmware& aFirmware2);
    friend bool(::operator!=)(const Firmware& aFirmware1, const Firmware& aFirmware2);

  public:
    //! Default Target-specified Constructor
    Firmware(const std::string& aFileName);

    //! Default Destructor
    virtual ~Firmware();

    const std::vector<uint8_t>& Bitstream() const;

    const std::string& FileName() const;

    const bool& isBitSwapped() const;

    void BitSwap();

  protected:
    std::string          mFileName;
    std::vector<uint8_t> mBitStream;
    bool                 mBitSwapped;

  private:
    static const uint8_t mLUT[];
};

class XilinxBitStream : public Firmware
{
  public:
    //! Default Target-specified Constructor
    XilinxBitStream();

    //         void BPIappend( std::vector<uint32_t>::const_iterator aStart, const
    //         std::vector<uint32_t>::const_iterator& aEnd );
    void BigEndianAppend(std::vector<uint32_t>::const_iterator aStart, const std::vector<uint32_t>::const_iterator& aEnd);

    //! Default Destructor
    virtual ~XilinxBitStream();
};

/**
   XilinxBitFile is

   @author Andrew W. Rose
   @date 2011
*/
class XilinxBitFile : public Firmware
{
    friend std::ostream&(::operator<<)(std::ostream& aStream, const XilinxBitFile& aBitFile);

  public:
    //! Default Target-specified Constructor
    XilinxBitFile(const std::string& aFileName);

    //! Default Destructor
    virtual ~XilinxBitFile();

    const std::string&              DesignName() const;
    const std::string&              DeviceName() const;
    const boost::posix_time::ptime& TimeStamp() const;

    std::string StandardizedFileName() const;

  private:
    void parse(std::vector<uint8_t>::iterator& aIt, uint16_t& aByteCount, std::string& aString);
    void parse(std::vector<uint8_t>::iterator& aIt, const char& aExpectedDelimeter, uint16_t& aByteCount, std::string& aString);
    void parse(std::vector<uint8_t>::iterator& aIt, const char& aExpectedDelimeter, uint16_t& aByteCount, uint32_t& aUint);

    std::string              mDesignName;
    std::string              mDeviceName;
    boost::posix_time::ptime mTimeStamp;
};

/**
   XilinxBinFile is
   @author Andrew W. Rose
   @date 2011
*/
class XilinxBinFile : public Firmware
{
  public:
    //! Default Target-specified Constructor
    XilinxBinFile(const std::string& aFileName);

    //! Default Destructor
    virtual ~XilinxBinFile();
};
} // namespace fc7

#endif
