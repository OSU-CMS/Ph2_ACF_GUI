/*!
        \file                DQMEvent.h
        \brief               A reader class for SLinkEvent
        \author              Suchandra Dutta and Subir Sarkar
        \version             0.8
        \date                05/11/17
        Support :            mail to : Suchandra.Dutta@cern.ch, Subir.Sarkar@cern.ch

*/

#ifndef __DQMEVENT_H__
#define __DQMEVENT_H__

#include <bitset> // std::bitset
#include <climits>
#include <cmath>
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class SLinkEvent;
class DQMEvent;

class ReadoutStatus
{
  public:
    ReadoutStatus(size_t word) : error1_((word >> 19) & 0x1), error2_((word >> 18) & 0x1), plAdd_((word >> 9) & 0x1FF), l1ACounter_(word & 0x1FF) {}
    bool     error1() const { return error1_; }
    bool     error2() const { return error2_; }
    uint16_t PA() const { return plAdd_; }
    uint16_t l1ACounter() const { return l1ACounter_; }

    void print(bool printHeader = true, std::ostream& os = std::cout) const
    {
        if(printHeader) os << "Error1 Error2 plAdd l1ACounter" << std::endl;
        os << std::setw(6) << error1_ << std::setw(7) << error2_ << std::setw(6) << plAdd_ << std::setw(11) << l1ACounter_ << std::endl;
    }

  private:
    bool     error1_;
    bool     error2_;
    uint16_t plAdd_;
    uint16_t l1ACounter_;
};
class StubInfo
{
  public:
    StubInfo(uint16_t word) : chipId_((word >> 12) & 0xF), address_((word >> 4) & 0xFF), bend_(word & 0xF) {}
    uint8_t chipId() const { return chipId_; }
    uint8_t address() const { return address_; }
    uint8_t bend() const { return bend_; }
    float   channel() const { return (chipId_ * 127 + 0.5 * address_); }
    void    print(bool printHeader = true, std::ostream& os = std::cout) const
    {
        if(printHeader) os << " chipId address channel bend" << std::endl;
        os << std::setw(7) << +chipId_ << std::setw(8) << +address_ << std::setiosflags(std::ios::fixed) << std::setprecision(1) << std::setw(8) << channel() << std::resetiosflags(std::ios::fixed)
           << std::setw(5) << +bend_ << std::endl;
    }

  private:
    uint8_t chipId_;
    uint8_t address_;
    uint8_t bend_;
};
/*!
 * \class DQMEvent
 * \brief A reader class for SLinkEvent
 */
class DQMEvent
{
  public:
    static constexpr unsigned int MAX_FE_PER_FED            = 72;
    static constexpr unsigned int MAX_READOUT_PER_FE        = 16;
    static constexpr unsigned int STRIPS_PER_READOUT        = 254;
    static constexpr unsigned int STRIPS_PADDING            = 2;
    static constexpr unsigned int STRIPS_PER_READOUT_PADDED = STRIPS_PER_READOUT + STRIPS_PADDING;

    static constexpr unsigned int READOUT_STATUS_WORD_WIDTH = 20;
    static constexpr unsigned int NSTUB_WORD_WIDTH          = 5;
    static constexpr unsigned int STUB_WORD_WIDTH           = 16;

    // Masks
    static constexpr unsigned int MASK_BITS_2  = 0x3;
    static constexpr unsigned int MASK_BITS_4  = 0xF;
    static constexpr unsigned int MASK_BITS_8  = 0xFF;
    static constexpr unsigned int MASK_BITS_9  = 0x1FF;
    static constexpr unsigned int MASK_BITS_12 = 0xFFF;
    static constexpr unsigned int MASK_BITS_16 = 0xFFFF;
    static constexpr unsigned int MASK_BITS_24 = 0xFFFFFF;
    static constexpr unsigned int MASK_BITS_30 = 0x3FFFFFFF;
    static constexpr unsigned int MASK_BITS_32 = 0xFFFFFFFF;

    static size_t getWord(const std::vector<bool>& v, size_t ishift, size_t len)
    {
        size_t retval = 0;
        int    i      = 0;
        for(std::vector<bool>::const_iterator it = v.begin() + ishift; it != v.begin() + ishift + len; ++it, ++i) retval |= ((*it) ? 1 : 0) << (len - 1 - i);
        return retval;
    }
    static std::vector<bool> getChannelData(const std::vector<bool>& v, size_t ishift, size_t len = STRIPS_PER_READOUT_PADDED)
    {
        std::vector<bool> retval;
        int               i = 0;
        for(std::vector<bool>::const_iterator it = v.begin() + ishift; it != v.begin() + ishift + len; ++it, ++i) retval.push_back((*it));
        return retval;
    }
    static uint16_t encodeId(const uint8_t& pHybridId, const uint8_t& pCbcId) { return (pHybridId << 8 | pCbcId); }
    static void     decodeId(const uint16_t& pKey, uint8_t& pHybridId, uint8_t& pCbcId)
    {
        pHybridId = (pKey >> 8) & MASK_BITS_8;
        pCbcId    = pKey & MASK_BITS_8;
    }
    // total number of Readout units conencted
    static size_t nReadout(const uint64_t& word) { return ((word >> 8) & MASK_BITS_16); }
    // Number of FEs enabled
    static size_t nFrondEnd(const uint64_t& w1, const uint64_t& w2)
    {
        std::bitset<8>  b(w1 & MASK_BITS_8);
        std::bitset<64> a(w2);

        return (a.count() + b.count());
    }
    static size_t attachedFE(const std::bitset<72>& bs, std::vector<uint8_t>& v)
    {
        v.clear();
        for(size_t i = 0; i < bs.size(); ++i)
            if(bs.test(i)) v.push_back(i);

        return v.size();
    }
    static size_t readoutInFE(const std::bitset<16>& bs, std::vector<uint8_t>& v)
    {
        v.clear();
        for(size_t i = 0; i < bs.size(); ++i)
            if(bs.test(i)) v.push_back(i);

        return v.size();
    }

    // inner classes
    class DAQHeader
    {
      public:
        void set(uint64_t word)
        {
            boe_    = (word >> 60) & MASK_BITS_4;
            evType_ = (word >> 56) & MASK_BITS_4;
            lv1Id_  = (word >> 32) & MASK_BITS_24;
            bxId_   = (word >> 20) & MASK_BITS_12;
            srcId_  = (word >> 8) & MASK_BITS_12;
        }
        uint8_t  BoE() const { return boe_; }
        uint8_t  eventType() const { return evType_; }
        size_t   LV1Id() const { return lv1Id_; }
        uint16_t sourceId() const { return srcId_; }
        uint16_t bxId() const { return bxId_; }

        void print(std::ostream& os = std::cout) const
        {
            os << "[DAQHeader]" << std::endl;
            os << "boe1 evType   lv1Id bxId srcId" << std::endl;
            os << std::setw(4) << +boe_ << std::setw(7) << +evType_ << std::setw(8) << lv1Id_ << std::setw(5) << bxId_ << std::setw(6) << srcId_ << std::endl;
        }

      private:
        uint8_t  boe_;
        uint8_t  evType_;
        size_t   lv1Id_;
        uint16_t srcId_;
        uint16_t bxId_;
    };
    class DAQTrailer
    {
      public:
        void set(uint64_t word)
        {
            evLen_  = (word >> 32) & MASK_BITS_24;
            crc_    = (word >> 16) & MASK_BITS_16;
            evStat_ = (word >> 8) & MASK_BITS_4;
        }
        size_t   eventLen() const { return evLen_; }
        uint16_t crc() const { return crc_; }
        uint8_t  eventStatus() const { return evStat_; }

        void print(std::ostream& os = std::cout) const
        {
            os << "[DAQTrailer]" << std::endl;
            os << "    evLen     crc evStat" << std::endl;
            os << std::setw(9) << evLen_ << std::setw(8) << crc_ << std::setw(7) << +evStat_ << std::endl;
        }

      private:
        size_t   evLen_;
        uint16_t crc_;
        uint8_t  evStat_;
    };
    class TrackerHeader
    {
      public:
        size_t set(const std::vector<uint64_t>& wordList)
        {
            size_t wbit = sizeof(uint64_t) * CHAR_BIT;

            const auto& w1 = wordList.at(1);

            // No of readout chips
            size_t nRO = nReadout(w1);
            version_   = (w1 >> 60) & MASK_BITS_4;
            format_    = (w1 >> 58) & MASK_BITS_2;
            eventType_ = (w1 >> 54) & MASK_BITS_4;
            dtcStatus_ = (w1 >> 24) & MASK_BITS_30;

            // FE Status
            const auto&     w2 = wordList.at(2);
            std::bitset<8>  a(w1 & MASK_BITS_8);
            std::bitset<64> b(w2);

            // Fill the 72 bit word
            for(size_t i = 0; i < b.size(); ++i) feStatus_.set(i, b[i]);
            for(size_t i = 0; i < a.size(); ++i) feStatus_.set(64 + i, a[i]);

            // Readout Status, collect information
            size_t            nbits  = nRO * READOUT_STATUS_WORD_WIDTH;
            size_t            nwords = std::ceil(nbits * 1.0 / wbit);
            std::vector<bool> data;
            for(size_t i = 0; i < nwords; ++i)
            {
                uint64_t w = wordList.at(3 + i);
                for(size_t j = 0; j < wbit; ++j) data.push_back((w >> (wbit - 1 - j)) & 0x1);
            }
            // Now fill information
            for(size_t i = 0; i < nRO; ++i)
            {
                size_t        status = getWord(data, i * READOUT_STATUS_WORD_WIDTH, READOUT_STATUS_WORD_WIDTH);
                ReadoutStatus obj(status);
                roList_.push_back(obj);
            }
            nwords_ = nwords;
            return nwords;
        }
        uint8_t                           version() const { return version_; }
        uint8_t                           format() const { return format_; }
        uint8_t                           evType() const { return eventType_; }
        size_t                            DTCStatus() const { return dtcStatus_; }
        size_t                            nReadoutUnits() const { return roList_.size(); }
        size_t                            nwords() const { return nwords_; }
        const std::bitset<72>&            FEStatus() const { return feStatus_; }
        const std::vector<ReadoutStatus>& readoutStatusList() const { return roList_; }
        const ReadoutStatus&              readoutStatus(int i) const
        {
            // throws an out_of_range exception if i is outside the bounds of valid elements in the vector
            return roList_.at(i);
        }

        void print(std::ostream& os = std::cout) const
        {
            os << "[TrackerHeader]" << std::endl;
            os << "Version Format EvType DTCStatus NReadout " << std::endl;
            os << std::setw(7) << +version_ << std::setw(7) << +format_ << std::setw(7) << std::bitset<4>(eventType_) << std::setw(10) << dtcStatus_ << std::setw(9) << nReadoutUnits() << std::endl;

            // position of the attached FEs
            std::vector<uint8_t> feList;
            attachedFE(feStatus_, feList);
            os << "FE position: <";
            for(size_t i = 0; i < feList.size(); ++i) os << i << " ";

            os << ">" << std::endl;

            // Readout Status
            for(size_t i = 0; i < roList_.size(); ++i) roList_[i].print((i == 0 ? true : false), os);
        }

      private:
        uint8_t                    version_;
        uint8_t                    format_;
        uint8_t                    eventType_;
        size_t                     dtcStatus_;
        size_t                     nwords_;
        std::bitset<72>            feStatus_;
        std::vector<ReadoutStatus> roList_;
    };
    // Tracker payload information. ReadoutStatus is also merged
    class TrackerPayload
    {
      public:
        size_t set(const std::vector<uint64_t>& wordList, size_t ishift, const std::bitset<72>& feStatus, const std::vector<ReadoutStatus>& roStatusList)
        {
            size_t wbit = sizeof(uint64_t) * CHAR_BIT;

            std::vector<uint8_t> feList;
            size_t               nFE  = attachedFE(feStatus, feList);
            size_t               nROU = roStatusList.size();

            size_t            nwords = std::ceil((nFE * MAX_READOUT_PER_FE + nROU * STRIPS_PER_READOUT_PADDED) * 1.0 / wbit);
            std::vector<bool> data;
            for(size_t i = 0; i < nwords; ++i)
            {
                uint64_t w = wordList.at(ishift + i);
                for(size_t j = 0; j < wbit; ++j) data.push_back((w >> (wbit - 1 - j)) & 0x1);
            }
            // now the complicated part
            size_t ipos = nFE * MAX_READOUT_PER_FE;
            for(size_t i = 0; i < nFE; ++i)
            {
                size_t               feh = getWord(data, i * MAX_READOUT_PER_FE, MAX_READOUT_PER_FE);
                std::vector<uint8_t> readoutList;
                uint8_t              nro = readoutInFE(std::bitset<MAX_READOUT_PER_FE>(feh), readoutList);
                feReadoutMappingList_.push_back({feList.at(i), readoutList});
                for(int j = 0; j < nro; ++j)
                {
                    size_t            jpos  = ipos + j * STRIPS_PER_READOUT_PADDED;
                    std::vector<bool> cdata = getChannelData(data, jpos, STRIPS_PER_READOUT_PADDED);
                    ReadoutStatus     roStatus(roStatusList.at(i * nro + j)); // check nro carefully

                    uint16_t cKey = encodeId(feList.at(i), readoutList.at(j));
                    readoutDataMap_.insert({cKey, {roStatus, cdata}});
                }
                ipos += nro * STRIPS_PER_READOUT_PADDED;
            }
            nwords_ = nwords;
            return nwords;
        }
        const std::map<uint16_t, std::pair<ReadoutStatus, std::vector<bool>>>& getMap() const { return readoutDataMap_; }
        const std::vector<std::pair<uint8_t, std::vector<uint8_t>>>&           feReadoutMapping() const { return feReadoutMappingList_; }
        // Get Readout Status Object
        const ReadoutStatus& readoutStatus(uint16_t cKey) const
        {
            // throws an out_of_range exception if the key cKey is not found in the container (map)
            return readoutDataMap_.at(cKey).first;
        }
        const ReadoutStatus& readoutStatus(uint8_t hybridId, uint8_t readoutId) const
        {
            uint16_t cKey = encodeId(hybridId, readoutId);
            return readoutStatus(cKey);
        }
        // Get Readout Channel data
        const std::vector<bool>& channelData(uint16_t cKey) const
        {
            // throws an out_of_range exception if the key cKey is not found in the container (map)
            return readoutDataMap_.at(cKey).second;
        }
        const std::vector<bool>& channelData(uint8_t hybridId, uint8_t readoutId) const
        {
            uint16_t cKey = encodeId(hybridId, readoutId);
            return channelData(cKey);
        }
        // Overall Readout data
        const std::pair<ReadoutStatus, std::vector<bool>>& readoutData(uint16_t cKey) const
        {
            // throws an out_of_range exception if the key cKey is not found in the container (map)
            return readoutDataMap_.at(cKey);
        }
        const std::pair<ReadoutStatus, std::vector<bool>>& readoutData(uint8_t hybridId, uint8_t readoutId) const
        {
            uint16_t cKey = encodeId(hybridId, readoutId);
            return readoutData(cKey);
        }
        size_t nwords() const { return nwords_; }
        void   print(std::ostream& os = std::cout) const
        {
            os << "[TrackerPayload]" << std::endl;
            for(std::map<uint16_t, std::pair<ReadoutStatus, std::vector<bool>>>::const_iterator it = readoutDataMap_.begin(); it != readoutDataMap_.end(); ++it)
            {
                uint16_t cKey = it->first;
                uint8_t  hybridId, readoutId;
                decodeId(cKey, hybridId, readoutId);
                os << "== hybridId: <" << +hybridId << ">, readoutId: <" << +readoutId << ">" << std::endl;
                const ReadoutStatus& rs = it->second.first;
                rs.print(true, os);

                const std::vector<bool>& cdata = it->second.second;
                os << "Channels hit: <";
                for(size_t i = 0; i < cdata.size(); ++i)
                {
                    if(cdata.at(i)) os << i << " ";
                }
                os << ">" << std::endl;
            }
        }

      private:
        std::map<uint16_t, std::pair<ReadoutStatus, std::vector<bool>>> readoutDataMap_;
        std::vector<std::pair<uint8_t, std::vector<uint8_t>>>           feReadoutMappingList_;
        size_t                                                          nwords_;
    };
    class StubData
    {
      public:
        size_t set(const std::vector<uint64_t>& wordList, size_t ishift, const std::bitset<72>& feStatus)
        {
            // First try to find the position of the FE header and the Stub data length
            size_t            wbit = sizeof(uint64_t) * CHAR_BIT;
            std::vector<bool> data;
            size_t            nwords = 0;
            while(1)
            {
                uint64_t w = wordList.at(ishift + nwords);
                // std::cout << "stub word: " << std::bitset<64>(w) << std::endl;
                // if (nwords > 0 && !(w >> 32)) break;  // using the first condition data information (Condition Data
                // size)
                if(nwords > 0 && (w > 0 && w < 10) && !(w >> 32)) break; // using the first condition data information (Condition Data size)
                for(size_t j = 0; j < wbit; ++j) data.push_back((w >> (wbit - 1 - j)) & 0x1);
                ++nwords;
            }
            // Now store the results properly
            int                  ipos = 0;
            std::vector<uint8_t> feList;
            size_t               nFE = attachedFE(feStatus, feList);
            for(size_t i = 0; i < nFE; ++i)
            {
                size_t                nstub = getWord(data, ipos, NSTUB_WORD_WIDTH);
                std::vector<StubInfo> stubList;
                for(size_t j = 0; j < nstub; ++j)
                {
                    size_t   jpos      = NSTUB_WORD_WIDTH + 1 + j * STUB_WORD_WIDTH;
                    size_t   stub_word = getWord(data, jpos, STUB_WORD_WIDTH);
                    StubInfo info(stub_word);
                    stubList.push_back(info);
                }
                dMap_.insert({feList[i], stubList});
                ipos += NSTUB_WORD_WIDTH + 1 + nstub * STUB_WORD_WIDTH;
            }
            nwords_ = nwords;
            return nwords;
        }
        void print(std::ostream& os = std::cout) const
        {
            os << "[StubData]" << std::endl;
            for(const auto& v: dMap_)
            {
                os << "hybridId: <" << +v.first << ">" << std::endl;
                const std::vector<StubInfo>& stubList = v.second;
                for(size_t i = 0; i < stubList.size(); ++i) stubList.at(i).print((i == 0 ? true : false));
            }
        }
        const std::map<uint8_t, std::vector<StubInfo>>& getMap() const { return dMap_; }
        const std::vector<StubInfo>&                    stubs(uint8_t hybridId) const
        {
            // throws an out_of_range exception if the key hybridId is not found in the container (map)
            return dMap_.at(hybridId);
        }
        const StubInfo& stub(uint8_t hybridId, size_t stub_index) const
        {
            // throws an out_of_range exception if either the key hybridId is not found in the container (map)
            // or stub_index is out-of-range in the vector
            return dMap_.at(hybridId).at(stub_index);
        }
        size_t nwords() const { return nwords_; }

      private:
        std::map<uint8_t, std::vector<StubInfo>> dMap_;
        size_t                                   nwords_;
    };
    class ConditionData
    {
      public:
        void set(uint64_t word)
        {
            uint8_t hybridId = (word >> 56) & MASK_BITS_8;
            uint8_t xId      = (word >> 52) & MASK_BITS_4; // roID or sensorId
            uint8_t i2cPage  = (word >> 48) & MASK_BITS_4;
            uint8_t i2cReg   = (word >> 40) & MASK_BITS_8;
            uint8_t uid      = (word >> 32) & MASK_BITS_8;
            size_t  value    = word & MASK_BITS_32;

            dList_.push_back(std::make_tuple(hybridId, xId, i2cPage, i2cReg, uid, value));
        }
        void print(std::ostream& os = std::cout) const
        {
            os << "[ConditionData]" << std::endl;
            os << " for UID=1 x=Readout, UID=5 x=Sensor" << std::endl;
            os << " index  hybridId   xID i2cPage  i2cReg   UID       value" << std::endl;
            for(size_t i = 0; i < dList_.size(); ++i)
            {
                const auto& t = dList_[i];
                os << std::setw(6) << i << std::setw(6) << +std::get<0>(t) << std::setw(6) << +std::get<1>(t) << std::setw(8) << std::hex << +std::get<2>(t) << std::setw(8) << std::hex
                   << +std::get<3>(t) << std::setw(6) << std::hex << +std::get<4>(t) << std::setw(12) << std::hex << +std::get<5>(t) << std::dec << std::endl;
            }
        }
        const std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, size_t>>& getList() const { return dList_; }
        size_t                                                                              size() const { return dList_.size(); }
        const std::tuple<uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, size_t>&              data(size_t index) const
        {
            // throws an out_of_range exception if index is outside the bounds of valid elements in the vector
            return dList_.at(index);
        }

      private:
        // <hybridId, readout/sensor-Id, i2cPage, i2cRegister, UID, value>
        std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, size_t>> dList_;
    };

  public:
    /*!
     * constructor
     */
    // DQMEvent();
    // Constructors expect either an SLinkEvent object pointer or data vector in SLinkEvent Format
    // In the second case an SLinkEvent object is created from the data vector and the pointer stored
    DQMEvent(SLinkEvent* ptr);
    DQMEvent(std::vector<uint64_t>& dVec);

    void setEvent(std::vector<uint64_t>& dVec);
    void setEvent(SLinkEvent* ptr);
    /*!
     * destructor
     */
    virtual ~DQMEvent() {}
    void parseEvent(bool printData = false);

    // for file IO and debugging
    friend std::ostream& operator<<(std::ostream& os, const DQMEvent& ev)
    {
        ev.print(os);
        return os;
    }
    void printRaw(std::ostream& os = std::cout) const;

    void print(std::ostream& os = std::cout) const
    {
        this->printRaw(os);

        daqHeader().print(os);
        trkHeader().print(os);
        trkPayload().print(os);
        stubData().print(os);
        condData().print(os);
        daqTrailer().print(os);
        os << std::flush;
    }
    DAQHeader&      daqHeader() { return daqHeader_; }
    DAQTrailer&     daqTrailer() { return daqTrailer_; }
    TrackerHeader&  trkHeader() { return trkHeader_; }
    TrackerPayload& trkPayload() { return trkPayload_; }
    StubData&       stubData() { return stubData_; }
    ConditionData&  condData() { return condData_; }

    const DAQHeader&      daqHeader() const { return daqHeader_; }
    const DAQTrailer&     daqTrailer() const { return daqTrailer_; }
    const TrackerHeader&  trkHeader() const { return trkHeader_; }
    const TrackerPayload& trkPayload() const { return trkPayload_; }
    const StubData&       stubData() const { return stubData_; }
    const ConditionData&  condData() const { return condData_; }

  private:
    std::unique_ptr<SLinkEvent> ptr_;

    DAQHeader      daqHeader_;
    DAQTrailer     daqTrailer_;
    TrackerHeader  trkHeader_;
    TrackerPayload trkPayload_;
    StubData       stubData_;
    ConditionData  condData_;

  private:
    // we need to explicitly disable value-copying, as it's not safe!
    DQMEvent(DQMEvent& other) : ptr_(std::move(other.ptr_)) {}

    DQMEvent& operator=(DQMEvent& other)
    {
        ptr_ = std::move(other.ptr_);
        return *this;
    }
};
#endif
