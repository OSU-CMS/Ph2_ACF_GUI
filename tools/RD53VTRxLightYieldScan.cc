/*!
  \file                  RD53VTRxLightYieldScan.cc
  \brief                 Implementaion of VTRx light yield scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  29/11/24
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53VTRxLightYieldScan.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void VTRxLightYieldScan::ConfigureCalibration()
{
    // #######################
    // # Retrieve parameters #
    // #######################
    CalibBase::ConfigureCalibration();
    biasStart       = this->findValueInSettings<double>("VTRxBiasStart");
    biasStop        = this->findValueInSettings<double>("VTRxBiasStop");
    biasStep        = this->findValueInSettings<double>("VTRxBiasStep", 1);
    modulationStart = this->findValueInSettings<double>("VTRxModulationStart");
    modulationStop  = this->findValueInSettings<double>("VTRxModulationStop");
    modulationStep  = this->findValueInSettings<double>("VTRxModulationStep", 1);
    doDisplay       = this->findValueInSettings<double>("DisplayHisto");
    doUpdateChip    = this->findValueInSettings<double>("UpdateChipCfg");

    // ##############################
    // # Initialize dac scan values #
    // ##############################
    size_t nSteps = (biasStop - biasStart) / biasStep + 1;
    for(auto i = 0u; i < nSteps; i++) dac1List.push_back(biasStart + biasStep * i);
    nSteps = (modulationStop - modulationStart) / modulationStep + 1;
    for(auto i = 0u; i < nSteps; i++) dac2List.push_back(modulationStart + modulationStep * i);
}

void VTRxLightYieldScan::Running()
{
    CalibBase::theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[VTRxLightYieldScan::Running] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    VTRxLightYieldScan::run();
    VTRxLightYieldScan::draw();
    VTRxLightYieldScan::sendData();
}

void VTRxLightYieldScan::sendData()
{
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theContainerSerialization("VTRxLightYieldScan");
        theContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, theVTRxLightYieldScanContainer);
    }
}

void VTRxLightYieldScan::Stop()
{
    LOG(INFO) << GREEN << "[VTRxLightYieldScan::Stop] Stopping" << RESET;
    CalibBase::Stop();
}

void VTRxLightYieldScan::localConfigure(const std::string& histoFileName, int currentRun)
{
    // ############################
    // # CalibBase localConfigure #
    // ############################
    CalibBase::localConfigure(histoFileName, currentRun);

    histos = nullptr;

    LOG(INFO) << GREEN << "[VTRxLightYieldScan::localConfigure] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    // ##########################
    // # Initialize calibration #
    // ##########################
    VTRxLightYieldScan::ConfigureCalibration();

    // ###############################
    // # Initialize output directory #
    // ###############################
    this->CreateResultDirectory(dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR, false, false);

    // #########################################
    // # Initialize histogram and binary files #
    // #########################################
    CalibBase::initializeFiles(histoFileName, "VTRxLightYieldScan", histos);
}

void VTRxLightYieldScan::run()
{
    ContainerFactory::copyAndInitOpticalGroup<std::vector<float>>(*fDetectorContainer, theVTRxLightYieldScanContainer);

    // ####################
    // # Pause monitoring #
    // ####################
    if(this->fDetectorMonitor != nullptr) this->fDetectorMonitor->pauseMonitoring();

    for(auto cBoard: *fDetectorContainer)
        for(auto cOpticalGroup: *cBoard)
        {
            theVTRxLightYieldScanContainer.getOpticalGroup(cBoard->getId(), cOpticalGroup->getId())->getSummary<std::vector<float>>().clear();

            for(auto i = 0u; i < dac1List.size(); i++)
            {
                if(cOpticalGroup->flpGBT == nullptr) throw std::runtime_error("LpGBT not enabled in configuration file for optical group ID " + std::to_string(cOpticalGroup->getId()));
                this->flpGBTInterface->WriteChipReg(cOpticalGroup->flpGBT, "_I2CVTRxRegCH0BIAS", dac1List[i]);

                for(auto j = 0u; j < dac2List.size(); j++)
                {
                    this->flpGBTInterface->WriteChipReg(cOpticalGroup->flpGBT, "_I2CVTRxRegCH0MOD", dac2List[j] | 0x80);
                    std::this_thread::sleep_for(std::chrono::microseconds(lpGBTconstants::DEEPSLEEP));
                    auto value = static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->GetSFPParameter("RX", flpGBTInterface->GetSFPchannel(cOpticalGroup));

                    // #################
                    // # Progress menu #
                    // #################
                    LOG(INFO) << CYAN << "************* " << GREEN << "Scanning" << CYAN << " *************" << RESET;
                    LOG(INFO) << GREEN << "Bias: " << BOLDYELLOW << std::setw(3) << std::fixed << dac1List[i] << "/" << std::setw(3) << std::fixed << dac1List[dac1List.size() - 1] << RESET << GREEN
                              << " -- Modulation: " << BOLDYELLOW << std::setw(3) << std::fixed << dac2List[j] << "/" << std::setw(3) << std::fixed << dac2List[dac2List.size() - 1] << RESET;
                    LOG(INFO) << CYAN << "************************************" << RESET;
                    if((i < dac1List.size() - 1) || (j < dac2List.size() - 1)) std::cout << std::setprecision(-1) << "\x1b[A\x1b[A\x1b[A";

                    theVTRxLightYieldScanContainer.getOpticalGroup(cBoard->getId(), cOpticalGroup->getId())->getSummary<std::vector<float>>().push_back(value);
                }
            }
        }

    // #####################
    // # Resume monitoring #
    // #####################
    if(this->fDetectorMonitor != nullptr) this->fDetectorMonitor->resumeMonitoring();
}

void VTRxLightYieldScan::draw(bool saveData)
{
    if(saveData == true) CalibBase::saveChipRegisters(doUpdateChip);

#ifdef __USE_ROOT__
    TApplication* myApp = nullptr;

    if(doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    CalibBase::bookHistoSaveMetadata(histos);
    VTRxLightYieldScan::fillHisto();
    histos->process();

    if(doDisplay == true) myApp->Run(true);
#endif
}

void VTRxLightYieldScan::fillHisto()
{
#ifdef __USE_ROOT__
    histos->fillIntensity(theVTRxLightYieldScanContainer);
#endif
}
