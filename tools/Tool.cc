#include "tools/Tool.h"
#include <future>
#include <numeric>

#include "HWDescription/Chip.h"
#include "Utils/ChannelGroupHandler.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/Utilities.h"

#include "Utils/ConfigureInfo.h"
#include "Utils/DataContainer.h"
#include "Utils/EmptyContainer.h"
#include "Utils/MPAChannelGroupHandler.h"
#include "Utils/Occupancy.h"
#include "Utils/SSAChannelGroupHandler.h"
#include "Utils/StartInfo.h"

#include "tools/MetadataHandlerIT.h"
#include "tools/MetadataHandlerOT.h"

#include "Parser/FileDumper.h"

using namespace Ph2_System;
using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

std::atomic<bool> Tool::fKeepRunning{false};
std::atomic<int>  Tool::fRunNumber{0};

Tool::Tool()
    : SystemController()
    ,
#ifdef __USE_ROOT__
    fCanvasMap()
    , fChipHistMap()
    , fHybridHistMap()
    , fSummaryTree(nullptr)
    ,
#endif
    fType()
    , fTestGroupChannelMap()
    , fDirectoryName("")
    ,
#ifdef __USE_ROOT__
    fResultFile(nullptr)
    ,
#endif
    fSkipMaskedChannels(false)
    , fAllChan(false)
    , fMaskChannelsFromOtherGroups(false)
    , fTestPulse(false)
    , fDoBoardBroadcast(false)
    , fDoHybridBroadcast(false)
    , fOfStream(nullptr)
{
#if defined __USE_ROOT__ && defined __HTTP__
    fHttpServer = nullptr;
#endif
}

#if defined __USE_ROOT__ && defined __HTTP__
Tool::Tool(THttpServer* pHttpServer)
    : SystemController()
    , fCanvasMap()
    , fChipHistMap()
    , fHybridHistMap()
    , fType()
    , fTestGroupChannelMap()
    , fDirectoryName("")
    , fResultFile(nullptr)
    , fHttpServer(pHttpServer)
    , fSkipMaskedChannels(false)
    , fAllChan(false)
    , fMaskChannelsFromOtherGroups(false)
    , fTestPulse(false)
    , fDoBoardBroadcast(false)
    , fDoHybridBroadcast(false)
{
}
#endif

Tool::Tool(const Tool& pTool) { this->Inherit(&pTool); }

Tool::~Tool() {}

// @Mauro@
// void Tool::privateRunning(std::promise<int>&& thePromise)
// {
//     try
//     {
//         Running();
//         Tool::InformImDone();
//         thePromise.set_value(0);
//     }
//     catch(...)
//     {
//         Tool::InformImDone();
//         thePromise.set_value(99);
//         thePromise.set_exception(std::current_exception());
//     }
// }

bool Tool::GetRunningStatus()
{
    std::future_status runningStatus = fRunningFuture.wait_for(std::chrono::milliseconds(500u));
    if(runningStatus == std::future_status::ready || runningStatus == std::future_status::deferred)
    {
        try
        {
            if(fRunningFuture.valid()) fRunningFuture.get();
        }
        catch(const std::future_error& e)
        {
            LOG(INFO) << "Ignoring future exception, future already retrieved";
        }
        catch(const std::exception& e)
        {
            throw std::runtime_error(e.what());
        }
        return true;
    }
    else
        return false;
}

void Tool::waitForRunToBeCompleted()
{
    try
    {
        if(fRunningFuture.valid()) fRunningFuture.wait();
    }
    catch(const std::future_error& e)
    {
        LOG(INFO) << "Ignoring future exception, future already retrieved";
    }
    // @Mauro@
    // std::unique_lock<std::recursive_mutex> theGuard(theMtx);
    // wakeUp.wait(theGuard, [this]() { return doExit; });
}

void Tool::Configure(const ConfigureInfo& theConfigureInfo, bool pReInitialize)
{
    std::string startOfTestTime = getTimeStampString();
    SystemController::Configure(theConfigureInfo, pReInitialize);

    if(fBoardType == BoardType::D19C)
        fMetadataHandler = new MetadataHandlerOT(startOfTestTime);
    else if(fBoardType == BoardType::RD53)
        fMetadataHandler = new MetadataHandlerIT(startOfTestTime);
    else
    {
        LOG(ERROR) << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Board type not defined!! Impossible to create DQM for metadata, aborting..." << std::endl;
        abort();
    }

    ConfigureCalibration();
}

void Tool::Start(const StartInfo& theStartInfo)
{
    LOG(DEBUG) << BOLDBLUE << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] fDirectoryName = " << BOLDYELLOW << fDirectoryName << RESET;
    if(fDirectoryName == "")
    {
        std::string resultDirectory = getResultDirectoryName(theStartInfo);
        CreateResultDirectory(resultDirectory, false, false);
    }

    InitResultFile("Results");

    if(fMetadataHandler != nullptr)
    {
        fMetadataHandler->Inherit(this);
        fMetadataHandler->initMetadata();
        fMetadataHandler->fillInitialConditions();
    }

    // doExit             = false; // @Mauro@
    Tool::fKeepRunning = true;
    fRunNumber         = theStartInfo.getRunNumber();
    fRunningFuture     = std::async(std::launch::async, &Tool::Running, this); // @Fabio@
    // @Mauro@
    // std::promise<int> thePromise;
    // fRunningFuture = thePromise.get_future();
    // fRunningThread = std::thread(&Tool::privateRunning, this, std::move(thePromise));
}

// @Mauro@
// void Tool::InformImDone()
// {
//     std::unique_lock<std::recursive_mutex> theGuard(theMtx);
//     doExit = true;
//     theGuard.unlock();
//     wakeUp.notify_one();
// }

void Tool::readBitslipRegs()
{
    auto getRegisterName = [](const std::string& type, size_t linkNumber, size_t hybridId)
    {
        std::stringstream registerNameStream;
        registerNameStream << std::hex << "fc7_daq_ctrl.physical_interface_block.link" << std::uppercase << linkNumber << "_hybrid" << hybridId << "_" << type << "_bitslip" << std::dec;
        return registerNameStream.str();
    };

    std::vector<std::pair<std::string, uint32_t>> alignedBitslipRegisters;
    for(size_t linkNumber = 0; linkNumber < 12; ++linkNumber)
    {
        for(size_t hybridId = 0; hybridId < 2; ++hybridId)
        {
            alignedBitslipRegisters.push_back({getRegisterName("stub", linkNumber, hybridId), 0xFFFFFFFF});
            alignedBitslipRegisters.push_back({getRegisterName("L1A", linkNumber, hybridId), 0xFFFFFFFF});
        }
    }

    // Reading all bitslip registers
    fBeBoardInterface->ReadBoardMultReg(fDetectorContainer->getFirstObject(), alignedBitslipRegisters);

    for(const auto& registerNameAndValue: alignedBitslipRegisters)
    {
        LOG(INFO) << BOLDYELLOW << "Reading  " << registerNameAndValue.first << " = 0x" << std::hex << registerNameAndValue.second << std::dec << RESET;
    }
}

void Tool::Stop()
{
    if(Tool::fKeepRunning == true)
    {
        Tool::fKeepRunning = false;
        Tool::waitForRunToBeCompleted();
        // if(fRunningThread.joinable() == true) fRunningThread.join(); // @Mauro@
        try
        {
            if(fRunningFuture.valid()) fRunningFuture.get();
        }
        catch(const std::future_error& e)
        {
            LOG(INFO) << "Ignoring future exception, future already retrieved";
        }
        catch(const std::exception& e)
        {
            throw std::runtime_error(e.what());
        }

        SystemController::Stop();
        Tool::dumpConfigFiles();
        if(fMetadataHandler != nullptr) fMetadataHandler->fillFinalConditions();

        if(fDQMStreamerEnabled == true)
        {
            std::string  doneWithRunMessage = END_OF_TRANSMISSION_MESSAGE;
            PacketHeader thePacketHeader;
            thePacketHeader.addPacketHeader(doneWithRunMessage);
            fDQMStreamer->broadcast(doneWithRunMessage);
        }

        Tool::SaveResults();
        Tool::WriteRootFile();
    }
}

void Tool::Inherit(const Tool* pTool)
{
    SystemController::Inherit(pTool);

#ifdef __USE_ROOT__
    fResultFile = pTool->fResultFile;
#endif
    fType          = pTool->fType;
    fDirectoryName = pTool->fDirectoryName;
#ifdef __USE_ROOT__
    fSummaryTree          = pTool->fSummaryTree;
    fCanvasMap            = pTool->fCanvasMap;
    fChipHistMap          = pTool->fChipHistMap;
    fHybridHistMap        = pTool->fHybridHistMap;
    fBeBoardHistMap       = pTool->fBeBoardHistMap;
    fSummaryTreeParameter = pTool->fSummaryTreeParameter;
    fSummaryTreeValue     = pTool->fSummaryTreeValue;
#endif
    fTestGroupChannelMap         = pTool->fTestGroupChannelMap;
    fSkipMaskedChannels          = pTool->fSkipMaskedChannels;
    fAllChan                     = pTool->fAllChan;
    fMaskForTestGroupChannelMap  = pTool->fMaskForTestGroupChannelMap;
    fMaskChannelsFromOtherGroups = pTool->fMaskChannelsFromOtherGroups;
    fTestPulse                   = pTool->fTestPulse;
    fDoBoardBroadcast            = pTool->fDoBoardBroadcast;
    fDoHybridBroadcast           = pTool->fDoHybridBroadcast;
    fDirectoryName               = pTool->fDirectoryName;
    fResultFileName              = pTool->fResultFileName;
    fUseReadNEvents              = pTool->fUseReadNEvents;
    fNReadbackEvents             = pTool->fNReadbackEvents;
    fNormalize                   = pTool->fNormalize;
    fOfStream                    = pTool->fOfStream;
    fMetadataHandler             = pTool->fMetadataHandler;

#if defined __USE_ROOT__ && defined __HTTP__
    fHttpServer = pTool->fHttpServer;
#endif
}

void Tool::Inherit(const SystemController* pSystemController) { SystemController::Inherit(pSystemController); }

void Tool::resetPointers() {}

void Tool::Destroy()
{
    LOG(INFO) << BOLDRED << "Destroying memory objects" << RESET;
#if defined __USE_ROOT__ && defined __HTTP__
    LOG(INFO) << BOLDRED << "Destroying HttpServer" << RESET;
    if(fHttpServer)
    {
        delete fHttpServer;
        fHttpServer = nullptr;
    }
    LOG(INFO) << BOLDRED << "HttpServer Destroyed" << RESET;
#endif

    SoftDestroy();
    LOG(INFO) << BOLDRED << "Memory objects destroyed" << RESET;
    SystemController::Destroy();
}

void Tool::SoftDestroy()
{
    if(fOfStream != nullptr)
    {
        delete fOfStream;
        fOfStream = nullptr;
    }
    if(fMetadataHandler != nullptr)
    {
        delete fMetadataHandler;
        fMetadataHandler = nullptr;
    }
#ifdef __USE_ROOT__
    if(fResultFile != nullptr)
    {
        if(fResultFile->IsOpen()) fResultFile->Close();

        if(fResultFile)
        {
            delete fResultFile;
            fResultFile = nullptr;
        }
    }

    for(auto canvas: fCanvasMap)
    {
        delete canvas.second;
        canvas.second = nullptr;
    }
    fCanvasMap.clear();
    for(auto chip: fChipHistMap)
    {
        for(auto hist: chip.second)
        {
            delete hist.second;
            hist.second = nullptr;
        }
    }
    fChipHistMap.clear();
    for(auto chip: fHybridHistMap)
    {
        for(auto hist: chip.second)
        {
            delete hist.second;
            hist.second = nullptr;
        }
    }
    fHybridHistMap.clear();
    for(auto chip: fBeBoardHistMap)
    {
        for(auto hist: chip.second)
        {
            delete hist.second;
            hist.second = nullptr;
        }
    }
    fBeBoardHistMap.clear();
#endif
    fTestGroupChannelMap.clear();
}

#ifdef __USE_ROOT__
std::string Tool::fSummaryTreeParameter = "";
double      Tool::fSummaryTreeValue     = 0.0;

/*!
 * \brief Initialize a 'summary' TTree in the ROOT File, with branches 'parameter'(string) and 'value'(double)
 */
void Tool::bookSummaryTree()
{
    fResultFile->cd();
    fSummaryTreeParameter = "";
    fSummaryTreeValue     = 0;
    fSummaryTree          = new TTree("summaryTree", "Most relevant results");
    fSummaryTree->Branch("Parameter", &fSummaryTreeParameter);
    fSummaryTree->Branch("Value", &fSummaryTreeValue);
}

/*!
 * \brief Insert data into the summary tree
 * \param cParameter : Name of the measurement to be stored
 * \param cValue: Value of the measurement to be stored
 */
void Tool::fillSummaryTree(std::string cParameter, Double_t cValue) // MINE
{
    fResultFile->cd();
    fSummaryTreeParameter.clear();
    TString cParameter_TString(cParameter);
    fSummaryTreeParameter = cParameter_TString;
    fSummaryTreeValue     = cValue;
    if(fSummaryTree) fSummaryTree->Fill();
}

Double_t Tool::getSummaryParameter(std::string cParameter)
{
    for(int i = 0; i < fSummaryTree->GetEntries(); i++)
    {
        fSummaryTree->GetEntry(i);
        if(fSummaryTreeParameter == cParameter) return fSummaryTreeValue;
    }
    return -1.0;
}

void Tool::bookHistogram(ChipContainer* pChip, std::string pName, TObject* pObject)
{
    TH1* tmpHistogramPointer = dynamic_cast<TH1*>(pObject);
    if(tmpHistogramPointer != nullptr) tmpHistogramPointer->SetDirectory(0);

    // Find or create map<string,TOBject> for specific CBC
    auto cChipHistMap = fChipHistMap.find(pChip);

    if(cChipHistMap == std::end(fChipHistMap))
    {
        // Fabio: CBC specific -> to be moved out from Tool
        LOG(INFO) << "Histo Map for CBC " << int(pChip->getId()) << " (FE " << int(static_cast<ReadoutChip*>(pChip)->getHybridId()) << ") does not exist - creating ";
        std::map<std::string, TObject*> cTempChipMap;

        fChipHistMap[pChip] = cTempChipMap;
        cChipHistMap        = fChipHistMap.find(pChip);
    }

    // Find histogram with given name: if it exists, delete the object, if not create
    auto cHisto = cChipHistMap->second.find(pName);

    if(cHisto != std::end(cChipHistMap->second)) cChipHistMap->second.erase(cHisto);

    cChipHistMap->second[pName] = pObject;
#if defined __USE_ROOT__ && defined __HTTP__
    if(fHttpServer) fHttpServer->Register("/Histograms", pObject);
#endif
}

void Tool::bookHistogram(HybridContainer* pHybrid, std::string pName, TObject* pObject)
{
    TH1* tmpHistogramPointer = dynamic_cast<TH1*>(pObject);
    if(tmpHistogramPointer != nullptr) tmpHistogramPointer->SetDirectory(0);

    // Find or create map<string,TOBject> for specific CBC
    auto cHybridHistMap = fHybridHistMap.find(pHybrid);

    if(cHybridHistMap == std::end(fHybridHistMap))
    {
        LOG(INFO) << "Histo Map for Hybrid " << int(pHybrid->getId()) << " does not exist - creating ";
        std::map<std::string, TObject*> cTempHybridMap;

        fHybridHistMap[pHybrid] = cTempHybridMap;
        cHybridHistMap          = fHybridHistMap.find(pHybrid);
    }

    // Find histogram with given name: if it exists, delete the object, if not create
    auto cHisto = cHybridHistMap->second.find(pName);

    if(cHisto != std::end(cHybridHistMap->second)) cHybridHistMap->second.erase(cHisto);

    cHybridHistMap->second[pName] = pObject;
#if defined __USE_ROOT__ && defined __HTTP__
    if(fHttpServer) fHttpServer->Register("/Histograms", pObject);
#endif
}

void Tool::bookHistogram(BoardContainer* pBeBoard, std::string pName, TObject* pObject)
{
    TH1* tmpHistogramPointer = dynamic_cast<TH1*>(pObject);
    if(tmpHistogramPointer != nullptr) tmpHistogramPointer->SetDirectory(0);

    // Find or create map<string,TOBject> for specific CBC
    auto cBeBoardHistMap = fBeBoardHistMap.find(pBeBoard);

    if(cBeBoardHistMap == std::end(fBeBoardHistMap))
    {
        LOG(INFO) << "Histo Map for Hybrid " << int(pBeBoard->getId()) << " does not exist - creating ";
        std::map<std::string, TObject*> cTempHybridMap;

        fBeBoardHistMap[pBeBoard] = cTempHybridMap;
        cBeBoardHistMap           = fBeBoardHistMap.find(pBeBoard);
    }

    // Find histogram with given name: if it exists, delete the object, if not create
    auto cHisto = cBeBoardHistMap->second.find(pName);

    if(cHisto != std::end(cBeBoardHistMap->second)) cBeBoardHistMap->second.erase(cHisto);

    cBeBoardHistMap->second[pName] = pObject;
#if defined __USE_ROOT__ && defined __HTTP__
    if(fHttpServer) fHttpServer->Register("/Histograms", pObject);
#endif
}

TObject* Tool::getHist(ChipContainer* pChip, std::string pName)
{
    auto cChipHistMap = fChipHistMap.find(pChip);

    if(cChipHistMap == std::end(fChipHistMap))
    {
        // Fabio: CBC specific -> to be moved out from Tool
        LOG(ERROR) << BOLDRED << "Error: could not find the Histograms for CBC " << int(pChip->getId()) << " (FE " << int(static_cast<ReadoutChip*>(pChip)->getHybridId()) << ")" << RESET;
        return nullptr;
    }
    else
    {
        auto cHisto = cChipHistMap->second.find(pName);

        if(cHisto == std::end(cChipHistMap->second))
        {
            LOG(ERROR) << BOLDRED << "Error: could not find the Histogram with the name " << BOLDYELLOW << pName << RESET;
            return nullptr;
        }
        else
            return cHisto->second;
    }
}

TObject* Tool::getHist(HybridContainer* pHybrid, std::string pName)
{
    auto cHybridHistMap = fHybridHistMap.find(pHybrid);

    if(cHybridHistMap == std::end(fHybridHistMap))
    {
        LOG(ERROR) << BOLDRED << "Error: could not find the Histograms for Hybrid " << BOLDYELLOW << int(pHybrid->getId()) << RESET;
        return nullptr;
    }
    else
    {
        auto cHisto = cHybridHistMap->second.find(pName);

        if(cHisto == std::end(cHybridHistMap->second))
        {
            LOG(ERROR) << BOLDRED << "Error: could not find the Histogram with the name " << BOLDYELLOW << pName << RESET;
            return nullptr;
        }
        else
            return cHisto->second;
    }
}

TObject* Tool::getHist(BoardContainer* pBeBoard, std::string pName)
{
    auto cBeBoardHistMap = fBeBoardHistMap.find(pBeBoard);

    if(cBeBoardHistMap == std::end(fBeBoardHistMap))
    {
        LOG(ERROR) << BOLDRED << "Error: could not find the Histograms for Hybrid " << BOLDYELLOW << int(pBeBoard->getId()) << RESET;
        return nullptr;
    }
    else
    {
        auto cHisto = cBeBoardHistMap->second.find(pName);

        if(cHisto == std::end(cBeBoardHistMap->second))
        {
            LOG(ERROR) << BOLDRED << "Error: could not find the Histogram with the name " << BOLDYELLOW << pName << RESET;
            return nullptr;
        }
        else
            return cHisto->second;
    }
}
#endif

void Tool::WriteRootFile()
{
#ifdef __USE_ROOT__
    if((fResultFile != nullptr) && (fResultFile->IsOpen() == true)) fResultFile->Write();
#endif
}

void Tool::SaveResults()
{
#ifdef __USE_ROOT__
    for(const auto& cBeBoard: fBeBoardHistMap)
    {
        fResultFile->cd();

        for(const auto& cHist: cBeBoard.second) cHist.second->Write(cHist.second->GetName(), TObject::kOverwrite);

        fResultFile->cd();
    }

    // Now per FE
    for(const auto& cHybrid: fHybridHistMap)
    {
        TString  cDirName = Form("FE%d", cHybrid.first->getId());
        TObject* cObj     = gROOT->FindObject(cDirName);

        if(!cObj) fResultFile->mkdir(cDirName);

        fResultFile->cd(cDirName);

        for(const auto& cHist: cHybrid.second) cHist.second->Write(cHist.second->GetName(), TObject::kOverwrite);

        fResultFile->cd();
    }

    for(const auto& cChip: fChipHistMap)
    {
        std::string cDescr = "";
        auto        cType  = static_cast<ReadoutChip*>(cChip.first)->getFrontEndType();
        if(cType == FrontEndType::CBC3) cDescr = "CBC";
        if(cType == FrontEndType::SSA2) cDescr = "SSA2";
        if(cType == FrontEndType::MPA2) cDescr = "MPA2";

        // Fabio: CBC specific -> to be moved out from Tool
        TString  cDirName = Form("Hybrid%d%s%d", static_cast<ReadoutChip*>(cChip.first)->getHybridId(), cDescr.c_str(), cChip.first->getId());
        TObject* cObj     = gROOT->FindObject(cDirName);

        if(!cObj) fResultFile->mkdir(cDirName);

        fResultFile->cd(cDirName);

        for(const auto& cHist: cChip.second) cHist.second->Write(cHist.second->GetName(), TObject::kOverwrite);

        fResultFile->cd();
    }

    // Save Canvasses too
    for(const auto& cCanvas: fCanvasMap)
    {
        cCanvas.second->Write(cCanvas.second->GetName(), TObject::kOverwrite);
        std::string cPdfName = fDirectoryName + "/" + cCanvas.second->GetName() + ".pdf";
        cCanvas.second->SaveAs(cPdfName.c_str());
    }

    // Save summary TTree
    if((fResultFile != nullptr) && (fResultFile->IsOpen() == true)) fResultFile->cd();
#endif
}

void Tool::CreateResultDirectory(const std::string& pDirname, bool pMode, bool pDate)
{
    std::string nDirname;

    if(std::getenv("GIPHT_RESULT_FOLDER"))
    {
        LOG(INFO) << "OT Module GUI (GIPHT) result directory environmental variable set: " << std::getenv("GIPHT_RESULT_FOLDER");
        nDirname = std::getenv("GIPHT_RESULT_FOLDER");
    }
    else if(std::getenv("OTSDAQ_RESULTS_FOLDER"))
    {
        LOG(INFO) << "OTSDAQ result directory environmental variable set: " << std::getenv("OTSDAQ_RESULTS_FOLDER");
        nDirname = std::string(std::getenv("OTSDAQ_RESULTS_FOLDER")) + "/" + pDirname + "/";
    }
    else
        nDirname = pDirname;
    if(pDate) nDirname += currentDateTime();

    std::string cCommand = "mkdir -p " + nDirname;

    try
    {
        system(cCommand.c_str());
    }
    catch(std::exception& e)
    {
        LOG(ERROR) << BOLDRED << "Exception when trying to create Result Directory: " << BOLDYELLOW << e.what() << RESET;
    }

    fDirectoryName = nDirname;
}

/*!
 * \brief Initialize the result Root file
 * \param pFilename : Root filename
 */
void Tool::InitResultFile(const std::string& pFilename)
{
#ifdef __USE_ROOT__
    if(fResultFile != nullptr) return;
    if(!fDirectoryName.empty())
    {
        std::string cFilename = fDirectoryName + "/" + pFilename + ".root";

        try
        {
            fResultFile     = TFile::Open(cFilename.c_str(), "RECREATE");
            fResultFileName = cFilename;
            // AddMetadata();
        }
        catch(std::exception& e)
        {
            LOG(ERROR) << BOLDRED << "Exceptin when trying to create Result File: " << BOLDYELLOW << e.what() << RESET;
        }
    }
    else
        LOG(WARNING) << BOLDRED << "Error: " << BOLDYELLOW << "no result directory initialized - not saving results" << RESET;
#endif
}

void Tool::CloseResultFile()
{
#ifdef __USE_ROOT__
    if(fResultFile != nullptr)
    {
        LOG(INFO) << GREEN << "Closing result file: " << BOLDYELLOW << fResultFileName << RESET;
        fResultFile->Close();
        delete fResultFile;
        fResultFile = nullptr;
    }
#endif
}

#ifdef __USE_ROOT__
void Tool::AddMetadata()
{
    fResultFile->cd();
    TTree* t = new TTree();
    t->SetName("metadata");

    // Save username
    std::string user;
    try
    {
        user = std::string(std::getenv("USER"));
    }
    catch(const std::exception& e)
    {
        LOG(WARNING) << e.what();
        LOG(WARNING) << __PRETTY_FUNCTION__ << " Username not set, using dummy name";
        user = "user";
    }
    t->Branch("username", &user);

    // save chip IDs
    std::vector<uint32_t> chipIds;

    uint16_t chipVectorSize = 0;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup) { chipVectorSize += cHybrid->size(); }
        }
    }
    chipIds.reserve(chipVectorSize); // Need to be done to avoid jumps of the vector memory that messes with the pointer associated to the branch

    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    std::string label = getReadoutChipString(cBoard->getId(), cOpticalGroup->getId(), cHybrid->getId(), cChip->getId());

                    uint32_t value = fReadoutChipInterface->ReadChipFuseID(cChip);

                    chipIds.push_back(value);
                    // this is ok because we will only set one value per branch
                    t->Branch(label.c_str(), &chipIds.back());
                }
            }
        }
    }

    t->Fill();

    t->Write();
    fResultFile->Write();
}

void Tool::StartHttpServer(const int pPort, bool pReadonly)
{
#if defined __USE_ROOT__ && defined __HTTP__

    if(fHttpServer)
    {
        delete fHttpServer;
        fHttpServer = nullptr;
    }

    char hostname[HOST_NAME_MAX];

    try
    {
        fHttpServer = new THttpServer(Form("http:%d", pPort));
        fHttpServer->SetReadOnly(pReadonly);
        fHttpServer->SetTimer(0, kTRUE);
        fHttpServer->SetJSROOT("https://root.cern.ch/js/latest/");

        // Configure the server
        // see: https://root.cern.ch/gitweb/?p=root.git;a=blob_plain;f=tutorials/http/httpcontrol.C;hb=HEAD
        fHttpServer->SetItemField("/", "_monitoring", "5000");
        fHttpServer->SetItemField("/", "_layout", "grid2x2");

        gethostname(hostname, HOST_NAME_MAX);
    }
    catch(std::exception& e)
    {
        LOG(ERROR) << BOLDRED << "Exception when trying to start THttpServer: " << BOLDYELLOW << e.what() << RESET;
    }

    LOG(INFO) << "Opening THttpServer on port " << pPort << ". Point your browser to: " << GREEN << hostname << ":" << pPort << RESET;
#else
    LOG(INFO) << "Error, ROOT version < 5.34 detected or not compiled with Http Server support!"
              << " No THttpServer available! - The webgui will fail to show plots!";
    LOG(INFO) << "ROOT must be built with '--enable-http' flag to use this feature.";
#endif
}

void Tool::HttpServerProcess()
{
#if defined __USE_ROOT__ && defined __HTTP__

    if(fHttpServer)
    {
        gSystem->ProcessEvents();
        fHttpServer->ProcessRequests();
    }

#endif
}
#endif

void Tool::readAllReadOnlyRegisters()
{
    for(auto theBoard: *fDetectorContainer)
    {
        LOG(INFO) << BOLDYELLOW << "Reading all readable registers for BeBoard " << +theBoard->getId() << RESET;

        const auto theBeBoardFW = this->fBeBoardFWMap.at(theBoard->getId());
        const auto hwInterface  = theBeBoardFW->getHardwareInterface();

        auto theBoardFreeRegisterRegex = theBoard->getFreeRegisterRegex();

        for(const auto& path: hwInterface->getNodes())
        {
            const auto& node = hwInterface->getNode(path);

            if((node.getPermission() == uhal::defs::READWRITE) && (++node.begin() == node.end()))
            {
                bool isFreeRegister = false;
                for(const auto& freeRegister: theBoardFreeRegisterRegex)
                {
                    isFreeRegister = std::regex_match(path, freeRegister.first);
                    if(isFreeRegister) break;
                }
                if(isFreeRegister) continue;
                fBeBoardInterface->ReadBoardReg(theBoard, path);
            }
        }

        for(auto theOpticalGroup: *theBoard)
        {
            auto theLpGBT = theOpticalGroup->flpGBT;
            if(theLpGBT != nullptr)
            {
                LOG(INFO) << BOLDYELLOW << "Reading read-only registers for BeBoard " << +theBoard->getId() << " OpticalGroup " << +theOpticalGroup->getId() << " LpGBT" << RESET;
                flpGBTInterface->ReadChipMultReg(theLpGBT, theLpGBT->getReadOnlyRegisterList());
            }
            for(auto theHybrid: *theOpticalGroup)
            {
                if(fCicInterface != nullptr)
                {
                    LOG(INFO) << BOLDYELLOW << "Reading read-only registers for BeBoard " << +theBoard->getId() << " OpticalGroup " << +theOpticalGroup->getId() << " Hybrid " << +theHybrid->getId()
                              << " CIC" << RESET;
                    auto theCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                    fCicInterface->ReadChipMultReg(theCic, theCic->getReadOnlyRegisterList());
                }
                FrontEndType             theCurrentFrontEndType = FrontEndType::UNDEFINED;
                std::vector<std::string> readOnlyRegisters{};
                for(auto theChip: *theHybrid)
                {
                    if(theCurrentFrontEndType != theChip->getFrontEndType())
                    {
                        theCurrentFrontEndType = theChip->getFrontEndType();
                        readOnlyRegisters.clear();
                        readOnlyRegisters = theChip->getReadOnlyRegisterList();
                    }
                    LOG(INFO) << BOLDYELLOW << "Reading read-only registers for BeBoard " << +theBoard->getId() << " OpticalGroup " << +theOpticalGroup->getId() << " Hybrid " << +theHybrid->getId()
                              << " Chip " << +theChip->getId() << RESET;
                    fReadoutChipInterface->ReadChipMultReg(theChip, readOnlyRegisters);
                }
            }
        }
    }
}

void Tool::dumpConfigFiles(bool checkReadOnlyRegisters)
{
    if(fDetectorContainer->getFirstObject()->getBoardType() == BoardType::RD53) return; // IT does not dump the files

    LOG(INFO) << BOLDBLUE << "Reading all the read-only registers to update output files" << RESET;

    if(checkReadOnlyRegisters) readAllReadOnlyRegisters();

    if(!fDirectoryName.empty())
    {
        FileDumper theFileDumper(fDirectoryName);
        auto       fileDumpStream = theFileDumper.dumpConfigurationFiles(fDetectorContainer, fSettingsMap, fCommunicationSettingConfig, fDetectorMonitorConfig).str();
        if(fMetadataHandler != nullptr) fMetadataHandler->setFinalConfigurationFileContent(fileDumpStream);
    }
    else
    {
        LOG(ERROR) << BOLDRED << "Error: " << BOLDYELLOW << "no results Directory initialized" << RESET;
        abort();
    }
}

void Tool::setSystemTestPulse(uint8_t pTPAmplitude, uint8_t pTestGroup, bool pTPState, bool pHoleMode)
{
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    // Fabio: CBC specific but not used by common scans - BEGIN
                    // first, get the Amux Value
                    uint8_t cOriginalAmuxValue;
                    cOriginalAmuxValue = cChip->getReg("MiscTestPulseCtrl&AnalogMux");
                    // uint8_t cOriginalHitDetectSLVSValue = cChip->getReg ("HitDetectSLVS" );

                    std::vector<std::pair<std::string, uint16_t>> cRegVec;
                    uint8_t                                       cRegValue = to_reg(0, pTestGroup);

                    if(cChip->getFrontEndType() == FrontEndType::CBC3)
                    {
                        uint8_t cTPRegValue;

                        if(pTPState)
                            cTPRegValue = (cOriginalAmuxValue | 0x1 << 6);
                        else if(!pTPState)
                            cTPRegValue = (cOriginalAmuxValue & ~(0x1 << 6));

                        // uint8_t cHitDetectSLVSValue = (cOriginalHitDetectSLVSValue & ~(0x1 << 6));

                        // cRegVec.push_back ( std::make_pair ( "HitDetectSLVS", cHitDetectSLVSValue ) );
                        cRegVec.push_back(std::make_pair("MiscTestPulseCtrl&AnalogMux", cTPRegValue));
                        cRegVec.push_back(std::make_pair("TestPulseDel&ChanGroup", cRegValue));
                        cRegVec.push_back(std::make_pair("TestPulsePotNodeSel", pTPAmplitude));
                        LOG(DEBUG) << BOLDBLUE << "Read original Amux Value to be: " << std::bitset<8>(cOriginalAmuxValue) << " and changed to " << std::bitset<8>(cTPRegValue) << " - the TP is bit 6!"
                                   << RESET;
                    }

                    this->fReadoutChipInterface->WriteChipMultReg(cChip, cRegVec);
                    // Fabio: CBC specific but not used by common scans - END
                }
            }
        }
    }
}

void Tool::enableTestPulse(bool enableTP)
{
    setFWTestPulse(enableTP);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid) { fReadoutChipInterface->enableInjection(cChip, enableTP); }
            }
        }
    }
}

void Tool::selectGroupTestPulse(Chip* cChip, uint8_t pTestGroup)
{
    switch(cChip->getFrontEndType())
    {
    case FrontEndType::CBC3:
    {
        uint8_t cRegValue = to_reg(0, pTestGroup);
        this->fReadoutChipInterface->WriteChipReg(cChip, "TestPulseDel&ChanGroup", cRegValue);
        break;
    }

    default:
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " FrontEnd type not recognized for Bebord " << cChip->getBeBoardId() << " Hybrid " << cChip->getHybridId() << " Chip " << +cChip->getId()
                   << ", aborting" << RESET;
        throw("[Tool::selectGroupTestPulse]\tError, FrontEnd type not found");
        break;
    }
    }
}

void Tool::setFWTestPulse(bool inject)
{
    fTestPulse = inject;
    for(auto cBoard: *fDetectorContainer)
    {
        std::vector<std::pair<std::string, uint32_t>> cRegVec;
        if(cBoard->getBoardType() == BoardType::D19C)
        {
            EventType cEventType = cBoard->getEventType();
            bool      cAsync     = (cEventType == EventType::PSAS);

            if(!cAsync)
            {
                if(inject)
                    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", 6});
                else
                    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", 3});
            }
            else
            {
                cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", 12});
                if(!inject)
                {
                    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " Counter readout without injection not implemented, aborting" << RESET;
                    throw("[Tool::setFWTestPulse]\tError, Counter readout without injection not implemented");
                }
            }
            cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
        }
        else
        {
            LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " setFWTestPulse not available for type of board with id " << cBoard->getId() << ", aborting" << RESET;
            throw("[Tool::setFWTestPulse]\tError, setFWTestPulse not available for board with type different from D19C");
        }

        fBeBoardInterface->WriteBoardMultReg(cBoard, cRegVec);
    }
}

void Tool::CreateReport()
{
    std::ofstream report;
    report.open(fDirectoryName + "/TestReport.txt", std::ofstream::out | std::ofstream::app);
    report.close();
}

void Tool::AmmendReport(std::string pString)
{
    std::ofstream report;
    report.open(fDirectoryName + "/TestReport.txt", std::ofstream::out | std::ofstream::app);
    report << pString << std::endl;
    report.close();
}

std::pair<float, float> Tool::getStats(std::vector<float> pData)
{
    float              cMean = std::accumulate(pData.begin(), pData.end(), 0.) / pData.size();
    std::vector<float> cTmp(pData.size(), 0);
    std::transform(pData.begin(), pData.end(), cTmp.begin(), [&](float el) { return (el - cMean) * (el - cMean); });
    float cStandardDeviation = std::sqrt(std::accumulate(cTmp.begin(), cTmp.end(), 0.) / (cTmp.size() - 1.));
    return std::make_pair(cMean, cStandardDeviation);
}

std::pair<std::vector<float>, std::vector<float>> Tool::getDerivative(std::vector<float> pData, std::vector<float> pValues, bool pIgnoreNegative)
{
    std::vector<float> cWeights(pData.size());
    std::adjacent_difference(pData.begin(), pData.end(), cWeights.begin());
    // replace negative entries with 0s
    if(pIgnoreNegative) std::replace_if(cWeights.begin(), cWeights.end(), [](float i) { return std::signbit(i); }, 0);
    cWeights.erase(cWeights.begin(), cWeights.begin() + 1);
    pValues.erase(pValues.begin(), pValues.begin() + 1);
    return std::make_pair(cWeights, pValues);
}

std::pair<float, float> Tool::evalNoise(std::vector<float> pData, std::vector<float> pValues, bool pIgnoreNegative)
{
    std::vector<float> cWeights(pData.size());
    std::adjacent_difference(pData.begin(), pData.end(), cWeights.begin());
    cWeights.erase(cWeights.begin(), cWeights.begin() + 1);
    if(pIgnoreNegative) std::replace_if(cWeights.begin(), cWeights.end(), [](float i) { return std::signbit(i); }, 0);
    float cN            = static_cast<float>(cWeights.size() - std::count(cWeights.begin(), cWeights.end(), 0.));
    float cSumOfWeights = std::accumulate(cWeights.begin(), cWeights.end(), 0.);
    // Weighted sum of scan values to get pedestal
    pData.erase(pData.begin(), pData.begin() + 1);
    std::fill(pData.begin(), pData.end(), 0.);
    std::transform(cWeights.begin(), cWeights.end(), pValues.begin(), pData.begin(), std::multiplies<float>());
    float cMean = std::accumulate(pData.begin(), pData.end(), 0.);
    cMean /= cSumOfWeights;
    // Weighted sample variance of scan values to get noise
    std::transform(pValues.begin(), pValues.end(), pData.begin(), [&](float el) { return (el - cMean) * (el - cMean); });
    std::transform(cWeights.begin(), cWeights.end(), pData.begin(), pData.begin(), std::multiplies<float>());
    float cCorrection = std::sqrt(cN / (cN - 1.));
    float cNoise      = (std::accumulate(pData.begin(), pData.end(), 0.) / (cSumOfWeights));
    cNoise            = std::sqrt(cNoise);
    LOG(DEBUG) << BOLDBLUE << "Noise is " << cNoise << " -- sum of weights  " << cSumOfWeights << " " << cN << " non zero-elements : correction of " << cCorrection << RESET;
    return std::make_pair(cMean, cNoise * cCorrection);
}

// Then a method to un-mask pairs of channels on a given CBC
void Tool::unmaskPair(Chip* cChip, std::pair<uint8_t, uint8_t> pPair)
{
    // Fabio: CBC specific but not used by common scans - BEGIN

    // Get ready to mask/un-mask channels in pairs...
    MaskedChannelsList cMaskedList;
    MaskedChannels     cMaskedChannels;
    cMaskedChannels.clear();
    cMaskedChannels.push_back(pPair.first);

    uint8_t     cRegisterId  = pPair.first >> 3;
    std::string cMaskRegName = fChannelMaskMapCBC3.at(cRegisterId);
    cMaskedList.insert(std::pair<std::string, MaskedChannels>(cMaskRegName.c_str(), cMaskedChannels));

    cRegisterId  = pPair.second >> 3;
    cMaskRegName = fChannelMaskMapCBC3.at(cRegisterId);
    auto it      = cMaskedList.find(cMaskRegName.c_str());
    if(it != cMaskedList.end()) { (it->second).push_back(pPair.second); }
    else
    {
        cMaskedChannels.clear();
        cMaskedChannels.push_back(pPair.second);
        cMaskedList.insert(std::pair<std::string, MaskedChannels>(cMaskRegName.c_str(), cMaskedChannels));
    }

    // Do the actual channel un-masking
    for(auto cMasked: cMaskedList)
    {
        uint8_t     cRegValue = 0; // cChip->getReg (cMasked.first);
        std::string cOutput   = "";
        for(auto cMaskedChannel: cMasked.second)
        {
            uint8_t cBitShift = (cMaskedChannel) & 0x7;
            cRegValue |= (1 << cBitShift);
            std::string cChType = ((+cMaskedChannel & 0x1) == 0) ? "seed" : "correlation";
            std::string cOut    = "Channel " + std::to_string((int)cMaskedChannel) + " in the " + cChType.c_str() + " layer\t";
            cOutput += cOut.c_str();
        }
        // Channels for stub sweep : " << cOutput.c_str() << RESET ;
        fReadoutChipInterface->WriteChipReg(cChip, cMasked.first, cRegValue);
    }
    // Fabio: CBC specific but not used by common scans - END
}

uint16_t Tool::getMaxNumberOfGroups()
{
    uint16_t maxNumberOfGroups = 0;
    for(const auto board: *fDetectorContainer)
    {
        for(const auto opticalGroup: *board)
        {
            for(const auto hybrid: *opticalGroup)
            {
                for(const auto chip: *hybrid)
                {
                    uint16_t numberOfGroups = getChannelGroupHandlerContainer()
                                                  ->getObject(board->getId())
                                                  ->getObject(opticalGroup->getId())
                                                  ->getObject(hybrid->getId())
                                                  ->getObject(chip->getId())
                                                  ->getSummary<std::shared_ptr<ChannelGroupHandler>>()
                                                  ->getNumberOfGroups();
                    if(numberOfGroups > maxNumberOfGroups) maxNumberOfGroups = numberOfGroups;
                }
            }
        }
    }

    return maxNumberOfGroups;
}

// Two dimensional dac scan
void Tool::scanDacDac(const std::string&                               dac1Name,
                      const std::vector<uint16_t>&                     dac1List,
                      const std::string&                               dac2Name,
                      const std::vector<uint16_t>&                     dac2List,
                      uint32_t                                         numberOfEvents,
                      std::vector<std::vector<DetectorDataContainer*>> detectorContainerVectorOfVector,
                      int32_t                                          numberOfEventsPerBurst)
{
    for(auto board: *fDetectorContainer) { scanBeBoardDacDac(board->getId(), dac1Name, dac1List, dac2Name, dac2List, numberOfEvents, detectorContainerVectorOfVector, numberOfEventsPerBurst); }

    return;
}

// Two dimensional dac scan per BeBoard
void Tool::scanBeBoardDacDac(uint16_t                                         boardId,
                             const std::string&                               dac1Name,
                             const std::vector<uint16_t>&                     dac1List,
                             const std::string&                               dac2Name,
                             const std::vector<uint16_t>&                     dac2List,
                             uint32_t                                         numberOfEvents,
                             std::vector<std::vector<DetectorDataContainer*>> detectorContainerVectorOfVector,
                             int32_t                                          numberOfEventsPerBurst)
{
    if(dac1List.size() != detectorContainerVectorOfVector.size())
    {
        LOG(ERROR) << __PRETTY_FUNCTION__ << " dacList and detector container vector have different sizes, aborting";
        abort();
    }

    for(size_t dacIt = 0; dacIt < dac1List.size(); ++dacIt)
    {
        if(boardId == 0) LOG(INFO) << BOLDBLUE << " Scanning dac1 " << dac1Name << ", value = " << dac1List.at(dacIt) << " vs " << dac2Name << RESET;
        setSameDacBeBoard(fDetectorContainer->getObject(boardId), dac1Name, dac1List.at(dacIt));
        scanBeBoardDac(boardId, dac2Name, dac2List, numberOfEvents, detectorContainerVectorOfVector.at(dacIt), numberOfEventsPerBurst);
    }
}

// One dimensional dac scan
void Tool::scanDac(const std::string&                  dacName,
                   const std::vector<uint16_t>&        dacList,
                   uint32_t                            numberOfEvents,
                   std::vector<DetectorDataContainer*> detectorContainerVector,
                   int32_t                             numberOfEventsPerBurst)
{
    for(auto board: *fDetectorContainer) { scanBeBoardDac(board->getId(), dacName, dacList, numberOfEvents, detectorContainerVector, numberOfEventsPerBurst); }
}

// bit wise scan
void Tool::bitWiseScan(const std::string& dacName, uint32_t numberOfEvents, const float& targetOccupancy, int32_t numberOfEventsPerBurst)
{
    for(auto board: *fDetectorContainer) { bitWiseScanBeBoard(board->getId(), dacName, numberOfEvents, targetOccupancy, numberOfEventsPerBurst); }
}

// bit wise scan per BeBoard
void Tool::bitWiseScanBeBoard(uint16_t boardId, const std::string& dacName, uint32_t numberOfEvents, const float& targetOccupancy, int32_t numberOfEventsPerBurst)
{
    DetectorDataContainer* outputDataContainer = fDetectorDataContainer;
    ReadoutChip*           cReadoutChip        = fDetectorContainer->getObject(boardId)->getFirstObject()->getFirstObject()->getFirstObject(); // assumption: one BeBoard has only one type of chip;
    bool                   localDAC            = cReadoutChip->isDACLocal(dacName);
    uint8_t                numberOfBits        = cReadoutChip->getNumberOfBits(dacName);
    LOG(DEBUG) << BOLDBLUE << "Number of bits in this DAC is " << +numberOfBits << RESET;
    bool                   occupanyDirectlyProportionalToDAC;
    DetectorDataContainer* previousStepOccupancyContainer = new DetectorDataContainer();
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, *previousStepOccupancyContainer);
    DetectorDataContainer* currentStepOccupancyContainer = new DetectorDataContainer();
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, *currentStepOccupancyContainer);

    DetectorDataContainer* previousDacList = new DetectorDataContainer();
    DetectorDataContainer* currentDacList  = new DetectorDataContainer();

    uint16_t allZeroRegister = 0;
    uint16_t allOneRegister  = (0xFFFF >> (16 - numberOfBits));
    if(localDAC)
    {
        ContainerFactory::copyAndInitChannel<uint16_t>(*fDetectorContainer, *previousDacList, allZeroRegister);
        ContainerFactory::copyAndInitChannel<uint16_t>(*fDetectorContainer, *currentDacList, allOneRegister);
    }
    else
    {
        ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, *previousDacList, allZeroRegister);
        ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, *currentDacList, allOneRegister);
    }
    LOG(DEBUG) << BOLDBLUE << "Setting all bits of register " << dacName << "  to  " << +allZeroRegister << RESET;
    if(localDAC)
        setAllLocalDacBeBoard(boardId, dacName, *previousDacList);
    else
        setAllGlobalDacBeBoard(boardId, dacName, *previousDacList);

    fDetectorDataContainer = previousStepOccupancyContainer;
    LOG(DEBUG) << BOLDBLUE << "\t\t... measuring occupancy...." << RESET;
    measureBeBoardData(boardId, numberOfEvents, numberOfEventsPerBurst);

    LOG(DEBUG) << BOLDBLUE << "Setting all bits of register " << dacName << "  to  " << +allOneRegister << RESET;
    if(localDAC)
        setAllLocalDacBeBoard(boardId, dacName, *currentDacList);
    else
        setAllGlobalDacBeBoard(boardId, dacName, *currentDacList);

    fDetectorDataContainer = currentStepOccupancyContainer;
    LOG(DEBUG) << BOLDBLUE << "\t\t... measuring occupancy...." << RESET;
    measureBeBoardData(boardId, numberOfEvents, numberOfEventsPerBurst);

    // This fails sometimes depending on settings, need to make it an option...
    occupanyDirectlyProportionalToDAC = currentStepOccupancyContainer->getObject(boardId)->getSummary<Occupancy, Occupancy>().fOccupancy >
                                        previousStepOccupancyContainer->getObject(boardId)->getSummary<Occupancy, Occupancy>().fOccupancy;

    // Hacked solution for PS
    if((cReadoutChip->getFrontEndType() == FrontEndType::MPA2) or (cReadoutChip->getFrontEndType() == FrontEndType::SSA2))
    {
        if(localDAC)
            occupanyDirectlyProportionalToDAC = true;
        else
            occupanyDirectlyProportionalToDAC = false;
    }

    if(!occupanyDirectlyProportionalToDAC)
    {
        DetectorDataContainer* tmpPointer = previousDacList;
        previousDacList                   = currentDacList;
        currentDacList                    = tmpPointer;
    }
    // LOG (INFO) << BOLDBLUE << "START " << RESET;

    for(int iBit = numberOfBits - 1; iBit >= 0; --iBit)
    {
        LOG(DEBUG) << BOLDBLUE << "Bit number " << +iBit << " of " << dacName << RESET;
        for(auto cOpticalGroup: *(fDetectorContainer->getObject(boardId)))
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(localDAC)
                    {
                        for(uint16_t row = 0; row < cChip->getNumberOfRows(); ++row)
                        {
                            for(uint16_t col = 0; col < cChip->getNumberOfCols(); ++col)
                            {
                                if(occupanyDirectlyProportionalToDAC)
                                    currentDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getChannel<uint16_t>(row, col) =
                                        previousDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getChannel<uint16_t>(row, col) +
                                        (1 << iBit);
                                else
                                    currentDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getChannel<uint16_t>(row, col) =
                                        previousDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getChannel<uint16_t>(row, col) &
                                        (0xFFFF - (1 << iBit));
                            }
                        }
                    }
                    else
                    {
                        if(occupanyDirectlyProportionalToDAC)
                            currentDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                previousDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() + (1 << iBit);
                        else
                            currentDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                previousDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() &
                                (0xFFFF - (1 << iBit));
                    }
                }
            }
        }

        if(localDAC)
            setAllLocalDacBeBoard(boardId, dacName, *currentDacList);
        else
            setAllGlobalDacBeBoard(boardId, dacName, *currentDacList);

        Occupancy noOccupancy;
        ContainerFactory::reinitializeContainer(*currentStepOccupancyContainer, noOccupancy);
        fDetectorDataContainer = currentStepOccupancyContainer;
        measureBeBoardData(boardId, numberOfEvents, numberOfEventsPerBurst);
        // TO-DO.. generalize so that I don't need the MPA/SSA
        if(fNormalize == 0)
        {
            float cMaxOcc               = 1.0;
            auto& cDataContainerThisBrd = fDetectorDataContainer->getObject(boardId);
            for(auto cOpticalGroup: *(fDetectorContainer->getObject(boardId)))
            {
                auto& cDataContainerThisOG = cDataContainerThisBrd->getObject(cOpticalGroup->getId());
                for(auto cHybrid: *cOpticalGroup)
                {
                    auto& cDataContainerThisFE = cDataContainerThisOG->getObject(cHybrid->getId());
                    for(auto cChip: *cHybrid)
                    {
                        auto&                cDataContainerThisChip = cDataContainerThisFE->getObject(cChip->getId());
                        auto&                cSummary               = cDataContainerThisChip->getSummary<Occupancy, Occupancy>();
                        ChannelGroupHandler* cHandler;
                        if(cChip->getFrontEndType() == FrontEndType::MPA2)
                            cHandler = new MPAChannelGroupHandler();
                        else
                            cHandler = new SSAChannelGroupHandler();
                        // LOG(DEBUG) << BOLDYELLOW << " Normalizing assuming " << fNReadbackEvents << " events and " << cHandler->allChannelGroup()->getNumberOfEnabledChannels() << " enabled
                        // channels."
                        //            << RESET;
                        float  cGlobalOcc = 0;
                        size_t cNenabled  = 0;
                        for(uint16_t cChnl = 0; cChnl < cDataContainerThisChip->size(); cChnl++)
                        {
                            uint32_t cRow = cChnl % cHandler->allChannelGroup()->getNumberOfRows();
                            uint32_t cCol;
                            if(cHandler->allChannelGroup()->getNumberOfCols() == 0)
                                cCol = 0;
                            else
                                cCol = cChnl / cHandler->allChannelGroup()->getNumberOfRows();
                            if(cHandler->allChannelGroup()->isChannelEnabled(cRow, cCol))
                            {
                                cDataContainerThisChip->getChannel<Occupancy>(cRow, cCol).fOccupancy /= fNReadbackEvents;
                                cGlobalOcc += cDataContainerThisChip->getChannel<Occupancy>(cRow, cCol).fOccupancy;
                                cNenabled++;
                                // if(cChnl < 10 || cChnl > 15 * 120 + 110)
                                //     LOG(DEBUG) << BOLDBLUE << cChnl << " [ " << cRow << " , " << cCol << " ] " << cDataContainerThisChip->getChannel<Occupancy>(cRow, cCol).fOccupancy << RESET;
                            }
                        }
                        cGlobalOcc /= cNenabled;
                        cSummary.fOccupancy = std::min(cMaxOcc, cGlobalOcc);
                    }
                }
            }
        }

        // Determine if it is better or not
        for(auto cOpticalGroup: *(fDetectorContainer->getObject(boardId)))
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    std::stringstream cOut;
                    if(localDAC)
                    {
                        for(uint16_t row = 0; row < cChip->getNumberOfRows(); ++row)
                        {
                            for(uint16_t col = 0; col < cChip->getNumberOfCols(); ++col)
                            {
                                cOut << BOLDBLUE << "localocc "
                                     << currentStepOccupancyContainer->getObject(boardId)
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getChannel<Occupancy>(row, col)
                                            .fOccupancy
                                     << "\n";

                                if(currentStepOccupancyContainer->getObject(boardId)
                                       ->getObject(cOpticalGroup->getId())
                                       ->getObject(cHybrid->getId())
                                       ->getObject(cChip->getId())
                                       ->getChannel<Occupancy>(row, col)
                                       .fOccupancy <= targetOccupancy)
                                {
                                    previousDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getChannel<uint16_t>(row, col) =
                                        currentDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getChannel<uint16_t>(row, col);
                                    previousStepOccupancyContainer->getObject(boardId)
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<Occupancy>(row, col)
                                        .fOccupancy = currentStepOccupancyContainer->getObject(boardId)
                                                          ->getObject(cOpticalGroup->getId())
                                                          ->getObject(cHybrid->getId())
                                                          ->getObject(cChip->getId())
                                                          ->getChannel<Occupancy>(row, col)
                                                          .fOccupancy;
                                }
                            }
                        }
                    }
                    else
                    {
                        auto& cCurrentDAC = currentDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();
                        cOut << "Occupancy Chip#" << +cChip->getId() << "\t[ global] "
                             << currentStepOccupancyContainer->getObject(boardId)
                                    ->getObject(cOpticalGroup->getId())
                                    ->getObject(cHybrid->getId())
                                    ->getObject(cChip->getId())
                                    ->getSummary<Occupancy, Occupancy>()
                                    .fOccupancy
                             << " for a DAC value of " << cCurrentDAC;

                        if(currentStepOccupancyContainer->getObject(boardId)
                               ->getObject(cOpticalGroup->getId())
                               ->getObject(cHybrid->getId())
                               ->getObject(cChip->getId())
                               ->getSummary<Occupancy, Occupancy>()
                               .fOccupancy <= targetOccupancy)
                        {
                            previousDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                                currentDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();
                            previousStepOccupancyContainer->getObject(boardId)
                                ->getObject(cOpticalGroup->getId())
                                ->getObject(cHybrid->getId())
                                ->getObject(cChip->getId())
                                ->getSummary<Occupancy, Occupancy>()
                                .fOccupancy = currentStepOccupancyContainer->getObject(boardId)
                                                  ->getObject(cOpticalGroup->getId())
                                                  ->getObject(cHybrid->getId())
                                                  ->getObject(cChip->getId())
                                                  ->getSummary<Occupancy, Occupancy>()
                                                  .fOccupancy;
                        }
                    }
                    // LOG(DEBUG) << BOLDYELLOW << cOut.str() << RESET;
                }
            }
        }
    }

    if(localDAC) { setAllLocalDacBeBoard(boardId, dacName, *previousDacList); }
    else
        setAllGlobalDacBeBoard(boardId, dacName, *previousDacList);

    fDetectorDataContainer = outputDataContainer;
    measureBeBoardData(boardId, numberOfEvents, numberOfEventsPerBurst);

    delete previousStepOccupancyContainer;
    delete currentStepOccupancyContainer;
    delete previousDacList;
    delete currentDacList;

    return;
}

// full scan, eed a way to traport
void Tool::fullScan(const std::string& dacName, uint32_t numberOfEvents, const float& targetOccupancy, int32_t numberOfEventsPerBurst, int32_t startVal, bool mask)
{
    for(auto board: *fDetectorContainer) { fullScanBeBoard(board->getId(), dacName, numberOfEvents, targetOccupancy, numberOfEventsPerBurst, startVal, mask); }
}

// full scan per BeBoard. Returns untrimmed objects list (channels/chips)
void Tool::fullScanBeBoard(uint16_t boardId, const std::string& dacName, uint32_t numberOfEvents, const float& targetOccupancy, int32_t numberOfEventsPerBurst, int32_t startVal, bool mask)
{
    DetectorDataContainer* outputDataContainer = fDetectorDataContainer;
    ReadoutChip*           cReadoutChip        = fDetectorContainer->getObject(boardId)->getFirstObject()->getFirstObject()->getFirstObject(); // assumption: one BeBoard has only one type of chip;
    bool                   localDAC            = cReadoutChip->isDACLocal(dacName);
    DetectorDataContainer* previousStepOccupancyContainer = new DetectorDataContainer();
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, *previousStepOccupancyContainer);
    DetectorDataContainer* currentStepOccupancyContainer = new DetectorDataContainer();
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, *currentStepOccupancyContainer);

    DetectorDataContainer* currentDacList  = new DetectorDataContainer();
    DetectorDataContainer* currentDoneList = new DetectorDataContainer();
    DetectorDataContainer* currentMaskList = new DetectorDataContainer();

    std::vector<std::pair<uint16_t, uint16_t>> returnVec;

    // For masking so we dont exclude the outliers.  only works if all chips are identical -- which is the current use case.
    std::pair<int, int>     NoccDiff(0, 0);
    std::pair<float, float> occDiff(0.0, 0.0);

    uint16_t allOneRegister = startVal;

    uint16_t                                   allZeroRegister  = 0;
    uint16_t                                   allFalseRegister = 0;
    std::vector<std::pair<uint16_t, uint16_t>> maskvec;
    if(localDAC)
    {
        ContainerFactory::copyAndInitChannel<uint16_t>(*fDetectorContainer, *currentDacList, allOneRegister);
        ContainerFactory::copyAndInitChannel<uint16_t>(*fDetectorContainer, *currentDoneList, allFalseRegister);
        ContainerFactory::copyAndInitChip<std::vector<std::pair<uint16_t, uint16_t>>>(*fDetectorContainer, *currentMaskList, maskvec);
    }
    else
    {
        ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, *currentDacList, allOneRegister);
        ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, *currentDoneList, allFalseRegister);
    }

    for(int iThresh = startVal; iThresh >= allZeroRegister; iThresh--)
    {
        bool first = (iThresh == startVal);

        int threshToSet = iThresh;
        if(localDAC) threshToSet = 31 - iThresh;

        returnVec.clear();

        LOG(INFO) << BOLDBLUE << "Threshold set to " << int(threshToSet) << " for " << dacName << RESET;
        for(auto cOpticalGroup: *(fDetectorContainer->getObject(boardId)))
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(localDAC)
                    {
                        currentMaskList->getObject(boardId)
                            ->getObject(cOpticalGroup->getId())
                            ->getObject(cHybrid->getId())
                            ->getObject(cChip->getId())
                            ->getSummary<std::vector<std::pair<uint16_t, uint16_t>>>()
                            .clear();
                        for(uint16_t row = 0; row < cChip->getNumberOfRows(); ++row)
                        {
                            for(uint16_t col = 0; col < cChip->getNumberOfCols(); ++col)
                            {
                                if(not currentDoneList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getChannel<uint16_t>(row, col))
                                {
                                    currentDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getChannel<uint16_t>(row, col) =
                                        threshToSet;
                                }
                            }
                        }
                    }
                    else
                    {
                        if(not currentDoneList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>())
                        {
                            currentDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() = threshToSet;
                            // LOG(DEBUG) << BOLDBLUE << "\t.. current setting is "
                            //            << currentDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>()
                            //            << RESET;
                        }
                    }
                }
            }
        }

        if(localDAC)
            setAllLocalDacBeBoard(boardId, dacName, *currentDacList);
        else
            setAllGlobalDacBeBoard(boardId, dacName, *currentDacList);

        Occupancy noOccupancy;
        ContainerFactory::reinitializeContainer(*currentStepOccupancyContainer, noOccupancy);
        fDetectorDataContainer = currentStepOccupancyContainer;
        measureBeBoardData(boardId, numberOfEvents, numberOfEventsPerBurst);

        // Determine if it is better or not
        for(auto cOpticalGroup: *(fDetectorContainer->getObject(boardId)))
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    std::stringstream cOut;
                    if(localDAC)
                    {
                        for(uint16_t row = 0; row < cChip->getNumberOfRows(); ++row)
                        {
                            for(uint16_t col = 0; col < cChip->getNumberOfCols(); ++col)
                            {
                                cOut << BOLDBLUE << "localocc "
                                     << currentStepOccupancyContainer->getObject(boardId)
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getChannel<Occupancy>(row, col)
                                            .fOccupancy
                                     << "\n";
                                if(not currentDoneList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getChannel<uint16_t>(row, col))
                                {
                                    // if (cChip->size()<500 and iChannel<10)std::cout<<"PRE occDiff
                                    // "<<iChannel<<":"<<currentStepOccupancyContainer->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getChannel<Occupancy>(row,
                                    // col).fOccupancy<<std::endl;
                                    currentStepOccupancyContainer->getObject(boardId)
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<Occupancy>(row, col)
                                        .fOccupancy = std::max(currentStepOccupancyContainer->getObject(boardId)
                                                                   ->getObject(cOpticalGroup->getId())
                                                                   ->getObject(cHybrid->getId())
                                                                   ->getObject(cChip->getId())
                                                                   ->getChannel<Occupancy>(row, col)
                                                                   .fOccupancy,
                                                               previousStepOccupancyContainer->getObject(boardId)
                                                                   ->getObject(cOpticalGroup->getId())
                                                                   ->getObject(cHybrid->getId())
                                                                   ->getObject(cChip->getId())
                                                                   ->getChannel<Occupancy>(row, col)
                                                                   .fOccupancy);
                                    if((currentStepOccupancyContainer->getObject(boardId)
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getChannel<Occupancy>(row, col)
                                            .fOccupancy >= targetOccupancy) and
                                       (previousStepOccupancyContainer->getObject(boardId)
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getChannel<Occupancy>(row, col)
                                            .fOccupancy < targetOccupancy) and
                                       (not first))
                                    {
                                        if(std::fabs(currentStepOccupancyContainer->getObject(boardId)
                                                         ->getObject(cOpticalGroup->getId())
                                                         ->getObject(cHybrid->getId())
                                                         ->getObject(cChip->getId())
                                                         ->getChannel<Occupancy>(row, col)
                                                         .fOccupancy -
                                                     targetOccupancy) > std::fabs(previousStepOccupancyContainer->getObject(boardId)
                                                                                      ->getObject(cOpticalGroup->getId())
                                                                                      ->getObject(cHybrid->getId())
                                                                                      ->getObject(cChip->getId())
                                                                                      ->getChannel<Occupancy>(row, col)
                                                                                      .fOccupancy -
                                                                                  targetOccupancy))
                                        {
                                            currentDacList->getObject(boardId)
                                                ->getObject(cOpticalGroup->getId())
                                                ->getObject(cHybrid->getId())
                                                ->getObject(cChip->getId())
                                                ->getChannel<uint16_t>(row, col) -= 1;
                                            occDiff.first += previousStepOccupancyContainer->getObject(boardId)
                                                                 ->getObject(cOpticalGroup->getId())
                                                                 ->getObject(cHybrid->getId())
                                                                 ->getObject(cChip->getId())
                                                                 ->getChannel<Occupancy>(row, col)
                                                                 .fOccupancy -
                                                             targetOccupancy;

                                            NoccDiff.first += 1;
                                        }
                                        else
                                        {
                                            occDiff.second += currentStepOccupancyContainer->getObject(boardId)
                                                                  ->getObject(cOpticalGroup->getId())
                                                                  ->getObject(cHybrid->getId())
                                                                  ->getObject(cChip->getId())
                                                                  ->getChannel<Occupancy>(row, col)
                                                                  .fOccupancy -
                                                              targetOccupancy;
                                            NoccDiff.second += 1;
                                        }

                                        currentDoneList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getChannel<uint16_t>(row, col) =
                                            1;
                                    }

                                    else
                                    {
                                        currentMaskList->getObject(boardId)
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getSummary<std::vector<std::pair<uint16_t, uint16_t>>>()
                                            .push_back({row, col});

                                        returnVec.push_back({row, col});
                                        previousStepOccupancyContainer->getObject(boardId)
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getChannel<Occupancy>(row, col)
                                            .fOccupancy = currentStepOccupancyContainer->getObject(boardId)
                                                              ->getObject(cOpticalGroup->getId())
                                                              ->getObject(cHybrid->getId())
                                                              ->getObject(cChip->getId())
                                                              ->getChannel<Occupancy>(row, col)
                                                              .fOccupancy;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        auto& cCurrentDAC = currentDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>();
                        cOut << "Occupancy Chip#" << +cChip->getId() << "\t[ global] "
                             << currentStepOccupancyContainer->getObject(boardId)
                                    ->getObject(cOpticalGroup->getId())
                                    ->getObject(cHybrid->getId())
                                    ->getObject(cChip->getId())
                                    ->getSummary<Occupancy, Occupancy>()
                                    .fOccupancy
                             << " for a DAC value of " << cCurrentDAC;

                        float chanavg = 0.0;
                        for(uint16_t row = 0; row < cChip->getNumberOfRows(); ++row)
                        {
                            for(uint16_t col = 0; col < cChip->getNumberOfCols(); ++col)
                            {
                                currentStepOccupancyContainer->getObject(boardId)
                                    ->getObject(cOpticalGroup->getId())
                                    ->getObject(cHybrid->getId())
                                    ->getObject(cChip->getId())
                                    ->getChannel<Occupancy>(row, col)
                                    .fOccupancy = std::max(currentStepOccupancyContainer->getObject(boardId)
                                                               ->getObject(cOpticalGroup->getId())
                                                               ->getObject(cHybrid->getId())
                                                               ->getObject(cChip->getId())
                                                               ->getChannel<Occupancy>(row, col)
                                                               .fOccupancy,
                                                           previousStepOccupancyContainer->getObject(boardId)
                                                               ->getObject(cOpticalGroup->getId())
                                                               ->getObject(cHybrid->getId())
                                                               ->getObject(cChip->getId())
                                                               ->getChannel<Occupancy>(row, col)
                                                               .fOccupancy);

                                chanavg += float(currentStepOccupancyContainer->getObject(boardId)
                                                     ->getObject(cOpticalGroup->getId())
                                                     ->getObject(cHybrid->getId())
                                                     ->getObject(cChip->getId())
                                                     ->getChannel<Occupancy>(row, col)
                                                     .fOccupancy >= targetOccupancy); // counts number found instead
                            }
                        }
                        chanavg /= float(cChip->size());
                        currentStepOccupancyContainer->getObject(boardId)
                            ->getObject(cOpticalGroup->getId())
                            ->getObject(cHybrid->getId())
                            ->getObject(cChip->getId())
                            ->getSummary<Occupancy, Occupancy>()
                            .fOccupancy = chanavg;

                        if(not currentDoneList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>())
                        {
                            if(currentStepOccupancyContainer->getObject(boardId)
                                       ->getObject(cOpticalGroup->getId())
                                       ->getObject(cHybrid->getId())
                                       ->getObject(cChip->getId())
                                       ->getSummary<Occupancy, Occupancy>()
                                       .fOccupancy >= 0.5 and
                               previousStepOccupancyContainer->getObject(boardId)
                                       ->getObject(cOpticalGroup->getId())
                                       ->getObject(cHybrid->getId())
                                       ->getObject(cChip->getId())
                                       ->getSummary<Occupancy, Occupancy>()
                                       .fOccupancy < 0.5)
                            {
                                if(std::fabs(currentStepOccupancyContainer->getObject(boardId)
                                                 ->getObject(cOpticalGroup->getId())
                                                 ->getObject(cHybrid->getId())
                                                 ->getObject(cChip->getId())
                                                 ->getSummary<Occupancy, Occupancy>()
                                                 .fOccupancy -
                                             0.5) > std::fabs(previousStepOccupancyContainer->getObject(boardId)
                                                                  ->getObject(cOpticalGroup->getId())
                                                                  ->getObject(cHybrid->getId())
                                                                  ->getObject(cChip->getId())
                                                                  ->getSummary<Occupancy, Occupancy>()
                                                                  .fOccupancy -
                                                              0.5))
                                {
                                    currentDacList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() += 1;
                                }
                                currentDoneList->getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() = 1;
                            }
                            else
                            {
                                previousStepOccupancyContainer->getObject(boardId)
                                    ->getObject(cOpticalGroup->getId())
                                    ->getObject(cHybrid->getId())
                                    ->getObject(cChip->getId())
                                    ->getSummary<Occupancy, Occupancy>()
                                    .fOccupancy = currentStepOccupancyContainer->getObject(boardId)
                                                      ->getObject(cOpticalGroup->getId())
                                                      ->getObject(cHybrid->getId())
                                                      ->getObject(cChip->getId())
                                                      ->getSummary<Occupancy, Occupancy>()
                                                      .fOccupancy;

                                for(uint16_t row = 0; row < cChip->getNumberOfRows(); ++row)
                                {
                                    for(uint16_t col = 0; col < cChip->getNumberOfCols(); ++col)
                                    {
                                        returnVec.push_back({row, col});
                                        previousStepOccupancyContainer->getObject(boardId)
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getChannel<Occupancy>(row, col)
                                            .fOccupancy = currentStepOccupancyContainer->getObject(boardId)
                                                              ->getObject(cOpticalGroup->getId())
                                                              ->getObject(cHybrid->getId())
                                                              ->getObject(cChip->getId())
                                                              ->getChannel<Occupancy>(row, col)
                                                              .fOccupancy;
                                    }
                                }
                            }
                        }
                    }
                    // LOG(DEBUG) << BOLDYELLOW << cOut.str() << RESET;
                }
            }
        }
        size_t nRunning = returnVec.size();
        LOG(INFO) << BOLDYELLOW << "Pedestals remaining: " << nRunning << RESET;
        if(nRunning == 0) break;
    }
    fDetectorDataContainer = outputDataContainer;
    measureBeBoardData(boardId, numberOfEvents, numberOfEventsPerBurst);
    if(localDAC)
    {
        occDiff.first  = (occDiff.first) / float(NoccDiff.first);
        occDiff.second = (occDiff.second) / float(NoccDiff.second);

        // Turn off for now -- todo
        occDiff.first  = 0;
        occDiff.second = 0;
        // Turn off for now -- todo

        // LOG(INFO) << BOLDYELLOW << "maxDiff:  " <<maxDiff<<RESET;

        for(auto cOpticalGroup: *(fDetectorContainer->getObject(boardId)))
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    auto cOriginalMask = cChip->getChipOriginalMask();
                    if(localDAC)
                    {
                        std::vector<std::pair<uint16_t, uint16_t>> tempMaskList = currentMaskList->getObject(boardId)
                                                                                      ->getObject(cOpticalGroup->getId())
                                                                                      ->getObject(cHybrid->getId())
                                                                                      ->getObject(cChip->getId())
                                                                                      ->getSummary<std::vector<std::pair<uint16_t, uint16_t>>>();

                        for(const auto& maskedChannel: tempMaskList)
                        {
                            // LOG(INFO) << BOLDYELLOW << "occDiff.first  "<< occDiff.first << " occDiff.second  "<< occDiff.second  <<RESET;
                            // LOG(INFO) << BOLDYELLOW << "SUM.first  "<< occDiff.first+targetOccupancy << " SUM.second  "<< occDiff.second +targetOccupancy <<RESET;

                            if(currentStepOccupancyContainer->getObject(boardId)
                                   ->getObject(cOpticalGroup->getId())
                                   ->getObject(cHybrid->getId())
                                   ->getObject(cChip->getId())
                                   ->getChannel<Occupancy>(maskedChannel.first, maskedChannel.second)
                                   .fOccupancy > targetOccupancy)
                            {
                                currentDacList->getObject(boardId)
                                    ->getObject(cOpticalGroup->getId())
                                    ->getObject(cHybrid->getId())
                                    ->getObject(cChip->getId())
                                    ->getChannel<uint16_t>(maskedChannel.first, maskedChannel.second) = 0;

                                if(mask)
                                {
                                    if((currentStepOccupancyContainer->getObject(boardId)
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getChannel<Occupancy>(maskedChannel.first, maskedChannel.second)
                                            .fOccupancy) > (targetOccupancy + 2.0 * occDiff.second))
                                    {
                                        cOriginalMask->disableChannel(maskedChannel.first, maskedChannel.second);
                                        LOG(INFO) << BOLDRED << "Masking Channel row " << maskedChannel.first << " col " << maskedChannel.second << RESET;
                                        // LOG(INFO) << BOLDYELLOW << "MASKHIGH"<<RESET;
                                    }
                                }
                            }
                            else
                            {
                                currentDacList->getObject(boardId)
                                    ->getObject(cOpticalGroup->getId())
                                    ->getObject(cHybrid->getId())
                                    ->getObject(cChip->getId())
                                    ->getChannel<uint16_t>(maskedChannel.first, maskedChannel.second) = 31;
                                if(mask)
                                {
                                    if((currentStepOccupancyContainer->getObject(boardId)
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getChannel<Occupancy>(maskedChannel.first, maskedChannel.second)
                                            .fOccupancy) < (targetOccupancy + 2.0 * occDiff.first))
                                    {
                                        cOriginalMask->disableChannel(maskedChannel.first, maskedChannel.second);
                                        LOG(INFO) << BOLDRED << "Masking Channel row " << maskedChannel.first << " col " << maskedChannel.second << RESET;
                                        // LOG(INFO) << BOLDYELLOW << "MASKLOW"<<RESET;
                                    }
                                }
                            }
                        }
                    }
                    // else -> probably wont be masking chips
                    // fReadoutChipInterface->ConfigureChipOriginalMask(cChip);
                    fReadoutChipInterface->maskChannelGroup(cChip, cOriginalMask);
                }
            }
        }
    }
    if(localDAC)
        setAllLocalDacBeBoard(boardId, dacName, *currentDacList);
    else
        setAllGlobalDacBeBoard(boardId, dacName, *currentDacList);

    delete previousStepOccupancyContainer;
    delete currentStepOccupancyContainer;
    delete currentDacList;
}

// set dac and measure occupancy
void Tool::setDacAndMeasureData(const std::string& dacName, const uint16_t dacValue, uint32_t numberOfEvents, int32_t numberOfEventsPerBurst)
{
    for(auto board: *fDetectorContainer) { setDacAndMeasureBeBoardData(board->getId(), dacName, dacValue, numberOfEvents, numberOfEventsPerBurst); }
}

// Set dac and measure occupancy per BeBoard
void Tool::setDacAndMeasureBeBoardData(uint16_t boardId, const std::string& dacName, const uint16_t dacValue, uint32_t numberOfEvents, int32_t numberOfEventsPerBurst)
{
    setSameDacBeBoard(fDetectorContainer->getObject(boardId), dacName, dacValue);
    measureBeBoardData(boardId, numberOfEvents, numberOfEventsPerBurst);
}

// Measure occupancy
void Tool::measureData(uint32_t numberOfEvents, int32_t numberOfEventsPerBurst)
{
    for(auto board: *fDetectorContainer) measureBeBoardData(board->getId(), numberOfEvents, numberOfEventsPerBurst);
}

class ScanBase
{
  public:
    ScanBase(Tool* theTool) : fTool(theTool) { ; }
    virtual ~ScanBase() { ; }

    virtual void operator()() = 0;
    void         setGroupHandlerContainer(const DetectorDataContainer* theChannelHandlerContainer, bool isSameChannelGroupForAllChannels)
    {
        fChannelHandlerContainer        = theChannelHandlerContainer;
        fSameChannelGroupForAllChannels = isSameChannelGroupForAllChannels;
    }
    void setGroup(int groupNumber) { fGroupNumber = groupNumber; }
    // void         setGroup(const ChannelGroupBase* cTestChannelGroup) { fTestChannelGroup = cTestChannelGroup; }
    void setBoardId(uint16_t boardId) { fBoardId = boardId; }
    void setNumberOfEvents(uint32_t numberOfEvents) { fNumberOfEvents = numberOfEvents; }
    void setNumberOfEventsPerBurst(int32_t numberOfEventsPerBurst) { fNumberOfEventsPerBurst = numberOfEventsPerBurst; }

    void setDetectorContainer(DetectorContainer* detectorContainer) { fDetectorContainer = detectorContainer; }

  protected:
    uint32_t                     fNumberOfEvents;
    int32_t                      fNumberOfEventsPerBurst{-1};
    uint32_t                     fNumberOfMSec;
    uint32_t                     fBoardId;
    const DetectorDataContainer* fChannelHandlerContainer;
    int                          fGroupNumber;
    Tool*                        fTool;
    DetectorContainer*           fDetectorContainer;
    bool                         fSameChannelGroupForAllChannels;
};

void Tool::doScanOnAllGroupsBeBoard(uint16_t boardId, uint32_t numberOfEvents, int32_t numberOfEventsPerBurst, ScanBase* groupScan)
{
    groupScan->setBoardId(boardId);
    groupScan->setNumberOfEvents(numberOfEvents);
    groupScan->setDetectorContainer(fDetectorContainer);
    groupScan->setNumberOfEventsPerBurst(numberOfEventsPerBurst);
    groupScan->setGroupHandlerContainer(getChannelGroupHandlerContainer(), fSameChannelGroupForAllChannels);
    if(!fAllChan)
    {
        uint16_t maxNumberOfGroups = getMaxNumberOfGroups();
        for(uint16_t groupNumber = 0; groupNumber < maxNumberOfGroups; ++groupNumber)
        {
            if(fMaskChannelsFromOtherGroups || fTestPulse)
            {
                for(auto cOpticalGroup: *(fDetectorContainer->getObject(boardId)))
                {
                    for(auto cHybrid: *cOpticalGroup)
                    {
                        for(auto cChip: *cHybrid)
                        {
                            auto channelGroup = getChannelGroup(groupNumber, boardId, cOpticalGroup->getId(), cHybrid->getId(), cChip->getId());
                            if(!channelGroup) continue;

                            fReadoutChipInterface->maskChannelsAndSetInjectionSchema(cChip, channelGroup, fMaskChannelsFromOtherGroups, fTestPulse);
                        }
                    }
                }
            }
            groupScan->setGroup(groupNumber);
            (*groupScan)();
        }

        if(fMaskChannelsFromOtherGroups) // Re-enable all the channels and evaluate
        {
            for(auto cOpticalGroup: *(fDetectorContainer->getObject(boardId)))
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid) { fReadoutChipInterface->ConfigureChipOriginalMask(cChip); }
                }
            }
        }
    }
    else
    {
        groupScan->setGroup(-1);
        (*groupScan)();
    }
}

class MeasureBeBoardDataPerGroup : public ScanBase
{
  public:
    MeasureBeBoardDataPerGroup(Tool* theTool) : ScanBase(theTool) { ; }
    ~MeasureBeBoardDataPerGroup() { ; }

    void operator()() override
    {
        uint32_t burstNumbers;
        uint32_t lastBurstNumberOfEvents;
        if(fNumberOfEventsPerBurst <= 0)
        {
            burstNumbers            = 1;
            lastBurstNumberOfEvents = fNumberOfEvents;
        }
        else
        {
            burstNumbers            = fNumberOfEvents / fNumberOfEventsPerBurst;
            lastBurstNumberOfEvents = fNumberOfEventsPerBurst;
            if(fNumberOfEvents % fNumberOfEventsPerBurst > 0)
            {
                ++burstNumbers;
                lastBurstNumberOfEvents = fNumberOfEvents % fNumberOfEventsPerBurst;
            }
        }

        while(burstNumbers > 0)
        {
            uint32_t currentNumberOfEvents = uint32_t(fNumberOfEventsPerBurst);
            if(burstNumbers == 1) currentNumberOfEvents = lastBurstNumberOfEvents;
            // LOG (INFO) << BOLDYELLOW << "Tool::ReadNEvents : number of events requested is " << +currentNumberOfEvents << RESET;

            if(fTool->ifUseReadNEvents())
                fTool->ReadNEvents(fDetectorContainer->getObject(fBoardId), currentNumberOfEvents);
            else
            {
                LOG(INFO) << BOLDYELLOW << "Will use measureBeBoardData with ReadData " << RESET;
                fTool->fBeBoardInterface->Start(fDetectorContainer->getObject(fBoardId));
                std::this_thread::sleep_for(std::chrono::milliseconds(fTool->getWait()));
                fTool->fBeBoardInterface->Stop(fDetectorContainer->getObject(fBoardId));
                fTool->ReadData(fDetectorContainer->getObject(fBoardId), false);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            // Loop over Events from this Acquisition
            const std::vector<Event*>& events = fTool->GetEvents();
            fTool->setNReadbackEvents(events.size());
            // Assuming all chip will have all channels enabled:
            if(fSameChannelGroupForAllChannels)
            {
                // LOG (INFO) << BOLDYELLOW << "MeasureBeBoardDataPerGroup fSameChannelGroupForAllChannels read-back " << events.size() << " event." << RESET;
                auto channelGroup = fTool->getChannelGroup(fGroupNumber);
                if(channelGroup == nullptr)
                    LOG(ERROR) << BOLDRED << "Channel group does not exist..." << RESET;
                else
                {
                    for(auto& event: events) event->fillDataContainer(fDetectorDataContainer->getObject(fBoardId), channelGroup);
                }
            }
            else
            {
                // LOG(DEBUG) << BOLDYELLOW << "MeasureBeBoardDataPerGroup !fSameChannelGroupForAllChannels read-back " << events.size() << RESET;
                for(auto cOpticalGroup: *fDetectorDataContainer->getObject(fBoardId))
                {
                    for(const auto cHybrid: *cOpticalGroup)
                    {
                        for(const auto cChip: *cHybrid)
                        {
                            auto channelGroup = fTool->getChannelGroup(fGroupNumber, fDetectorDataContainer->getObject(fBoardId)->getId(), cOpticalGroup->getId(), cHybrid->getId(), cChip->getId());
                            if(!channelGroup) continue;
                            for(auto& event: events) event->fillChipDataContainer(cChip, channelGroup, cHybrid->getId());
                        }
                    }
                }
            }
            --burstNumbers;
        }
    }

    void setDataContainer(DetectorDataContainer* detectorDataContainer) { fDetectorDataContainer = detectorDataContainer; }

  private:
    DetectorDataContainer* fDetectorDataContainer;
};

void Tool::measureBeBoardData(uint16_t boardId, uint32_t numberOfEvents, int32_t numberOfEventsPerBurst)
{
    MeasureBeBoardDataPerGroup theScan(this);

    theScan.setDataContainer(fDetectorDataContainer);
    // Make sure async mode uses ReadNEvents

    doScanOnAllGroupsBeBoard(boardId, numberOfEvents, numberOfEventsPerBurst, &theScan);

    if(fDetectorContainer->getObject(boardId)->getBoardType() == BoardType::D19C)
        numberOfEvents = numberOfEvents * (fBeBoardInterface->ReadBoardReg(fDetectorContainer->getObject(boardId), "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity") + 1);

    if(!fUseReadNEvents) numberOfEvents = fNReadbackEvents;

    if(fNormalize)
        fDetectorDataContainer->getObject(boardId)->normalizeAndAverageContainers(fDetectorContainer->getObject(boardId), getChannelGroupHandlerContainer()->getObject(boardId), numberOfEvents);
}

class ScanBeBoardDacPerGroup : public MeasureBeBoardDataPerGroup
{
  public:
    ScanBeBoardDacPerGroup(Tool* theTool) : MeasureBeBoardDataPerGroup(theTool) { ; }
    ~ScanBeBoardDacPerGroup() { ; }

    void operator()() override
    {
        for(size_t dacIt = 0; dacIt < fDacList->size(); ++dacIt)
        {
            fTool->setSameDacBeBoard(static_cast<BeBoard*>(fDetectorContainer->getObject(fBoardId)), fDacName, fDacList->at(dacIt));
            setDataContainer(fDetectorDataContainerVector->at(dacIt));
            MeasureBeBoardDataPerGroup::operator()();
        }
    }

    void setDataContainerVector(std::vector<DetectorDataContainer*>* detectorDataContainerVector) { fDetectorDataContainerVector = detectorDataContainerVector; }
    void setDacName(const std::string& dacName) { fDacName = dacName; }
    void setDacList(const std::vector<uint16_t>* dacList) { fDacList = dacList; }

  private:
    std::vector<DetectorDataContainer*>* fDetectorDataContainerVector;
    const std::vector<uint16_t>*         fDacList;
    std::string                          fDacName;
};

// One dimensional dac scan per BeBoard
void Tool::scanBeBoardDac(uint16_t                             boardId,
                          const std::string&                   dacName,
                          const std::vector<uint16_t>&         dacList,
                          uint32_t                             numberOfEvents,
                          std::vector<DetectorDataContainer*>& detectorContainerVector,
                          int32_t                              numberOfEventsPerBurst)
{
    if(dacList.size() != detectorContainerVector.size())
    {
        LOG(ERROR) << __PRETTY_FUNCTION__ << " dacList and detector container vector have different sizes, aborting";
        abort();
    }

    if(RD53Shared::firstChip->getFrontEndType() == FrontEndType::RD53A)
    {
        // #######################
        // # Loop over DAC ...   #
        // # Loop over goups ... #
        // #######################

        for(size_t dacIt = 0; dacIt < dacList.size(); ++dacIt)
        {
            fDetectorDataContainer = detectorContainerVector.at(dacIt);
            setDacAndMeasureBeBoardData(boardId, dacName, dacList.at(dacIt), numberOfEvents, numberOfEventsPerBurst);
            this->sendData();
        }
    }
    else
    {
        // #######################
        // # Loop over goups ... #
        // # Loop over DAC ...   #
        // #######################

        ScanBeBoardDacPerGroup theScan(this);
        theScan.setDataContainerVector(&detectorContainerVector);
        theScan.setDacName(dacName);
        theScan.setDacList(&dacList);

        doScanOnAllGroupsBeBoard(boardId, numberOfEvents, numberOfEventsPerBurst, &theScan);
        if(fDetectorContainer->getObject(boardId)->getBoardType() == BoardType::D19C)
        {
            numberOfEvents = numberOfEvents * (fBeBoardInterface->ReadBoardReg(fDetectorContainer->getObject(boardId), "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity") + 1);
        }
        for(auto container: detectorContainerVector) container->normalizeAndAverageContainers(fDetectorContainer, getChannelGroupHandlerContainer(), numberOfEvents);
    }
}

// Set global DAC for all CBCs in the BeBoard
void Tool::setAllGlobalDacBeBoard(uint16_t boardId, const std::string& dacName, DetectorDataContainer& globalDACContainer)
{
    for(auto cOpticalGroup: *(fDetectorContainer->getObject(boardId)))
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                fReadoutChipInterface->WriteChipReg(
                    cChip, dacName, globalDACContainer.getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>());
            }
        }
    }
}

// Set local dac per BeBoard
void Tool::setAllLocalDacBeBoard(uint16_t boardId, const std::string& dacName, DetectorDataContainer& globalDACContainer)
{
    for(auto cOpticalGroup: *(fDetectorContainer->getObject(boardId)))
        for(auto cHybrid: *cOpticalGroup)
            for(auto cChip: *cHybrid)
                fReadoutChipInterface->WriteChipAllLocalReg(
                    cChip, dacName, *globalDACContainer.getObject(boardId)->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId()));
}

// Set same global DAC for all chips
void Tool::setSameGlobalDac(const std::string& dacName, const uint16_t dacValue)
{
    for(auto cBoard: *fDetectorContainer) setSameGlobalDacBeBoard(static_cast<BeBoard*>(cBoard), dacName, dacValue);
}

// Set same global DAC for all chips in the BeBoard
void Tool::setSameGlobalDacBeBoard(BeBoard* pBoard, const std::string& dacName, const uint16_t dacValue)
{
    if(fDoBoardBroadcast == false)
    {
        for(auto cOpticalGroup: *pBoard)
            for(auto cHybrid: *cOpticalGroup)
                if(fDoHybridBroadcast == false)
                {
                    for(auto cChip: *cHybrid) { fReadoutChipInterface->WriteChipReg(static_cast<ReadoutChip*>(cChip), dacName, dacValue); }
                }
                else { fReadoutChipInterface->WriteHybridBroadcastChipReg(static_cast<Hybrid*>(cHybrid), dacName, dacValue); }
    }
    else { fReadoutChipInterface->WriteBoardBroadcastChipReg(pBoard, dacName, dacValue); }
}

// Set same local dac for all BeBoard
void Tool::setSameLocalDac(const std::string& dacName, const uint16_t dacValue)
{
    LOG(DEBUG) << BOLDMAGENTA << "Setting local dac [ " << dacName << " ] to " << dacValue << RESET;
    for(auto cBoard: *fDetectorContainer) { setSameLocalDacBeBoard(static_cast<BeBoard*>(cBoard), dacName, dacValue); }

    return;
}

// Set same local dac per BeBoard
void Tool::setSameLocalDacBeBoard(BeBoard* pBoard, const std::string& dacName, const uint16_t dacValue)
{
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                ChannelContainer<uint16_t>* dacVector = new ChannelContainer<uint16_t>(cChip->getNumberOfChannels(), dacValue);
                ChipContainer               theChipContainer(cChip->getId(), cChip->getNumberOfRows(), cChip->getNumberOfCols());
                theChipContainer.setChannelContainer(dacVector);

                fReadoutChipInterface->WriteChipAllLocalReg(cChip, dacName, theChipContainer);
            }
        }
    }
}

void Tool::setSameDacBeBoard(BeBoard* pBoard, const std::string& dacName, const uint16_t dacValue)
{
    // Assumption: 1 BeBoard has only 1 chip flavor
    bool isLocalDac  = false;
    bool isChipFound = false;
    for(auto theOpticalGroup: *pBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            if(theHybrid->size() > 0)
            {
                isLocalDac  = theHybrid->getFirstObject()->isDACLocal(dacName);
                isChipFound = true;
                break;
            }
        }
        if(isChipFound) break;
    }
    if(!isChipFound) return;
    if(isLocalDac) { setSameLocalDacBeBoard(pBoard, dacName, dacValue); }
    else { setSameGlobalDacBeBoard(pBoard, dacName, dacValue); }
}

void Tool::setSameDac(const std::string& dacName, const uint16_t dacValue)
{
    for(auto cBoard: *fDetectorContainer) { setSameDacBeBoard(static_cast<BeBoard*>(cBoard), dacName, dacValue); }
}

std::string Tool::getCalibrationName(void) const
{
    int32_t     status;
    std::string className     = abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status);
    std::string emptyTemplate = "<> ";
    size_t      found         = className.find(emptyTemplate);

    while(found != std::string::npos)
    {
        className.erase(found, emptyTemplate.length());
        found = className.find(emptyTemplate);
    }

    return className;
}
