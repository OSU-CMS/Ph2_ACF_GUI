#include "SLinkEvent.h"

CRCCalculator SLinkEvent::fCalculator;

SLinkEvent::SLinkEvent() : fSize(0), fCRCVal(0), fCondData(0), fFake(0) { fData.clear(); }

SLinkEvent::SLinkEvent(EventType pEventType, SLinkDebugMode pMode, FrontEndType pFrontEndType, uint32_t& pLV1Id, uint16_t& pBXId, int pSourceId)
    : fFrontEndType(pFrontEndType), fEventType(pEventType), fDebugMode(pMode), fSize(0), fCRCVal(0), fCondData(0), fFake(0)
{
    fData.clear();
    this->generateDAQHeader(pLV1Id, pBXId, pSourceId);
}
SLinkEvent::SLinkEvent(std::vector<uint64_t>& pDataVec) : fSize(pDataVec.size())
{
    fData.clear();
    fData = pDataVec;
    // fEventType = ;
    // fDebugMode = ;
    // fCRCVal = ;
}

// template<typename T>
// std::vector<T> SLinkEvent::getData()
//{
// return split_vec64<T> (fData);
//}

void SLinkEvent::print(std::ostream& out) const
{
    std::string cDebugMode, cReadoutMode;

    if(fDebugMode == SLinkDebugMode::ERROR) cDebugMode = "SLinkDebugMode::ERROR";

    if(fDebugMode == SLinkDebugMode::FULL) cDebugMode = "SLinkDebugMode::FULL";

    if(fDebugMode == SLinkDebugMode::SUMMARY) cDebugMode = "SLinkDebugMode::SUMMARY";

    if(fEventType == EventType::VR) cReadoutMode = "EventType::VR";

    if(fEventType == EventType::ZS) cReadoutMode = "EventType::ZS";

    out << BOLDYELLOW << "SLINK EVENT: Type " << RED << cReadoutMode << BOLDYELLOW << " - Debug Mode " << RED << cDebugMode << RESET << std::endl;
    int cCounter = 0;

    for(auto word: fData)
    {
        // char cCountString[3];
        // sprintf (cCountString, "%03d", cCounter++);
        out << BOLDYELLOW << "#" << std::setw(3) << cCounter << RESET << "  " << (static_cast<std::bitset<64>>(word)) << "  " << BOLDYELLOW << std::setw(16) << std::hex << word << std::dec << RESET
            << std::endl;
        cCounter++;
    }
}

void SLinkEvent::generateDAQHeader(uint32_t& pLV1Id, uint16_t& pBXId, int pSourceId)
{
    uint64_t cWord = 0;
    // BOE_1 | EVENT_TYPE | L1 ID | BX ID | SOURCE ID | FOV
    cWord |=
        ((uint64_t)BOE_1 & 0xF) << 60 | ((uint64_t)EVENT_TYPE & 0xF) << 56 | ((uint64_t)pLV1Id & 0x00FFFFFF) << 32 | ((uint64_t)pBXId & 0x0FFF) << 20 | (pSourceId & 0x0FFF) << 8 | (FOV & 0xF) << 4;
    LOG(DEBUG) << BOLDYELLOW << "DAQ header " << std::bitset<64>(cWord) << RESET;
    fData.insert(fData.begin(), cWord);
    fSize += 1;
}

void SLinkEvent::generateTkHeader(uint32_t& pBeStatus, uint16_t& pNChips, std::set<uint8_t>& pEnabledHybrids, bool pCondData, bool pFake)
{
    uint64_t cWord1 = 0;
    uint64_t cWord2 = 0;
    // version | format | event type | BeStatus | NChips | Hybrid Status (8)
    cWord1 |= ((uint64_t)0x2 & 0xF) << 60 | ((uint64_t)fDebugMode & 0x03) << 58 | ((uint64_t)fEventType & 0x03) << 56 | (uint64_t)pCondData << 55 | (uint64_t)!pFake << 54 |
              ((uint64_t)pBeStatus & 0x3FFFFFFF) << 24 | pNChips << 8; // pFeStatus is the end of the

    for(auto& cHybrid: pEnabledHybrids)
    {
        if(cHybrid < 64)
            cWord2 |= (uint64_t)1 << cHybrid;
        else
            cWord1 |= (uint64_t)1 << (cHybrid - 64);
    }

    fData.insert(fData.begin() + 1, cWord1);
    fData.insert(fData.begin() + 2, cWord2);

    LOG(DEBUG) << BOLDYELLOW << "Tracker header " << std::bitset<64>(cWord1) << RESET;
    LOG(DEBUG) << BOLDYELLOW << "Tracker header " << std::bitset<64>(cWord2) << RESET;

    fSize += 2;
}

void SLinkEvent::generateStatus(std::vector<uint64_t> pStatusVec)
{
    fData.insert(fData.begin() + 3, pStatusVec.begin(), pStatusVec.end());
    fSize += pStatusVec.size();
}

void SLinkEvent::generatePayload(std::vector<uint64_t> pPayloadVec)
{
    fData.insert(fData.end(), pPayloadVec.begin(), pPayloadVec.end());
    fSize += pPayloadVec.size();
}

void SLinkEvent::generateStubs(std::vector<uint64_t> pStubVec)
{
    fData.insert(fData.end(), pStubVec.begin(), pStubVec.end());
    fSize += pStubVec.size();
}

void SLinkEvent::generateConditionData(ConditionDataSet* pSet)
{
    // if there are condition data defined
    if(pSet != nullptr && pSet->fCondDataVector.size() != 0)
    {
        LOG(DEBUG) << BOLDYELLOW << +pSet->fCondDataVector.size() << " words in condition data." << RESET;
        std::vector<uint64_t> cVec;
        cVec.push_back(pSet->fCondDataVector.size());

        for(auto cCondItem: pSet->fCondDataVector)
        {
            uint64_t cWord = (((uint64_t)cCondItem.fHybridId & 0xFF) << 56 | ((uint64_t)cCondItem.fCbcId & 0xF) << 52 | ((uint64_t)cCondItem.fPage & 0xF) << 48 |
                              ((uint64_t)cCondItem.fRegister & 0xFF) << 40 | ((uint64_t)cCondItem.fUID & 0xFF) << 32 | cCondItem.fValue);
            LOG(DEBUG) << BOLDYELLOW << std::bitset<64>(cWord) << RESET;
            cVec.push_back(cWord);
        }

        fData.insert(fData.end(), cVec.begin(), cVec.end());
        fSize += cVec.size();
    }
}

void SLinkEvent::generateDAQTrailer()
{
    // fSize += 1;
    uint64_t cWord = 0;
    // EOE_1 | EvtLength | CRC | Event Stat | TTS
    // cWord |=  ( (uint64_t) EOE_1 << 56) |  ( ((uint64_t) fSize & 0xFFFFFF) << 32) | ( (TTS_VALUE & 0xF) << 4) ;
    cWord |= (((uint64_t)EOE_1 & 0xFF) << 56 | ((uint64_t)fSize & 0x00FFFFFF) << 32 | (TTS_VALUE & 0xF) << 4);
    LOG(DEBUG) << BOLDYELLOW << "DAQ trailer " << std::bitset<64>(cWord) << RESET;
    fData.push_back(cWord);
    this->calculateCRC();
    fData.back() |= ((uint64_t)fCRCVal & 0xFFFF) << 16;
}

uint32_t SLinkEvent::getLV1Id()
{
    if(fData.size() != 0)
        return (fData.at(0) >> 32) & 0x00FFFFFF;
    else
    {
        LOG(ERROR) << BOLDRED << "No content in Event!" << RESET;
        return 0;
    }
}

uint32_t SLinkEvent::getSourceId()
{
    if(fData.size() != 0)
        return (fData.at(0) >> 8) & 0x00000FFF;
    else
    {
        LOG(ERROR) << BOLDRED << "No content in Event!" << RESET;
        return 0;
    }
}

uint32_t SLinkEvent::getSize64() { return fData.size(); }

void SLinkEvent::calculateCRC()
{
    // original code before CRC Calculator
    // i need to split the data vector in octets and then it should work
    // uint16_t cCRC = 0xFFFF;
    // uint16_t cPolynomial = 0xA001;// Polynomial 2^16 + 2^15 + 2^2 + 2^0 = 0x8005.
    // uint8_t cBit, cParity;

    // for (uint32_t cWord = 0; cWord < fData.size(); cWord++) // iterate through data in words
    //{
    // uint8_t cOctetIndex = 0;
    // uint8_t cByte = 0;
    // uint64_t cWord64 = fData[cWord];

    // do
    //{
    ////ATTENTION: cByte holds the LSBs first - if this needs to be reversed, keep the code
    // cByte = cWord64 & 0xFF;
    // cCRC ^= cByte;

    // for (cBit = 0; cBit < 8; cBit++)
    //{
    // cParity = cCRC % 2;
    // cCRC >>= 1;

    // if (cParity)
    // cCRC ^= cPolynomial;
    //}

    // cOctetIndex++;
    //}
    // while (cWord64 >>= 8);
    //}

    // fCRCVal = cCRC;
    // CRCCalculator needs an array of unsigned char
    std::vector<uint8_t> cData = this->getData<uint8_t>();
    fCRCVal                    = fCalculator.compute(&cData[0], sizeof(&cData));
}
