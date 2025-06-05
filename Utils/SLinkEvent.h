#ifndef __SLINKEVENT_H__
#define __SLINKEVENT_H__

#include <bitset>
#include <deque>
#include <iomanip>
#include <iostream>
#include <set>
#include <vector>

#include "CRCCalculator.h"
#include "ConditionDataSet.h"
#include "ConsoleColor.h"
#include "GenericPayload.h"
#include "HWDescription/Definition.h"
#include "easylogging++.h"

#define BOE_1 0x5
#define EVENT_TYPE 0x01 // Physics Trigger
#define SOURCE_ID 0x33  // FedID = 51
#define FOV 0x00
#define EOE_1 0xA0
#define TTS_VALUE 0x7

class SLinkEvent
{
  public:
    SLinkEvent();
    SLinkEvent(EventType pEventType, SLinkDebugMode pMode, FrontEndType pFrontEndType, uint32_t& pLV1Id, uint16_t& pBXId, int pSourceId = SOURCE_ID);
    SLinkEvent(std::vector<uint64_t>& pDataVec); // playback Constructor

    ~SLinkEvent() { fData.clear(); }

    void generateDAQHeader(uint32_t& pLV1Id, uint16_t& pBXId, int pSourceId);
    void generateTkHeader(uint32_t& pBeStatus, uint16_t& pNChips, std::set<uint8_t>& pEnabledHybrids, bool pCondData = false, bool pFake = false);
    // the following 4 are dumb methods in that they just insert a vector of 64 bit words
    void generateStatus(std::vector<uint64_t> pStatusVec);
    void generatePayload(std::vector<uint64_t> pPayloadVec);
    void generateStubs(std::vector<uint64_t> pStubVec);
    // kind of important
    void     generateConditionData(ConditionDataSet* pSet);
    void     generateDAQTrailer();
    uint32_t getLV1Id();
    uint32_t getSourceId();
    uint32_t getSize64();

    // template<typename T>
    // std::vector<T> getData();
    template <typename T>
    std::vector<T> getData()
    {
        return split_vec64<T>(fData);
    }

    void print(std::ostream& out = std::cout) const;

    // for file IO and debugging
    friend std::ostream& operator<<(std::ostream& out, const SLinkEvent& ev)
    {
        ev.print(out);
        return out;
    }

    void clear()
    {
        fData.clear();
        fSize   = 0;
        fCRCVal = 0xFFFF;
    }

  private:
    // Enums defined in HWDescription/Definition.h
    FrontEndType         fFrontEndType;
    EventType            fEventType;
    SLinkDebugMode       fDebugMode;
    static CRCCalculator fCalculator;
    // using a std::deque as it is probably easier to push front and insert randomly
    std::vector<uint64_t> fData;
    // size of the complete event in 64 bit words
    size_t fSize;
    // to hold the cyclic redundancy check checksum
    uint16_t fCRCVal;
    // flags to signal presence of condition data and fake or real events
    bool fCondData, fFake;

    void calculateCRC();
};

#endif
