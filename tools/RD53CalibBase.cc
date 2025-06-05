/*!
  \file                  RD53CalibBase.cc
  \brief                 Implementaion of CalibBase
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53CalibBase.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void CalibBase::ConfigureCalibration()
{
    // #######################
    // # Retrieve parameters #
    // #######################
    splitFile     = this->findValueInSettings<double>("DoSplitByBoardHybrid", false);
    rowStart      = this->findValueInSettings<double>("ROWstart");
    rowStop       = this->findValueInSettings<double>("ROWstop");
    colStart      = this->findValueInSettings<double>("COLstart");
    colStop       = this->findValueInSettings<double>("COLstop");
    nEvents       = this->findValueInSettings<double>("nEvents", 1);
    nEvtsBurst    = this->findValueInSettings<double>("nEvtsBurst", 1) < nEvents ? this->findValueInSettings<double>("nEvtsBurst") : nEvents;
    nTRIGxEvent   = this->findValueInSettings<double>("nTRIGxEvent");
    dataOutputDir = this->findValueInSettings<std::string>("DataOutputDir", "");
    for(const auto cBoard: *fDetectorContainer) static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->showRunProgress = this->findValueInSettings<double>("ShowRunProgress", 1);
}

void CalibBase::Stop()
{
    Tool::Stop();

#ifdef __USE_ROOT__
    if(splitFile == true)
    {
        if((this->fResultFile != nullptr) && (this->fResultFile->IsOpen())) splitHistoFile(this->fResultFile);
        if((this->fDetectorMonitor != nullptr) && (this->fDetectorMonitor->getMonitorFile() != nullptr) && (this->fDetectorMonitor->getMonitorFile()->IsOpen()))
        {
            this->fDetectorMonitor->getMonitorFile()->Write();
            splitHistoFile(this->fDetectorMonitor->getMonitorFile());
        }
    }
#endif
    this->CloseResultFile();

    RD53RunProgress::reset();
}

void CalibBase::chipErrorReport() const
{
    if(showErrorReport == true)
        for(const auto cBoard: *fDetectorContainer)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid)
                    {
                        LOG(INFO) << GREEN << "Readout chip error report for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/"
                                  << cHybrid->getId() << "/" << +cChip->getId() << RESET << GREEN << "]" << RESET;
                        static_cast<RD53Interface*>(this->fReadoutChipInterface)->ChipErrorReport(cChip);
                    }
}

void CalibBase::copyMaskFromDefault(const std::string& which) const
{
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid) static_cast<RD53*>(cChip)->copyMaskFromDefault(which);
}

void CalibBase::saveChipRegisters(bool doUpdateChip)
{
    const std::string fileReg("Run" + RD53Shared::fromInt2Str(theCurrentRun) + "_");

    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
        {
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(doUpdateChip == true) cChip->saveRegMap(cChip->getFileName());
                    static_cast<RD53*>(cChip)->saveRegMap(cChip->getFileName(fileReg));
                    std::string command("mv " + cChip->getFileName(fileReg) + " " + this->fDirectoryName);
                    system(command.c_str());
                    LOG(INFO) << BOLDBLUE << "\t--> Current calibration saved the configuration file for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/"
                              << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/" << +cChip->getId() << RESET << BOLDBLUE << "] " << BOLDYELLOW << cChip->getFileName(fileReg) << RESET;
                }

            if(cOpticalGroup->flpGBT != nullptr)
            {
                if(doUpdateChip == true) cOpticalGroup->flpGBT->saveRegMap(cOpticalGroup->flpGBT->getFileName());
                cOpticalGroup->flpGBT->saveRegMap(cOpticalGroup->flpGBT->getFileName(fileReg));
                std::string command("mv " + cOpticalGroup->flpGBT->getFileName(fileReg) + " " + this->fDirectoryName);
                system(command.c_str());

                LOG(INFO) << BOLDBLUE << "\t--> Current calibration saved the LpGBT configuration file for [board/opticalGroup = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId()
                          << RESET << BOLDBLUE << "] " << BOLDYELLOW << cOpticalGroup->flpGBT->getFileName(fileReg) << RESET;
            }
        }
}

void CalibBase::downloadNewDACvalues(DetectorDataContainer& DACcontainer, const std::vector<const char*>& regNames, bool silentDownload, bool checkAgainst, int value, bool doNotDownload)
{
    const auto            chipInterface = static_cast<RD53Interface*>(this->fReadoutChipInterface);
    std::vector<uint16_t> chipCommandList;
    std::vector<uint32_t> hybridCommandList;

    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
        {
            hybridCommandList.clear();

            for(const auto cHybrid: *cOpticalGroup)
            {
                chipCommandList.clear();

                for(const auto cChip: *cHybrid)
                    for(const auto& regName: regNames)
                    {
                        if(((checkAgainst == true) &&
                            (DACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() != value)) ||
                           (checkAgainst == false))
                        {
                            chipInterface->PackWriteCommand(
                                cChip,
                                regName,
                                DACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>(),
                                chipCommandList,
                                true);

                            if(silentDownload == false)
                                LOG(INFO) << BOLDMAGENTA << ">>> " << (checkAgainst == true ? "Best " : "") << BOLDYELLOW << regName << BOLDMAGENTA
                                          << " value for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/"
                                          << +cChip->getId() << RESET << BOLDMAGENTA << "] = " << RESET << BOLDYELLOW
                                          << DACcontainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>()
                                          << BOLDMAGENTA << " <<<" << RESET;
                        }
                        else
                        {
                            if(silentDownload == false)
                                LOG(WARNING) << BOLDRED << ">>> Best " << BOLDYELLOW << regName << BOLDRED << " value for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/"
                                             << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/" << +cChip->getId() << BOLDRED << "] was not found <<<" << RESET;
                            return;
                        }
                    }

                chipInterface->PackHybridCommands(cBoard, chipCommandList, cHybrid->getId(), hybridCommandList);
            }

            if(doNotDownload == false) chipInterface->SendHybridCommands(cBoard, hybridCommandList);
        }
}

void CalibBase::saveSCurveOrGaindValues(const std::vector<DetectorDataContainer*>& detectorContainerVector,
                                        const std::vector<uint16_t>&               dacList,
                                        size_t                                     offset,
                                        size_t                                     nEvents,
                                        const std::string&                         name)
{
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    std::stringstream myString;
                    myString.clear();
                    myString.str("");
                    myString << this->fDirectoryName + "/Run" + RD53Shared::fromInt2Str(theCurrentRun) + "_" << name << "_"
                             << "B" << std::setfill('0') << std::setw(2) << +cBoard->getId() << "_"
                             << "O" << std::setfill('0') << std::setw(2) << +cOpticalGroup->getId() << "_"
                             << "M" << std::setfill('0') << std::setw(2) << +cHybrid->getId() << "_"
                             << "C" << std::setfill('0') << std::setw(2) << +cChip->getId() << ".dat";
                    std::ofstream fileOutID(myString.str(), std::ios::out);
                    for(auto i = 0u; i < dacList.size(); i++)
                    {
                        fileOutID << "Iteration " << i << " --- reg = " << dacList[i] - offset << std::endl;
                        for(auto row = 0u; row < RD53Shared::firstChip->getNRows(); row++)
                            for(auto col = 0u; col < RD53Shared::firstChip->getNCols(); col++)
                                if(cChip->getChipOriginalMask()->isChannelEnabled(row, col) && this->getChannelGroupHandlerContainer()
                                                                                                   ->getObject(cBoard->getId())
                                                                                                   ->getObject(cOpticalGroup->getId())
                                                                                                   ->getObject(cHybrid->getId())
                                                                                                   ->getObject(cChip->getId())
                                                                                                   ->getSummary<std::shared_ptr<ChannelGroupHandler>>()
                                                                                                   ->allChannelGroup()
                                                                                                   ->isChannelEnabled(row, col))
                                    fileOutID << "r " << row << " c " << col << " h "
                                              << detectorContainerVector[i]
                                                         ->getObject(cBoard->getId())
                                                         ->getObject(cOpticalGroup->getId())
                                                         ->getObject(cHybrid->getId())
                                                         ->getObject(cChip->getId())
                                                         ->getChannel<OccupancyAndPh>(row, col)
                                                         .fOccupancy *
                                                     nEvents
                                              << " a "
                                              << detectorContainerVector[i]
                                                     ->getObject(cBoard->getId())
                                                     ->getObject(cOpticalGroup->getId())
                                                     ->getObject(cHybrid->getId())
                                                     ->getObject(cChip->getId())
                                                     ->getChannel<OccupancyAndPh>(row, col)
                                                     .fPh
                                              << std::endl;
                    }
                    fileOutID.close();
                }
}

uint8_t CalibBase::assignGroupType(RD53Shared::INJtype injType) const
{
    auto groupType = (injType == RD53Shared::INJtype::None) ? RD53GroupType::AllPixels : RD53GroupType::Groups;

    if(injType == RD53Shared::INJtype::XtalkCoupled)
        groupType = RD53GroupType::XtalkCoupled;
    else if(injType == RD53Shared::INJtype::XtalkDeCoupled)
        groupType = RD53GroupType::XtalkDeCoupled;
    else if(injType == RD53Shared::INJtype::Custom)
        groupType = RD53GroupType::Custom;

    return groupType;
}

void CalibBase::prepareChipQueryForEnDis(const std::string& queryName)
{
    auto chipSubset = [](const ChipContainer* theChip) { return theChip->isEnabled(); };

    fDetectorContainer->resetReadoutChipQueryFunction();
    fDetectorContainer->addReadoutChipQueryFunction(chipSubset, queryName);
}

void CalibBase::setChipEnDis(bool enable)
{
    const auto            chipInterface = static_cast<RD53Interface*>(this->fReadoutChipInterface);
    std::vector<uint16_t> chipCommandList;
    std::vector<uint32_t> hybridCommandList;

    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
        {
            hybridCommandList.clear();

            for(const auto cHybrid: *cOpticalGroup)
            {
                chipCommandList.clear();

                for(auto i = 0u; i < cHybrid->fullSize(); i++)
                {
                    cHybrid->at(i)->setEnabled(enable);
                    chipInterface->EnDisChip(static_cast<RD53*>(cHybrid->at(i)), chipCommandList, enable);
                }

                chipInterface->PackHybridCommands(cBoard, chipCommandList, cHybrid->getId(), hybridCommandList);
            }

            chipInterface->SendHybridCommands(cBoard, hybridCommandList);
        }
}

bool CalibBase::shiftEnable(size_t indx)
{
    const auto            chipInterface = static_cast<RD53Interface*>(this->fReadoutChipInterface);
    std::vector<uint16_t> chipCommandList;
    std::vector<uint32_t> hybridCommandList;
    bool                  isDetectorEmpty = false;

    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
        {
            hybridCommandList.clear();

            for(const auto cHybrid: *cOpticalGroup)
            {
                chipCommandList.clear();

                // #########
                // # Index #
                // #########
                if(indx < cHybrid->fullSize())
                {
                    cHybrid->at(indx)->setEnabled(true);
                    chipInterface->EnDisChip(static_cast<RD53*>(cHybrid->at(indx)), chipCommandList, true);
                }

                // #############
                // # Index - 1 #
                // #############
                if((indx > 0) && (indx <= cHybrid->fullSize()))
                {
                    cHybrid->at(indx - 1)->setEnabled(false);
                    chipInterface->EnDisChip(static_cast<RD53*>(cHybrid->at(indx - 1)), chipCommandList, false);
                }

                isDetectorEmpty |= (cHybrid->size() == 0);

                chipInterface->PackHybridCommands(cBoard, chipCommandList, cHybrid->getId(), hybridCommandList);
            }

            chipInterface->SendHybridCommands(cBoard, hybridCommandList);
        }

    return isDetectorEmpty;
}

void CalibBase::setSinglePixel(ReadoutChip* pChip, size_t row, size_t col, bool enable, bool inject)
{
    RD53* pRD53 = static_cast<RD53*>(pChip);

    pRD53->enablePixel(row, col, enable);
    pRD53->injectPixel(row, col, inject);
    static_cast<RD53Interface*>(this->fReadoutChipInterface)->WriteRD53Mask(pRD53, 2, false, row, col);
}

void CalibBase::ResetBoardsReadBkFIFO()
{
    for(const auto cBoard: *fDetectorContainer) static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->ResetReadBkFIFO();
}

void CalibBase::ResetBoards()
{
    for(const auto cBoard: *fDetectorContainer)
    {
        static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->ResetBoard();
        std::this_thread::sleep_for(std::chrono::milliseconds(RD53Shared::SUPERDEEPSLEEP));
        static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->ConfigureBoard(cBoard);
        std::this_thread::sleep_for(std::chrono::milliseconds(RD53Shared::SUPERDEEPSLEEP));
        this->ConfigureIT(cBoard);
        this->ConfigureFrontendIT(cBoard);
    }
}

void CalibBase::WriteBroadcastChipReg(const std::string& regName, const uint16_t value)
{
    for(const auto cBoard: *fDetectorContainer) this->fReadoutChipInterface->WriteBoardBroadcastChipReg(cBoard, regName, value);
}

void CalibBase::SilentRunning(bool doSilentRunning)
{
    for(const auto cBoard: *fDetectorContainer)
    {
        static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->silentRunning = doSilentRunning;
        static_cast<RD53Interface*>(this->fReadoutChipInterface)->silentRunning            = doSilentRunning;
    }
}

void CalibBase::localConfigure(const std::string& histoFileName, int currentRun)
{
    theCurrentRun = currentRun;
    for(const auto cBoard: *fDetectorContainer)
    {
        static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->resetNcorruptedNevents();
        static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->resetNtrialsNevents();
    }
}

// #######################################
// # Split output file by Board & Hybrid #
// #######################################

#ifdef __USE_ROOT__
bool CalibBase::splitHistoFile(TFile* theInputFile)
{
    LOG(INFO) << GREEN << "Splitting ROOT file " << BOLDYELLOW << theInputFile->GetName() << RESET << GREEN << " by Board & Hybrid" << RESET;

    const std::string detectorFolder = "Detector";
    if(CalibBase::openRootFileFolder(theInputFile, detectorFolder) == false) return false;

    for(const auto cBoard: *fDetectorContainer)
    {
        const std::string boardFolder     = "Board_" + std::to_string(cBoard->getId());
        const std::string fullBoardFolder = detectorFolder + "/" + boardFolder;
        if(CalibBase::openRootFileFolder(theInputFile, fullBoardFolder) == false) return false;

        for(const auto cOpticalGroup: *cBoard)
        {
            const std::string opticalGroupFolder     = "OpticalGroup_" + std::to_string(cOpticalGroup->getId());
            const std::string fullOpticalGroupFolder = fullBoardFolder + "/" + opticalGroupFolder;
            if(CalibBase::openRootFileFolder(theInputFile, fullOpticalGroupFolder) == false) return false;

            for(const auto cHybrid: *cOpticalGroup)
            {
                const std::string hybridFolder     = "Hybrid_" + std::to_string(cHybrid->getId());
                const std::string fullHybridFolder = fullOpticalGroupFolder + "/" + hybridFolder;
                if(CalibBase::openRootFileFolder(theInputFile, fullHybridFolder) == false) return false;

                // ##########################
                // # Create new output file #
                // ##########################
                std::string theOutputFileName = theInputFile->GetName();
                theOutputFileName.replace(theOutputFileName.find(".root"), 5, "_" + boardFolder + "_" + hybridFolder + ".root");
                TFile* theOutputFile = TFile::Open(theOutputFileName.c_str(), "RECREATE");

                // ################
                // # Copy content #
                // ################
                if(theOutputFile && (theOutputFile->IsZombie() == false)) CalibBase::copyDirectories(theInputFile, theOutputFile, boardFolder, hybridFolder);

                // #####################
                // # Close output file #
                // #####################
                theOutputFile->Close();
                delete theOutputFile;
            }
        }
    }

    return true;
}

void CalibBase::copyDirectories(TFile* theInputFile, TFile* theOutputFile, const std::string& boardName, const std::string& hybridName)
{
    const std::string detectorFolder = "Detector";
    CalibBase::copyContent(theInputFile, theOutputFile, detectorFolder);

    for(const auto cBoard: *fDetectorContainer)
    {
        const std::string boardFolder = "Board_" + std::to_string(cBoard->getId());

        if(boardFolder != boardName) continue;

        const std::string fullBoardFolder = detectorFolder + "/" + boardFolder;
        CalibBase::copyContent(theInputFile, theOutputFile, fullBoardFolder);

        for(const auto cOpticalGroup: *cBoard)
        {
            const std::string opticalGroupFolder     = "OpticalGroup_" + std::to_string(cOpticalGroup->getId());
            const std::string fullOpticalGroupFolder = fullBoardFolder + "/" + opticalGroupFolder;
            CalibBase::copyContent(theInputFile, theOutputFile, fullOpticalGroupFolder);

            for(const auto cHybrid: *cOpticalGroup)
            {
                const std::string hybridFolder = "Hybrid_" + std::to_string(cHybrid->getId());

                if(hybridFolder != hybridName) continue;

                const std::string fullHybridFolder = fullOpticalGroupFolder + "/" + hybridFolder;
                CalibBase::copyContent(theInputFile, theOutputFile, fullHybridFolder);

                for(const auto cChip: *cHybrid)
                {
                    const std::string chipFolder     = "Chip_" + std::to_string(cChip->getId());
                    const std::string fullChipFolder = fullHybridFolder + "/" + chipFolder;
                    CalibBase::copyContent(theInputFile, theOutputFile, fullChipFolder);

                    const std::string channelFolder     = "Channel";
                    const std::string fullChannelFolder = fullChipFolder + "/" + channelFolder;
                    CalibBase::copyContent(theInputFile, theOutputFile, fullChannelFolder);
                }
            }
        }
    }
}

void CalibBase::copyContent(TFile* theInputFile, TFile* theOutputFile, const std::string& dirName)
{
    // ############################
    // # Set input file directory #
    // ############################
    theInputFile->cd(dirName.c_str());
    TDirectory* inputDir = gDirectory;

    // #############################
    // # Set output file directory #
    // #############################
    theOutputFile->mkdir(dirName.c_str());
    theOutputFile->cd(dirName.c_str());
    TDirectory* outputDir = gDirectory;
    outputDir->cd();

    TKey* key;
    TIter nextkey(inputDir->GetListOfKeys());

    while((key = (TKey*)nextkey()))
    {
        const char* className = key->GetClassName();
        TClass*     theClass  = gROOT->GetClass(className);

        if(!theClass || (theClass->InheritsFrom(TDirectory::Class()))) continue;

        if(theClass->InheritsFrom(TTree::Class()))
        {
            if(!outputDir->FindObject(key->GetName()))
            {
                inputDir->cd();
                TTree* T = (TTree*)inputDir->Get(key->GetName());
                outputDir->cd();
                TTree* newT = T->CloneTree(-1, "fast");
                newT->Write();
                delete newT;
            }
        }
        else
        {
            inputDir->cd();
            TObject* newO = key->ReadObj();
            outputDir->cd();
            newO->Write(key->GetName(), TObject::kOverwrite);
            delete newO;
        }
    }

    outputDir->SaveSelf(kTRUE);
}

bool CalibBase::openRootFileFolder(TFile* theInputFile, const std::string& folderName)
{
    if(theInputFile->GetDirectory(folderName.c_str()) != nullptr)
    {
        theInputFile->cd(folderName.c_str());
        return true;
    }

    return false;
}
#endif
