/*!
  \file                  FileHeader.h
  \brief                 Binary file header
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinard@cern.ch
*/

#ifndef FILEHEADER_H
#define FILEHEADER_H

#include "ConsoleColor.h"
#include "HWDescription/Definition.h"
#include "easylogging++.h"

#include <iostream>
#include <vector>

// #############
// # CONSTANTS #
// #############
#define HEADER_SEPARATOR 0xAAAAAAAA

class FileHeader
{
  public:
    static const uint32_t fHeaderSize = 12;

    std::string fType;
    uint32_t    fVersionMajor;
    uint32_t    fVersionMinor;
    uint32_t    fBeId;
    uint32_t    fNchip;
    uint32_t    fEventSize;
    EventType   fEventType;
    bool        fValid;

  public:
    FileHeader() : fType(""), fVersionMajor(0), fVersionMinor(0), fBeId(0), fNchip(0), fEventSize(0), fEventType(EventType::VR), fValid(false) {}

    FileHeader(const std::string pType,
               const uint32_t&   pFWMajor,
               const uint32_t&   pFWMinor,
               const uint32_t&   pBeId,
               const uint32_t&   pNchip,
               const uint32_t&   pEventSize,
               EventType         pEventType = EventType::VR)
        : fType(pType), fVersionMajor(pFWMajor), fVersionMinor(pFWMinor), fBeId(pBeId), fNchip(pNchip), fEventSize(pEventSize), fEventType(pEventType), fValid(true)
    {
    }

    BoardType getBoardType()
    {
        if(fType == "D19C")
            return BoardType::D19C;
        else if(fType == "RD53")
            return BoardType::RD53;
        return BoardType::D19C;
    }

    std::vector<uint32_t> encodeHeader()
    {
        std::vector<uint32_t> cVec;

        // Surround every block with 10101...
        cVec.push_back(HEADER_SEPARATOR);
        char cType[8] = {0};

        if(fType.size() < 9)
            strcpy(cType, fType.c_str());
        else
            LOG(INFO) << BOLDRED << "Error, type string can only be up to 8 characters long" << RESET;

        cVec.push_back(cType[0] << 24 | cType[1] << 16 | cType[2] << 8 | cType[3]);
        cVec.push_back(cType[4] << 24 | cType[5] << 16 | cType[6] << 8 | cType[7]);

        cVec.push_back(HEADER_SEPARATOR);
        cVec.push_back(fVersionMajor);
        cVec.push_back(fVersionMinor);

        cVec.push_back(HEADER_SEPARATOR);
        cVec.push_back((uint32_t(fEventType) & 0x3) << 30 | (fBeId & 0x000003FF));
        cVec.push_back(fNchip);

        cVec.push_back(HEADER_SEPARATOR);
        cVec.push_back(fEventSize);

        cVec.push_back(HEADER_SEPARATOR);

        std::string cEventTypeString;

        if(fEventType == EventType::VR)
            cEventTypeString = "EventType::VR";
        else
            cEventTypeString = "EventType::ZS";

        return cVec;
    }

    void decodeHeader(const std::vector<uint32_t>& pVec)
    {
        if(pVec.at(0) == HEADER_SEPARATOR && pVec.at(3) == HEADER_SEPARATOR && pVec.at(6) == HEADER_SEPARATOR && pVec.at(9) == HEADER_SEPARATOR && pVec.at(11) == HEADER_SEPARATOR)
        {
            char cType[8] = {0};
            cType[0]      = (pVec.at(1) & 0xFF000000) >> 24;
            cType[1]      = (pVec.at(1) & 0x00FF0000) >> 16;
            cType[2]      = (pVec.at(1) & 0x0000FF00) >> 8;
            cType[3]      = (pVec.at(1) & 0x000000FF);

            cType[4] = (pVec.at(2) & 0xFF000000) >> 24;
            cType[5] = (pVec.at(2) & 0x00FF0000) >> 16;
            cType[6] = (pVec.at(2) & 0x0000FF00) >> 8;
            cType[7] = (pVec.at(2) & 0x000000FF);

            std::string cTypeString(cType);
            fType = cTypeString;

            fVersionMajor = pVec.at(4);
            fVersionMinor = pVec.at(5);

            uint32_t cEventTypeId = (pVec.at(7) & 0xC0000000) >> 30;
            fBeId                 = pVec.at(7) & 0x000003FF;
            fNchip                = pVec.at(8);

            fEventSize = pVec.at(10);
            fValid     = true;

            if(cEventTypeId == 0)
                fEventType = EventType::VR;
            else if(cEventTypeId == 1)
                fEventType = EventType::ZS;
            else
                fEventType = EventType::VR;

            std::string cEventTypeString;

            if(fEventType == EventType::VR)
                cEventTypeString = "EventType::VR";
            else
                cEventTypeString = "EventType::ZS";
        }
        else
        {
            LOG(ERROR) << BOLDRED << "Invalid header from binary file" << RESET;
            fValid = false;
        }
    }
};

#endif
