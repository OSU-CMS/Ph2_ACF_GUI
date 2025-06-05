#include "TApplication.h"
#include "TROOT.h"
#include "Utils/Timer.h"
#include "Utils/argvparser.h"
#include "tools/StubQuickCheck.h"
#include "tools/Tool.h"
#include <boost/filesystem.hpp>
#include <cstring>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;
using namespace CommandLineProcessing;

using namespace std;
INITIALIZE_EASYLOGGINGPP

struct SLinkEventHeader
{
    uint64_t              fDAQHeader;
    std::vector<uint64_t> fTrackerHeader;
    uint16_t              fNReadoutChips;
    uint16_t              fNenabledFEs;
    uint8_t               fDebugLevel;
    uint8_t               fEventType;
    bool                  fSparisified;
    uint16_t              fNStatusBits;
    uint8_t               fNStatusWords;

    void decode()
    {
        fNReadoutChips = (fTrackerHeader[0] & 0xFFFF00) >> 8;
        std::bitset<8>  cEnabledFEs_firstPart(fTrackerHeader[0] & 0xFF);
        std::bitset<64> cEnabledFEs_secondPart(fTrackerHeader[1]);
        fNenabledFEs = cEnabledFEs_firstPart.count() + cEnabledFEs_secondPart.count();

        fDebugLevel = ((uint64_t)fTrackerHeader[0] & ((uint64_t)0x3 << 58)) >> 58;
        fEventType  = ((uint64_t)fTrackerHeader[0] & ((uint64_t)0xF << 54)) >> 54;
        // TO-DO : use event type and sparsification to calculate this for all possible event types
        fSparisified  = (((fEventType & (0x1 << 4)) >> 4) == 0);
        fNStatusBits  = fNReadoutChips * 20;
        fNStatusWords = std::ceil(fNStatusBits / 64.);
    }

    void print()
    {
        LOG(INFO) << BOLDYELLOW << "DAQ header : " << std::bitset<64>(fDAQHeader) << RESET;
        LOG(INFO) << BOLDYELLOW << "Tracker header : " << std::bitset<64>(fTrackerHeader[0]) << RESET;
        LOG(INFO) << BOLDYELLOW << "Tracker header : " << std::bitset<64>(fTrackerHeader[1]) << RESET;
        LOG(INFO) << BOLDYELLOW << "S-link events contain information from " << fNReadoutChips << " readout chips. Number of enabled FEs : " << +fNenabledFEs << RESET;
        LOG(INFO) << BOLDYELLOW << "S-link debug level is " << std::bitset<2>(fDebugLevel) << " -- event type is " << std::bitset<4>(fEventType) << " event has " << +fNStatusWords << " status words."
                  << RESET;
    }
};

struct SLinkEventConditionData
{
    size_t                    fSize;
    std::vector<uint64_t>     fConditionData;
    std::vector<CondDataItem> fDataItems;

    void print()
    {
        LOG(INFO) << BOLDYELLOW << "S-link event condition contains " << +fSize << " 64 bit words of condition data." << RESET;
        size_t cWordCount = 0;
        for(auto cConditionData: fConditionData)
        {
            LOG(INFO) << BOLDYELLOW << "S-link event condition data word #" << +cWordCount << " : " << std::bitset<64>(cConditionData) << RESET;
            // auto cDataItem = fDataItems[cWordCount];
            // LOG (INFO) << BOLDYELLOW << "ConditionData [ " << +cWordCount << " ] : " << +cDataItem.fUID << " --- " <<
            // +cDataItem.fValue << " register " << +cDataItem.fRegister << RESET;
            cWordCount++;
        }
    }
    void set(std::vector<uint64_t>::iterator& pIterator)
    {
        fDataItems.clear();
        for(size_t cIndex = 0; cIndex < fSize; cIndex++)
        {
            fConditionData.push_back(*pIterator);
            pIterator++;
        }
    }
    void decode()
    {
        size_t cWordCount = 0;
        for(auto& cWord: fConditionData)
        {
            CondDataItem cDataItem;
            cDataItem.fHybridId = ((uint64_t)cWord & ((uint64_t)0xFF << 56)) >> 56;
            cDataItem.fCbcId    = ((uint64_t)cWord & ((uint64_t)0xF << 52)) >> 52;
            cDataItem.fPage     = ((uint64_t)cWord & ((uint64_t)0xF << 48)) >> 48;
            cDataItem.fRegister = ((uint64_t)cWord & ((uint64_t)0xFF << 40)) >> 40;
            cDataItem.fUID      = ((uint64_t)cWord & ((uint64_t)0xFF << 32)) >> 32;
            cDataItem.fValue    = ((uint64_t)cWord & ((uint64_t)0xFFFFFFFF));
            fDataItems.push_back(cDataItem);
            cWordCount++;
        }
    }
};

void decodeHits(std::vector<uint64_t>::iterator& pIterator, SLinkEventHeader pEventHeader, uint8_t pNpaddingBits = 2)
{
    size_t                   cReadoutChips = 0;
    size_t                   cBitCount     = 0;
    const size_t             pHeaderSize   = 16;
    std::bitset<pHeaderSize> cPayloadHeader(0);
    std::bitset<64>          cStream(*pIterator);
    // std::bitset<16> cEnabledReadoutChips( ( (uint64_t) (*pIterator) & ( (uint64_t) 0xFFFF << (64-(16+cBitCount))) )
    // >> (64-(16+cBitCount)) ); cBitCount += 16 ;

    while(cReadoutChips < pEventHeader.fNReadoutChips) //&& cEnabledChips > 0 )
    {
        // get header for this FE payload
        cPayloadHeader = std::bitset<pHeaderSize>(0);
        for(size_t cBitIndex = 0; cBitIndex < cPayloadHeader.size(); cBitIndex++)
        {
            if(cBitCount == 64)
            {
                cBitCount = 0;
                pIterator += 1;
                cStream = std::bitset<64>(*pIterator);
            }
            cPayloadHeader[cPayloadHeader.size() - (1 + cBitIndex)] = cStream[64 - (1 + cBitCount)];
            cBitCount++;
        }
        size_t cEnabledChips = cPayloadHeader.count();
        cReadoutChips += cEnabledChips;
        LOG(DEBUG) << BOLDYELLOW << "First word in tracker payload is " << std::bitset<64>(*pIterator) << " i.e. " << +cEnabledChips << " redaout chips enabled [ count is " << cReadoutChips << " ]."
                   << RESET;
        for(size_t cChipIndex = 0; cChipIndex < cEnabledChips; cChipIndex++)
        {
            std::bitset<256> cBitStream(0);
            for(size_t cBitIndex = 0; cBitIndex < cBitStream.size(); cBitIndex++)
            {
                if(cBitCount == 64)
                {
                    cBitCount = 0;
                    pIterator += 1;
                    cStream = std::bitset<64>(*pIterator);
                }
                // auto cBit = ( (uint64_t) *pIterator & ( (uint64_t) 0x1 << (64-(cBitCount+1)) ) ) >>
                // (64-(cBitCount+1)) ;
                cBitStream[cBitStream.size() - (1 + cBitIndex)] = cStream[64 - (1 + cBitCount)];
                cBitCount++;
            }
            LOG(INFO) << BOLDYELLOW << "Readout chip " << +cChipIndex << " : " << cBitStream << RESET;
            for(size_t cBitIndex = 0; cBitIndex < cBitStream.size(); cBitIndex++)
            {
                if(cBitStream[cBitIndex] > 0) LOG(INFO) << BOLDYELLOW << "Readout chip " << +cChipIndex << " hit in channel " << +(cBitStream.size() - 1 - cBitIndex) << RESET;
            }
            // if( cBitStream[cBitStream.size()-(1+cBitIndex)] == 1 )
            // {
            //     if( cBitIndex < cBitStream.size() )
            //     {
            //         LOG (INFO) << BOLDYELLOW << "Readout chip " << +cChipIndex << " hit in channel " << +(cBitIndex)
            //         << RESET;
            //         //LOG (INFO) << BOLDYELLOW << "Readout chip " << +cChipIndex << " hit in channel " <<
            //         +(cBitIndex-pNpaddingBits) << RESET; cBitStream.set(cBitIndex);
            //     }
            // }
        }
        // if( cReadoutChips >= pEventHeader.fNReadoutChips )
        //    continue;
    }
    pIterator += 1;
}

void decodeStubs(std::vector<uint64_t>::iterator& pIterator, SLinkEventHeader pEventHeader)
{
    const size_t pStubSize   = 16;
    const size_t pHeaderSize = 6;
    size_t       cBitCount   = 0;
    // I'm not sure how well this will work for events with mulitple stubs that span two 64 bit words
    std::bitset<64> cStream(*pIterator);
    for(size_t cFE = 0; cFE < pEventHeader.fNenabledFEs; cFE++)
    {
        std::bitset<pHeaderSize> cStubHeader(0);
        for(size_t cBitIndex = 0; cBitIndex < cStubHeader.size(); cBitIndex++)
        {
            if(cBitCount == 64)
            {
                cBitCount = 0;
                pIterator += 1;
                cStream = std::bitset<64>(*pIterator);
            }
            cStubHeader[cStubHeader.size() - (1 + cBitIndex)] = cStream[64 - (1 + cBitCount)];
            cBitCount++;
        }
        auto cNstubs = cStubHeader.to_ulong() >> 1;
        LOG(DEBUG) << BOLDYELLOW << "Stub header for FE " << +cFE << " : " << cStubHeader << " :  " << +cNstubs << " stub on this link." << RESET;
        for(size_t cStub = 0; cStub < cNstubs; cStub++)
        {
            std::bitset<pStubSize> cBitStream(0);
            for(size_t cBitIndex = 0; cBitIndex < cBitStream.size(); cBitIndex++)
            {
                if(cBitCount == 64)
                {
                    cBitCount = 0;
                    pIterator += 1;
                    cStream = std::bitset<64>(*pIterator);
                }
                cBitStream[cBitStream.size() - (1 + cBitIndex)] = cStream[64 - (1 + cBitCount)];
                cBitCount++;
            }
            auto cChipId   = ((uint16_t)cBitStream.to_ulong() & (0xF << 12)) >> 12;
            auto cSeed     = ((uint16_t)cBitStream.to_ulong() & (0xFF << 4)) >> 4;
            auto cBendCode = ((uint16_t)cBitStream.to_ulong() & (0xF << 0)) >> 0;
            LOG(INFO) << BOLDYELLOW << "\t\t.. stub : " << cBitStream << " stub in chip " << +cChipId << " with seed " << +cSeed << " and bend code " << std::bitset<4>(cBendCode) << RESET;
        }
    }
    pIterator += 1;
}

int main(int argc, char* argv[])
{
    // configure the logger
    el::Configurations conf(std::string(std::getenv("PH2ACF_BASE_DIR")) + "/settings/logger.conf");
    el::Loggers::reconfigureAllLoggers(conf);

    ArgvParser cmd;

    // init
    cmd.setIntroductoryDescription("CMS Ph2_ACF  miniDQM application");
    // error codes
    cmd.addErrorCode(0, "Success");
    cmd.addErrorCode(1, "Error");
    // options
    cmd.setHelpOption("h", "help", "Print this help page");

    cmd.defineOption("file", "Binary Data File", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("file", "f");

    cmd.defineOption("config", "Hw Description File . Default value: settings/HWDescription_2CBC.xml", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("config", "c");

    cmd.defineOption("events", "Number of events to process. Default value: 1000", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("events", "e");

    cmd.defineOption("decode", "Decode s-link events to verify conversion of S-link data in Event class.");
    cmd.defineOptionAlternative("decode", "d");

    int result = cmd.parse(argc, argv);
    if(result != ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(1);
    }
    std::string cHWFile = (cmd.foundOption("config")) ? cmd.optionValue("config") : "settings/HWDescription_2CBC.xml";

    // now query the parsing results
    std::string rawFilename = (cmd.foundOption("file")) ? cmd.optionValue("file") : "";
    int         cNevents    = cmd.foundOption("events") ? convertAnyInt(cmd.optionValue("events").c_str()) : 0;
    bool        cDecode     = (cmd.foundOption("decode"));

    if(rawFilename.empty())
    {
        LOG(ERROR) << "Error, no binary file provided. Quitting";
        exit(2);
    }

    // Check if the file can be found
    if(!boost::filesystem::exists(rawFilename))
    {
        LOG(ERROR) << "Error!! binary file " << rawFilename << " not found, exiting!";
        exit(3);
    }

    // Create the Histogrammer object
    Tool        cTool;
    auto        cFirstLocation = rawFilename.find("/");
    auto        cLastLocation  = rawFilename.find(".daq");
    std::string cRunNumber     = rawFilename.substr(cFirstLocation + 1, cLastLocation - cFirstLocation - 1);
    std::string cDAQFileName   = cRunNumber + ".daq";
    // FileHandler* cDAQFileHandler = nullptr;
    // cDAQFileHandler = new FileHandler(cDAQFileName, 'w');
    // LOG (INFO) << "Writing DAQ File to:   " << cDAQFileName << " - ConditionData, if present, parsed from " <<
    // cHWFile ;

    TString cDirectory = Form("Results/MiniSlinkConverter_%s", cRunNumber.c_str());
    cTool.CreateResultDirectory(cDirectory.Data());
    cTool.InitResultFile("Stubs");
    std::stringstream outp;
    LOG(INFO) << "HWfile=" << cHWFile;
    cTool.InitializeHw(cHWFile, outp);
    // BeBoard* cBoard = static_cast<BeBoard*>(cTool.fDetectorContainer->at ( 0 ));

    // Add File handler
    // cTool.addFileHandler ( rawFilename, 'r' );
    // FileHeader cHeader;
    // cTool.getFileHandler()->getHeader(cHeader);

    // Build the hardware setup
    // LOG (INFO) << outp.str();
    // outp.str ("");

    // Read the first event from the raw data file, needed to retrieve the event map
    // std::vector<uint32_t> dataVec;
    // LOG (INFO) << BOLDBLUE << "Reading .raw file" << RESET;
    // cTool.readFile (dataVec);
    // LOG (INFO) << BOLDBLUE << "Read back " << +dataVec.size() << " 32 bit words from .raw file" << RESET;
    // set data
    // Sarah - fix this....
    /*
    Data* cData = new Data();
    cData->DecodeData (cBoard, dataVec, cNevents, cTool.fBeBoardInterface->getBoardType (cBoard) );
    const std::vector<Event*>& cEvents = cData->GetEvents ( cBoard );
    LOG (INFO) << BOLDBLUE << "Read back " << +cEvents.size() << " events from the .raw file" << RESET;

    LOG (INFO) << BOLDBLUE << "Now writing s-link file from .raw" << RESET;
    for( auto cEvent : cEvents )
    {
        auto cEventCount = cEvent->GetEventCount();
        SLinkEvent cSLev = cEvent->GetSLinkEvent (cBoard);
        cDAQFileHandler->set ( cSLev.getData<uint32_t>() );
    }
    cDAQFileHandler->closeFile();
    */

    if(cDecode)
    {
        // now want to read back s-link file
        cTool.addFileHandler(cDAQFileName, 'r');
        std::vector<uint32_t> dataVecSLink;
        LOG(INFO) << BOLDBLUE << "Reading .daq file" << RESET;
        cTool.readFile(dataVecSLink);
        LOG(INFO) << BOLDBLUE << "Read back " << +dataVecSLink.size() << " 32 bit words from .daq file" << RESET;
        GenericPayload cSlinkPayload;
        for(auto cWord: dataVecSLink)
        {
            LOG(INFO) << BOLDBLUE << std::bitset<32>(cWord) << RESET;
            cSlinkPayload.append(cWord);
        }
        auto cSlinkData = cSlinkPayload.Data<uint64_t>();
        LOG(INFO) << BOLDBLUE << "Slink payload " << +cSlinkData.size() << " 64 bit words." << RESET;
        auto   cIterator   = cSlinkData.begin();
        size_t cEventCount = 0;
        while(cIterator < cSlinkData.end())
        {
            if(cEventCount % 100 == 0) LOG(INFO) << BOLDBLUE << "Decoding event " << +cEventCount << RESET;
            // first - decode event header
            SLinkEventHeader cEventHeader;
            auto&            cDAQHeader      = *cIterator;
            auto&            cTrackerHeader1 = *(cIterator + 1);
            auto&            cTrackerHeader2 = *(cIterator + 2);
            cEventHeader.fDAQHeader          = cDAQHeader;
            cEventHeader.fTrackerHeader.clear();
            cEventHeader.fTrackerHeader.push_back(cTrackerHeader1);
            cEventHeader.fTrackerHeader.push_back(cTrackerHeader2);
            cEventHeader.decode();
            cEventHeader.print();
            if(cEventHeader.fNReadoutChips == 0)
            {
                cIterator = cSlinkData.end();
                continue;
            }
            cIterator += 3;

            // // now - status words
            std::vector<uint64_t> cStatusWords(cIterator, cIterator + cEventHeader.fNStatusBits / 64);
            std::string           cStatus = "";
            for(auto cStatusWord: cStatusWords)
            {
                std::bitset<64> cWord(cStatusWord);
                LOG(INFO) << BOLDYELLOW << "\t.. Status : " << cWord << RESET;
                cStatus += cWord.to_string();
            }
            cIterator += cEventHeader.fNStatusWords;

            // // now - decode hits
            decodeHits(cIterator, cEventHeader);

            // // then decode stubs
            LOG(DEBUG) << BOLDYELLOW << "First S-link word with stub data should : " << std::bitset<64>(*cIterator) << RESET;
            decodeStubs(cIterator, cEventHeader);

            // // next - condition data
            SLinkEventConditionData cEventConditionData;
            LOG(DEBUG) << BOLDYELLOW << "First word of condition data : " << std::bitset<64>(*cIterator) << RESET;
            cEventConditionData.fSize = *(cIterator);
            cIterator++;
            cEventConditionData.set(cIterator);
            cEventConditionData.decode();
            cEventConditionData.print();

            // now - event trailer
            auto& cDAQTrailer = *(cIterator);
            LOG(DEBUG) << BOLDYELLOW << "S-link event DAQ trailer " << std::bitset<64>(cDAQTrailer) << RESET;
            cIterator++;
            LOG(DEBUG) << BOLDBLUE << "At word " << std::distance(cSlinkData.begin(), cIterator) << " of the list of s-link events [list contains " << +cSlinkData.size() << " words]." << RESET;
            cEventCount++;
            // cIterator = cSlinkData.end();
        }
        LOG(INFO) << BOLDBLUE << "Decoded " << +cEventCount << " events out of an expected " << +cNevents << " events from the S-link file." << RESET;
    }

    cTool.Destroy();
    return 0;
}