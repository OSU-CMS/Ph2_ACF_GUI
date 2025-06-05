#include "RD53EyeScanOptimization.h"
#include "Utils/ContainerFactory.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void EyeScanOptimization::ConfigureCalibration()
{
    // ##############################
    // # Initialize sub-calibration #
    // ##############################
    EyeDiag::ConfigureCalibration();

    // #######################
    // # Retrieve parameters #
    // #######################
    rowStart       = this->findValueInSettings<double>("ROWstart");
    rowStop        = this->findValueInSettings<double>("ROWstop");
    colStart       = this->findValueInSettings<double>("COLstart");
    colStop        = this->findValueInSettings<double>("COLstop");
    nEvents        = this->findValueInSettings<double>("nEvents");
    startValueTAP0 = this->findValueInSettings<double>("TAP0Start");
    stopValueTAP0  = this->findValueInSettings<double>("TAP0Stop");
    startValueTAP1 = this->findValueInSettings<double>("TAP1Start");
    stopValueTAP1  = this->findValueInSettings<double>("TAP1Stop");
    startValueTAP2 = this->findValueInSettings<double>("TAP2Start");
    stopValueTAP2  = this->findValueInSettings<double>("TAP2Stop");
    doDisplay      = this->findValueInSettings<double>("DisplayHisto");
    doUpdateChip   = this->findValueInSettings<double>("UpdateChipCfg");
    saveBinaryData = this->findValueInSettings<double>("SaveBinaryData");

    // ##############################
    // # Initialize dac scan values #
    // ##############################
    size_t nSteps = (stopValueTAP0 - startValueTAP0 + 1 >= RD53Shared::MAXSTEPS ? RD53Shared::MAXSTEPS : stopValueTAP0 - startValueTAP0 + 1);
    size_t step   = floor((stopValueTAP0 - startValueTAP0 + 1) / nSteps);
    for(auto i = 0u; i < nSteps; i++) dacListTAP0.push_back(startValueTAP0 + step * i);

    nSteps = (stopValueTAP1 - startValueTAP1 + 1 >= RD53Shared::MAXSTEPS ? RD53Shared::MAXSTEPS : stopValueTAP1 - startValueTAP1 + 1);
    step   = floor((stopValueTAP1 - startValueTAP1 + 1) / nSteps);
    for(auto i = 0u; i < nSteps; i++) dacListTAP1.push_back(startValueTAP1 + step * i);

    nSteps = (stopValueTAP2 - startValueTAP2 + 1 >= RD53Shared::MAXSTEPS ? RD53Shared::MAXSTEPS : stopValueTAP2 - startValueTAP2 + 1);
    step   = floor((stopValueTAP2 - startValueTAP2 + 1) / nSteps);
    for(auto i = 0u; i < nSteps; i++) dacListTAP2.push_back(startValueTAP2 + step * i);
}

void EyeScanOptimization::Running()
{
    theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[EyeScanOptimization::Running] Starting run: " << BOLDYELLOW << theCurrentRun << RESET;

    if(saveBinaryData == true)
    {
        this->addFileHandler(std::string(this->fDirectoryName) + "/Run" + RD53Shared::fromInt2Str(theCurrentRun) + "_EyeScanOptimization.raw", 'w');
        this->initializeWriteFileHandler();
    }

    if(fIs2D)
        EyeScanOptimization::run2d();
    else
        EyeScanOptimization::run();

    EyeScanOptimization::saveChipRegisters(theCurrentRun);
}

void EyeScanOptimization::Stop()
{
    LOG(INFO) << GREEN << "[EyeScanOptimization::Stop] Stopping" << RESET;
    Tool::Stop();

    EyeScanOptimization::draw();
    this->closeFileHandler();

    RD53RunProgress::reset();
}

void EyeScanOptimization::localConfigure(const std::string fileRes_, int currentRun, bool is2D)
{
#ifdef __USE_ROOT__
    histos = nullptr;
#endif

    fIs2D = is2D;

    if(currentRun >= 0)
    {
        theCurrentRun = currentRun;
        LOG(INFO) << GREEN << "[EyeScanOptimization::localConfigure] Starting run: " << BOLDYELLOW << theCurrentRun << RESET;
    }
    EyeScanOptimization::ConfigureCalibration();
    EyeScanOptimization::initializeFiles(fileRes_, currentRun);
}

void EyeScanOptimization::initializeFiles(const std::string fileRes_, int currentRun)
{
    fileRes = fileRes_;

#ifdef __USE_ROOT__
    delete histos;
    histos = new EyeScanOptimizationHistograms;
#endif
}

void EyeScanOptimization::run()
{
    std::cout << " [EyeScanOptimization]" << std::endl;
    const size_t TAPsize = RD53Shared::setBits(RD53Shared::MAXBITCHIPREG) + 1;

    ContainerFactory::copyAndInitChip<GenericDataArray<std::unordered_map<std::string, std::array<float, 7>>, TAPsize>>(*fDetectorContainer, theTAP0scanContainer);
    ContainerFactory::copyAndInitChip<GenericDataArray<std::unordered_map<std::string, std::array<float, 7>>, TAPsize>>(*fDetectorContainer, theTAP1scanContainer);
    ContainerFactory::copyAndInitChip<GenericDataArray<std::unordered_map<std::string, std::array<float, 7>>, TAPsize>>(*fDetectorContainer, theTAP2scanContainer);

    for(const auto cBoard: *fDetectorContainer) static_cast<RD53Interface*>(this->fReadoutChipInterface)->WriteBoardBroadcastChipReg(cBoard, "CML_CONFIG_SER_EN_TAP", 0x0);
    EyeScanOptimization::scanDac("DAC_CML_BIAS_0", dacListTAP0, nEvents, &theTAP0scanContainer);

    for(const auto cBoard: *fDetectorContainer) static_cast<RD53Interface*>(this->fReadoutChipInterface)->WriteBoardBroadcastChipReg(cBoard, "CML_CONFIG_SER_EN_TAP", 0x1);
    for(const auto cBoard: *fDetectorContainer) static_cast<RD53Interface*>(this->fReadoutChipInterface)->WriteBoardBroadcastChipReg(cBoard, "CML_CONFIG_SER_INV_TAP", 0x1);
    EyeScanOptimization::scanDac("DAC_CML_BIAS_1", dacListTAP1, nEvents, &theTAP1scanContainer);

    for(const auto cBoard: *fDetectorContainer) static_cast<RD53Interface*>(this->fReadoutChipInterface)->WriteBoardBroadcastChipReg(cBoard, "CML_CONFIG_SER_EN_TAP", 0x2);
    EyeScanOptimization::scanDac("DAC_CML_BIAS_2", dacListTAP2, nEvents, &theTAP2scanContainer);
}

void EyeScanOptimization::run2d()
{
    std::cout << " [EyeScanOptimization]" << std::endl;

    for(const auto cBoard: *fDetectorContainer) static_cast<RD53Interface*>(this->fReadoutChipInterface)->WriteBoardBroadcastChipReg(cBoard, "CML_CONFIG_SER_EN_TAP", 0x3);
    for(const auto cBoard: *fDetectorContainer) static_cast<RD53Interface*>(this->fReadoutChipInterface)->WriteBoardBroadcastChipReg(cBoard, "CML_CONFIG_SER_INV_TAP", 0x0);
    const size_t TAPsize = RD53Shared::setBits(RD53Shared::MAXBITCHIPREG) + 1;

    ContainerFactory::copyAndInitChip<GenericDataArray<std::unordered_map<std::string, std::array<float, 7>>, TAPsize * TAPsize * TAPsize>>(*fDetectorContainer, the3DContainer);
    EyeScanOptimization::scanDac3D("DAC_CML_BIAS_0", "DAC_CML_BIAS_1", "DAC_CML_BIAS_2", dacListTAP0, dacListTAP1, dacListTAP2, nEvents, &the3DContainer);

    // for (auto tap0 : dacListTAP0){
    //   LOG(INFO) << BOLDMAGENTA << ">>> " << BOLDYELLOW << "DAC_CML_BIAS_0" << BOLDMAGENTA << " value = " << BOLDYELLOW << tap0 << BOLDMAGENTA << " <<<" << RESET;
    //   for(const auto cBoard: *fDetectorContainer) this->fReadoutChipInterface->WriteBoardBroadcastChipReg(cBoard, "DAC_CML_BIAS_0", tap0);
    //   EyeScanOptimization::scanDac2D("DAC_CML_BIAS_1", "DAC_CML_BIAS_2", dacListTAP1, dacListTAP2, nEvents, &theTAP1scanContainer, "TAP0_"+std::to_string(tap0));
    // }
}

void EyeScanOptimization::fillHisto()
{
#ifdef __USE_ROOT__
    histos->fillScanTAP0(theTAP0scanContainer);
    histos->fillScanTAP1(theTAP1scanContainer);
    histos->fillScanTAP2(theTAP2scanContainer);
    histos->fillScan3D(the3DContainer);
#endif
}

void EyeScanOptimization::scanDac(const std::string& regName, const std::vector<uint16_t>& dacList, uint32_t nEvents, DetectorDataContainer* theContainer)
{
    const size_t TAPsize = RD53Shared::setBits(RD53Shared::MAXBITCHIPREG) + 1;

    for(auto i = 0u; i < dacList.size(); i++)
    {
        // ###########################
        // # Download new DAC values #
        // ###########################
        LOG(INFO) << BOLDMAGENTA << ">>> " << BOLDYELLOW << regName << BOLDMAGENTA << " value = " << BOLDYELLOW << dacList[i] << BOLDMAGENTA << " <<<" << RESET;
        for(const auto cBoard: *fDetectorContainer) this->fReadoutChipInterface->WriteBoardBroadcastChipReg(cBoard, regName, dacList[i]);

        // ################
        // # Run analysis #
        // ################
        EyeDiag::run(regName + "_" + std::to_string(dacList[i]));

        // #################
        // # Write results #
        // #################
        for(const auto cBoard: *theContainer)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid)
                        cChip->getSummary<GenericDataArray<std::unordered_map<std::string, std::array<float, 7>>, TAPsize>>().at(i) =
                            EyeDiag::theEyeDiagContainer.getObject(cBoard->getId())
                                ->getObject(cOpticalGroup->getId())
                                ->getObject(cHybrid->getId())
                                ->getObject(cChip->getId())
                                ->getSummary<std::unordered_map<std::string, std::array<float, 7>>>();
    }
}

void EyeScanOptimization::scanDac3D(const std::string&           regName1,
                                    const std::string&           regName2,
                                    const std::string&           regName3,
                                    const std::vector<uint16_t>& dacList1,
                                    const std::vector<uint16_t>& dacList2,
                                    const std::vector<uint16_t>& dacList3,
                                    uint32_t                     nEvents,
                                    DetectorDataContainer*       theContainer)
{
    const size_t TAPsize = RD53Shared::setBits(RD53Shared::MAXBITCHIPREG) + 1;

    for(auto i = 0u; i < dacList1.size(); i++)
    {
        for(auto j = 0u; j < dacList2.size(); j++)
        {
            for(auto k = 0u; k < dacList3.size(); k++)
            {
                // ###########################
                // # Download new DAC values #
                // ###########################
                LOG(INFO) << BOLDMAGENTA << ">>> " << BOLDYELLOW << regName1 << BOLDMAGENTA << " value = " << BOLDYELLOW << dacList1[i] << BOLDMAGENTA << " <<<" << RESET;
                for(const auto cBoard: *fDetectorContainer) this->fReadoutChipInterface->WriteBoardBroadcastChipReg(cBoard, regName1, dacList1[i]);
                LOG(INFO) << BOLDMAGENTA << ">>> " << BOLDYELLOW << regName2 << BOLDMAGENTA << " value = " << BOLDYELLOW << dacList2[j] << BOLDMAGENTA << " <<<" << RESET;
                for(const auto cBoard: *fDetectorContainer) this->fReadoutChipInterface->WriteBoardBroadcastChipReg(cBoard, regName2, dacList2[j]);
                LOG(INFO) << BOLDMAGENTA << ">>> " << BOLDYELLOW << regName3 << BOLDMAGENTA << " value = " << BOLDYELLOW << dacList3[k] << BOLDMAGENTA << " <<<" << RESET;
                for(const auto cBoard: *fDetectorContainer) this->fReadoutChipInterface->WriteBoardBroadcastChipReg(cBoard, regName3, dacList3[k]);

                // ################
                // # Run analysis #
                // ################
                EyeDiag::run(regName1 + "_" + std::to_string(dacList1[i]) + "_" + regName2 + "_" + std::to_string(dacList2[j]) + "_" + regName3 + "_" + std::to_string(dacList3[k]));
                for(const auto cBoard: *theContainer)
                    for(const auto cOpticalGroup: *cBoard)
                        for(const auto cHybrid: *cOpticalGroup)
                            for(const auto cChip: *cHybrid)
                                cChip->getSummary<GenericDataArray<std::unordered_map<std::string, std::array<float, 7>>, TAPsize>>().at(
                                    i + j * dacList1.size() + k * dacList1.size() * dacList2.size()) = EyeDiag::theEyeDiagContainer.getObject(cBoard->getId())
                                                                                                           ->getObject(cOpticalGroup->getId())
                                                                                                           ->getObject(cHybrid->getId())
                                                                                                           ->getObject(cChip->getId())
                                                                                                           ->getSummary<std::unordered_map<std::string, std::array<float, 7>>>();
            }
        }
    }
}

void EyeScanOptimization::saveChipRegisters(int currentRun)
{
    const std::string fileReg("Run" + RD53Shared::fromInt2Str(currentRun) + "_");

    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    static_cast<RD53*>(cChip)->copyMaskFromDefault();
                    if(doUpdateChip == true) static_cast<RD53*>(cChip)->saveRegMap(cChip->getFileName());
                    static_cast<RD53*>(cChip)->saveRegMap(fileReg);
                    std::string command("mv " + cChip->getFileName(fileReg) + " " + RD53Shared::RESULTDIR);
                    system(command.c_str());
                    LOG(INFO) << BOLDBLUE << "\t--> EyeScanOptimization saved the configuration file for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/"
                              << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/" << +cChip->getId() << RESET << BOLDBLUE << "]" << RESET;
                }
}

void EyeScanOptimization::draw()
{
#ifdef __USE_ROOT__
    TApplication* myApp = nullptr;

    if(doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    if((this->fResultFile == nullptr) || (this->fResultFile->IsOpen() == false))
    {
        this->InitResultFile(fileRes);
        LOG(INFO) << BOLDBLUE << "\t--> DataReadbackOptimization saving histograms..." << RESET;
    }

    histos->book(this->fResultFile, *fDetectorContainer, fSettingsMap);
    EyeScanOptimization::fillHisto();
    histos->process();
    this->WriteRootFile();

    if(doDisplay == true) myApp->Run(true);
#endif
}
