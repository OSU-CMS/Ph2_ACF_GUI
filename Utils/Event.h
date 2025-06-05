/*
        \file                          Event.h
        \brief                         Event handling from DAQ
        \author                        Nicolas PIERRE
        \version                       1.0
        \date                                  10/07/14
        Support :                      mail to : nicolas.pierre@icloud.com
 */

#ifndef __EVENT_H__
#define __EVENT_H__

#include "ConsoleColor.h"
#include "HWDescription/BeBoard.h"
#include "HWDescription/Definition.h"
#include "SLinkEvent.h"
#include "Utils/DataContainer.h"
#include "Utils/easylogging++.h"
#include <bitset>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

class BoardDataContainer;
class ChannelGroupBase;

namespace Ph2_HwInterface
{
using EventDataMap = std::map<uint16_t, std::vector<uint32_t>>;

class Event
{
    /*
       id of FeEvent should be the order of FeEvents in data stream starting from 0
       id of CbcEvent also should be the order of CBCEvents in data stream starting from 0
     */
  public:
    EventDataMap fEventDataMap;

  protected:
    uint32_t fEventCount;        /*!< Event Counter */
    uint32_t fTDC;               /*!< TDC value*/
    uint32_t fExternalTriggerID; /*!< TLU Trigger ID*/
    // for CBC2 use
    uint32_t fBunch;         /*!< Bunch value */
    uint32_t fOrbit;         /*!< Orbit value */
    uint32_t fLumi;          /*!< LuminositySection value */
    uint32_t fEventCountCBC; /*!< Cbc Event Counter */
    uint32_t fEventSize;
    uint16_t fL1Number;

    uint16_t encodeId(const uint8_t& pHybridId, const uint8_t& pCbcId) const { return (pHybridId << 8 | pCbcId); }

    void decodeId(const uint16_t& pKey, uint8_t& pHybridId, uint8_t& pCbcId) const
    {
        pHybridId = (pKey >> 8) & 0x00FF;
        pCbcId    = pKey & 0xFF;
    }

  public:
    /*!
     * \brief Constructor of the Event Class
     * \param pBoard : Board to work with
     * \param pNbCbc
     * \param pEventBuf : the pointer to the raw Event buffer of this Event
     */
    Event() {}
    /*!
     * \brief Copy Constructor of the Event Class
     */
    Event(const Event& pEvent) = default;
    /*!
     * \brief Copy Assignment of the Event Class
     */
    Event& operator=(const Event& pEvent) = default;
    /*!
     * \brief Destructor of the Event Class
     */
    virtual ~Event() {}
    /*!
     * \brief Clear the Event Map
     */
    void Clear() { fEventDataMap.clear(); }
    /*! \brief Get raw data */
    // const std::vector<uint32_t>& GetEventData() const
    //{
    // return fEventData;
    //}
    /*! \brief Get the event size in bytes */
    uint32_t GetSize() const { return fEventSize; }
    /*!
     * \brief Get the bunch value
     * \return Bunch value
     */
    uint32_t GetBunch() const { return fBunch; }
    /*!
     * \brief Get the orbit value
     * \return Orbit value
     */
    uint32_t GetOrbit() const { return fOrbit; }
    /*!
     * \brief Get the luminence value
     * \return Luminence value
     */
    uint32_t GetLumi() const { return fLumi; }
    /*!
     * \brief Get the Event counter
     * \return Event counter
     */
    uint32_t GetEventCount() const { return fEventCount; }
    /*!
     * \brief Get TDC value ??
     * \return TDC value
     */
    uint32_t GetTDC() const { return fTDC; }
    /*!
     * \brief Get External trigger  id ??
     * \return external trigger value
     */
    uint32_t GetExternalTriggerId() const { return fExternalTriggerID; }
    /*!
     * \brief Get an event contained in a Cbc
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return Event buffer
     */
    void GetCbcEvent(const uint8_t& pHybridId, const uint8_t& pCbcId, std::vector<uint32_t>& cbcData) const;
    /*!
     * \brief Get an event contained in a Cbc
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return Event buffer
     */
    void GetCbcEvent(const uint8_t& pHybridId, const uint8_t& pCbcId, std::vector<uint8_t>& cbcData) const;
    /*!
     * \brief Function to get the bit at the global data string position
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \param pPosition : Position in the data buffer
     * \return Bit
     */
    bool Bit(uint8_t pHybridId, uint8_t pCbcId, uint32_t pPosition) const;
    /*!
     * \brief Function to get bit string from the data offset and width
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \param pOffset : position Offset
     * \param pWidth : string width
     * \return Bit string
     */
    std::string BitString(uint8_t pHybridId, uint8_t pCbcId, uint32_t pOffset, uint32_t pWidth) const;
    /*!
     * \brief Function to get bit vector from the data offset and width
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \param pOffset : position Offset
     * \param pWidth : string width
     * \return Boolean/Bit vector
     */
    std::vector<bool> BitVector(uint8_t pHybridId, uint8_t pCbcId, uint32_t pOffset, uint32_t pWidth) const;
    /*!
     * \brief Function to get char at the global data string at position 8*i
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \param pBytePosition : Position of the byte
     * \return Char in given position
     */
    unsigned char Char(uint8_t pHybridId, uint8_t pCbcId, uint32_t pBytePosition);

    const EventDataMap& GetEventDataMap() const { return fEventDataMap; }

    bool operator==(const Event& pEvent) const;

    // ###################
    // # VIRTUAL METHODS #
    // ###################
    /*!
     * \brief Convert Data to Hex string
     * \return Data string in hex
     */
    virtual std::string HexString() const { return ""; }

    uint16_t GetL1Number() const { return fL1Number; }

    /*!
     * \brief Get the Cbc Event counter
     * \return Cbc Event counter
     */
    virtual uint32_t GetEventCountCBC() const { return 0; }
    /*!
     * \brief Function to get bit vector of CBC data
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return Data Bit vector
     */
    virtual std::vector<bool> DataBitVector(uint8_t pHybridId, uint8_t pCbcId) { return {}; }
    /*!
     * \brief Function to get all Error bits
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return Error bit
     */
    virtual uint32_t Error(uint8_t pHybridId, uint8_t pCbcId) { return 0; }
    /*!
     * \brief Function to get pipeline address
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return Pipeline address
     */
    virtual uint32_t PipelineAddress(uint8_t pHybridId, uint8_t pCbcId) { return 0; }
    /*!
     * \brief Function to get pipeline address
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return Pipeline address
     */
    virtual uint32_t L1Id(uint8_t pHybridId, uint8_t pCbcId) { return 0; }

    /*!
     * \brief Function to get pipeline address
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return Pipeline address
     */
    virtual uint32_t BxId(uint8_t pHybridId) { return 0; }
    /*!
     * \brief Function to get a CBC pixel bit data
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \param i : pixel bit data number i
     * \return Data Bit
     */
    virtual bool DataBit(uint8_t pHybridId, uint8_t pCbcId, uint8_t row, uint8_t col) { return true; }

    /*!
     * \brief Function to get Stub bit
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return stub bit?
     */
    virtual bool StubBit(uint8_t pHybridId, uint8_t pCbcId) { return true; }

    /*!
     * \brief Function to count the Hits in this event
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return number of hits
     */
    virtual uint32_t GetNHits(uint8_t pHybridId, uint8_t pCbcId) { return 0; }
    /*!
     * \brief Function to get a sparsified hit vector
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return vector with hit channels (row, col)
     */
    virtual std::vector<std::pair<uint16_t, uint16_t>> GetHits(uint8_t pHybridId, uint8_t pCbcId)
    {
        std::cerr << __PRETTY_FUNCTION__ << " not implemented! Aborting..." << std::endl;
        abort();
        return {};
    }

    virtual void fillDataContainer(BoardDataContainer* boardContainer, const std::shared_ptr<ChannelGroupBase> testChannelGroup);
    virtual void fillChipDataContainer(ChipDataContainer* boardContainer, const std::shared_ptr<ChannelGroupBase> testChannelGroup, uint8_t hybridId) = 0;

    // split stream of data
    template <std::size_t N>
    void splitStream(const std::vector<uint32_t> pData, std::vector<std::bitset<N>>& pBitSet, size_t pOffset, size_t pSize, size_t pBitOffset = 0)
    {
        uint32_t cBitCounter  = 0;
        uint32_t cId          = 0;
        auto     cIterator    = pData.begin() + pOffset;
        size_t   cWordCounter = 0;
        do {
            auto cWord = std::bitset<32>(*cIterator);
            // LOG(INFO) << BOLDBLUE << "Word " << +cWordCounter << " : " << cWord << RESET;
            for(size_t cIndex = 0; cIndex < 32; cIndex++)
            {
                if(cId >= pSize) continue;
                if(cIndex < pBitOffset and (cWordCounter == 0)) continue;

                pBitSet[cId][N - 1 - cBitCounter] = cWord[31 - cIndex];
                cId += (cBitCounter == (N - 1));
                cBitCounter = (cBitCounter + 1) % N;
            }
            cIterator++;
            cWordCounter++;
        } while(cIterator < pData.end() && cId < pSize);
    }
};

} // namespace Ph2_HwInterface
#endif
