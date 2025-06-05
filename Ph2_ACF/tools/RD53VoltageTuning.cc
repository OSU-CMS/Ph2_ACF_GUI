/*!
  \file                  RD53VoltageTuning.cc
  \brief                 Implementaion of Bit Error Rate test
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53VoltageTuning.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void VoltageTuning::ConfigureCalibration()
{
    // #######################
    // # Retrieve parameters #
    // #######################
    CalibBase::ConfigureCalibration();
    targetDig    = this->findValueInSettings<double>("VDDDTrimTarget", 1.3);
    targetAna    = this->findValueInSettings<double>("VDDATrimTarget", 1.2);
    toleranceDig = this->findValueInSettings<double>("VDDDTrimTolerance", 0.02);
    toleranceAna = this->findValueInSettings<double>("VDDATrimTolerance", 0.02);
    doDisplay    = this->findValueInSettings<double>("DisplayHisto");
}

void VoltageTuning::Running()
{
    CalibBase::theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[VoltageTuning::Running] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    VoltageTuning::run();
    VoltageTuning::analyze();
    VoltageTuning::draw();
    VoltageTuning::sendData();
}

void VoltageTuning::sendData()
{
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theVoltageDigitalSerialization("VoltageTuningVoltageDigital");
        theVoltageDigitalSerialization.streamByChipContainer(fDQMStreamer, theDigContainer);

        ContainerSerialization theVoltageAnalogSerialization("VoltageTuningVoltageAnalog");
        theVoltageAnalogSerialization.streamByChipContainer(fDQMStreamer, theAnaContainer);
    }
}

void VoltageTuning::Stop()
{
    LOG(INFO) << GREEN << "[VoltageTuning::Stop] Stopping" << RESET;
    CalibBase::Stop();
}

void VoltageTuning::localConfigure(const std::string& histoFileName, int currentRun)
{
    // ############################
    // # CalibBase localConfigure #
    // ############################
    CalibBase::localConfigure(histoFileName, currentRun);

    histos = nullptr;

    LOG(INFO) << GREEN << "[VoltageTuning::localConfigure] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    // ##########################
    // # Initialize calibration #
    // ##########################
    VoltageTuning::ConfigureCalibration();

    // ###############################
    // # Initialize output directory #
    // ###############################
    this->CreateResultDirectory(dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR, false, false);

    // #########################################
    // # Initialize histogram and binary files #
    // #########################################
    CalibBase::initializeFiles(histoFileName, "VoltageTuning", histos);
}

void VoltageTuning::run()
{
    const size_t          nBitsDig = RD53Shared::firstChip->getFEtype(colStart, colStop)->nBitTrimDig;
    const size_t          nBitsAna = RD53Shared::firstChip->getFEtype(colStart, colStop)->nBitTrimAna;
    const std::string     VDDDreg  = RD53Shared::firstChip->getFEtype(colStart, colStop)->VDDDreadReg;
    const std::string     VDDAreg  = RD53Shared::firstChip->getFEtype(colStart, colStop)->VDDAreadReg;
    std::vector<uint16_t> chipCommandList;
    std::vector<uint16_t> saveRegVal;
    uint16_t              init       = 0;
    float                 targetDig_ = targetDig;
    float                 targetAna_ = targetAna;
    bool                  doRepeatDig;
    bool                  doRepeatAna;

    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, theDigContainer, init);
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, theAnaContainer, init);

    auto RD53ChipInterface = static_cast<RD53Interface*>(this->fReadoutChipInterface);

    // ####################
    // # Pause monitoring #
    // ####################
    if(this->fDetectorMonitor != nullptr) this->fDetectorMonitor->pauseMonitoring();

    // ################################
    // # Prepare query and enable all #
    // ################################
    CalibBase::prepareChipQueryForEnDis("chipSubset");

    for(auto nAttempt = 0; nAttempt < RD53Shared::MAXATTEMPTS; nAttempt++)
    {
        doRepeatDig = false;
        doRepeatAna = false;

        for(const auto cBoard: *fDetectorContainer)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid)
                    {
                        unsigned int it;

                        // ##############
                        // # Start VDDD #
                        // ##############

                        LOG(INFO) << BOLDYELLOW << "VDDD" << RESET << GREEN << " tuning for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/"
                                  << cHybrid->getId() << "/" << +cChip->getId() << RESET << GREEN << "] starts with target value = " << BOLDYELLOW << std::setprecision(3) << targetDig_ << RESET
                                  << GREEN << " and tolerance = " << BOLDYELLOW << toleranceDig << RESET;

                        std::vector<float> trimVoltageDig;
                        std::vector<int>   trimVoltageDigIndex;

                        auto defaultDig = (static_cast<size_t>(RD53Shared::setBits(nBitsAna) * STARTfraction) << nBitsDig) | static_cast<size_t>(RD53Shared::setBits(nBitsDig) * STARTfraction);
                        RD53ChipInterface->WriteChipReg(cChip, "VOLTAGE_TRIM", defaultDig);
                        float initDig = RD53ChipInterface->ReadChipMonitor(cChip, VDDDreg, true) * CONVERSIONfactor;

                        std::vector<int> scanrangeDig = VoltageTuning::createScanRange(cChip, "VOLTAGE_TRIM_DIG", targetDig_, initDig);
                        bool             isUpward     = false;

                        if(initDig < targetDig_) isUpward = true;

                        for(it = 0; it < scanrangeDig.size(); it++)
                        {
                            auto vTrimDecimal = (static_cast<size_t>(RD53Shared::setBits(nBitsAna) * STARTfraction) << nBitsDig) | scanrangeDig[it];

                            RD53ChipInterface->WriteChipReg(cChip, "VOLTAGE_TRIM", vTrimDecimal);
                            float readingDig = RD53ChipInterface->ReadChipMonitor(cChip, VDDDreg, true) * CONVERSIONfactor;
                            float diff       = fabs(readingDig - targetDig_);

                            trimVoltageDig.push_back(diff);
                            trimVoltageDigIndex.push_back(scanrangeDig[it]);

                            if(isUpward && readingDig > (targetDig_ + toleranceDig)) break;
                            if(!isUpward && readingDig < (targetDig_ - toleranceDig)) break;
                        }

                        int minDigIndex    = std::min_element(trimVoltageDig.begin(), trimVoltageDig.end()) - trimVoltageDig.begin();
                        int vdddNewSetting = trimVoltageDigIndex[minDigIndex];

                        LOG(INFO) << BOLDYELLOW << "VDDD" << RESET << GREEN << " best setting: " << BOLDYELLOW << vdddNewSetting << std::setprecision(3) << RESET << GREEN
                                  << ", difference with respect to target = " << BOLDYELLOW << trimVoltageDig[minDigIndex] << RESET << GREEN << " V" << RESET;

                        if(trimVoltageDig[minDigIndex] > toleranceDig)
                        {
                            doRepeatDig = true;
                            LOG(WARNING) << RED << "Optimal value not found: " << BOLDYELLOW << "RETRY" << RESET;
                            break;
                        }
                        else
                            cChip->setEnabled(false);

                        // ##############
                        // # Start VDDA #
                        // ##############

                        LOG(INFO) << BOLDYELLOW << "VDDA" << RESET << GREEN << " tuning for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/"
                                  << cHybrid->getId() << "/" << +cChip->getId() << std::setprecision(3) << RESET << GREEN << "] starts with target value = " << BOLDYELLOW << targetAna_ << RESET
                                  << GREEN << " and tolerance = " << BOLDYELLOW << toleranceAna << RESET << GREEN << " and with VDDD = " << BOLDYELLOW << vdddNewSetting << RESET;

                        std::vector<float> trimVoltageAna;
                        std::vector<int>   trimVoltageAnaIndex;

                        auto defaultAna = (static_cast<size_t>(RD53Shared::setBits(nBitsAna) * STARTfraction) << nBitsDig) | vdddNewSetting;
                        RD53ChipInterface->WriteChipReg(cChip, "VOLTAGE_TRIM", defaultAna);
                        float initAna = RD53ChipInterface->ReadChipMonitor(cChip, VDDAreg, true) * CONVERSIONfactor;

                        std::vector<int> scanrangeAna = VoltageTuning::createScanRange(cChip, "VOLTAGE_TRIM_ANA", targetAna_, initAna);
                        isUpward                      = false;

                        if(initAna < targetAna_) isUpward = true;

                        for(it = 0; it < scanrangeAna.size(); it++)
                        {
                            auto vTrimDecimal = (scanrangeAna[it] << nBitsDig) | vdddNewSetting;

                            RD53ChipInterface->WriteChipReg(cChip, "VOLTAGE_TRIM", vTrimDecimal);
                            float readingAna = RD53ChipInterface->ReadChipMonitor(cChip, VDDAreg, true) * CONVERSIONfactor;
                            float diff       = fabs(readingAna - targetAna_);

                            trimVoltageAna.push_back(diff);
                            trimVoltageAnaIndex.push_back(scanrangeAna[it]);

                            if(isUpward && readingAna > (targetAna_ + toleranceAna)) break;
                            if(!isUpward && readingAna < (targetAna_ - toleranceAna)) break;
                        }

                        int minAnaIndex    = std::min_element(trimVoltageAna.begin(), trimVoltageAna.end()) - trimVoltageAna.begin();
                        int vddaNewSetting = trimVoltageAnaIndex[minAnaIndex];

                        LOG(INFO) << BOLDYELLOW << "VDDA" << RESET << GREEN << " best setting: " << BOLDYELLOW << vddaNewSetting << std::setprecision(3) << RESET << GREEN
                                  << ", difference with respect to target = " << BOLDYELLOW << trimVoltageAna[minAnaIndex] << RESET << GREEN << " V" << RESET;

                        if(trimVoltageAna[minAnaIndex] > toleranceAna)
                        {
                            doRepeatAna = true;
                            LOG(WARNING) << RED << "Optimal value not found: " << BOLDYELLOW << "RETRY" << RESET;
                            break;
                        }
                        else
                            cChip->setEnabled(false);

                        // ################
                        // # Final values #
                        // ################

                        auto finalDecimal = (vddaNewSetting << nBitsDig) | vdddNewSetting;

                        RD53ChipInterface->WriteChipReg(cChip, "VOLTAGE_TRIM", finalDecimal);

                        auto finalVDDD = RD53ChipInterface->ReadChipMonitor(cChip, VDDDreg, true) * CONVERSIONfactor;
                        auto finalVDDA = RD53ChipInterface->ReadChipMonitor(cChip, VDDAreg, true) * CONVERSIONfactor;

                        LOG(INFO) << CYAN << "Final voltage readings after tuning" << RESET;
                        LOG(INFO) << BOLDBLUE << "\t--> Final VDDD reading = " << std::setprecision(3) << BOLDYELLOW << finalVDDD << BOLDBLUE << " V" << RESET;
                        LOG(INFO) << BOLDBLUE << "\t--> Final VDDA reading = " << std::setprecision(3) << BOLDYELLOW << finalVDDA << BOLDBLUE << " V" << RESET;

                        theDigContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() = vdddNewSetting;
                        theAnaContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() = vddaNewSetting;
                    }

        if((doRepeatDig == false) && (doRepeatAna == false))
        {
            if((targetDig_ != targetDig) || (targetAna_ != targetAna))
                LOG(WARNING) << BOLDBLUE << "\t--> The original target values could not be achieved --> they were lowered by an automatic procedure" << RESET;
            break;
        }
        else
        {
            if(doRepeatDig == true) targetDig_ -= toleranceDig * NSIGMA;
            if(doRepeatAna == true) targetAna_ -= toleranceAna * NSIGMA;
        }

        // ##################
        // # Reset sequence #
        // ##################
        CalibBase::ResetBoards();
    }

    if((doRepeatDig == true) || (doRepeatAna == true)) LOG(ERROR) << BOLDRED << "The calibration was not able to run successfully on all chips" << RESET;

    // ###################################
    // # Read analog current consumption #
    // ###################################
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    // ###############################
                    // # Save original configuration #
                    // ###############################
                    saveRegVal.clear();
                    for(const auto& regName: static_cast<RD53*>(cChip)->getFEtype()->CoreColRegs) saveRegVal.push_back(cChip->getRegMap().find(regName)->second.fValue);

                    // ########################
                    // # Disable all channels #
                    // ########################
                    chipCommandList.clear();
                    RD53ChipInterface->EnDisChip(cChip, chipCommandList, false);
                    static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->WriteChipCommands(chipCommandList, cChip->getHybridId());

                    RD53ChipInterface->MaskAllChannels(cChip, true);
                    LOG(INFO) << GREEN << "Disabling all pixels for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId()
                              << "/" << +cChip->getId() << RESET << GREEN << "]" << RESET;
                    auto allDisabled_current = RD53ChipInterface->ReadChipMonitor(cChip, "ANA_IN_CURR", true);

                    // #######################
                    // # Enable all channels #
                    // #######################
                    chipCommandList.clear();
                    RD53ChipInterface->EnDisChip(cChip, chipCommandList, true);
                    static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->WriteChipCommands(chipCommandList, cChip->getHybridId());

                    RD53ChipInterface->MaskAllChannels(cChip, false);
                    LOG(INFO) << GREEN << "Enabling all pixels for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId()
                              << "/" << +cChip->getId() << RESET << GREEN << "]" << RESET;
                    auto allEnabled_current = RD53ChipInterface->ReadChipMonitor(cChip, "ANA_IN_CURR", true);

                    // ##################################
                    // # Restore original configuration #
                    // ##################################
                    int it(0);
                    for(const auto& regName: static_cast<RD53*>(cChip)->getFEtype()->CoreColRegs) RD53ChipInterface->WriteChipReg(cChip, regName, saveRegVal.at(it++));
                    static_cast<RD53*>(cChip)->copyMaskFromDefault();
                    RD53ChipInterface->ConfigureChipOriginalMask(cChip, false, true);

                    LOG(INFO) << GREEN << "Analog current consumption for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/"
                              << cHybrid->getId() << "/" << +cChip->getId() << RESET << GREEN << "]" << RESET;
                    LOG(INFO) << BOLDBLUE << "\t--> Entire chip: " << std::setprecision(3) << BOLDYELLOW << allEnabled_current - allDisabled_current << BOLDBLUE << " uA" << RESET;
                    LOG(INFO) << BOLDBLUE << "\t--> Single pixel cell: " << std::setprecision(6) << BOLDYELLOW
                              << (allEnabled_current - allDisabled_current) / (RD53Shared::firstChip->getNRows() * RD53Shared::firstChip->getNCols()) << BOLDBLUE << " uA" << RESET;
                }

    // ################################
    // # Restore query and enable all #
    // ################################
    fDetectorContainer->resetReadoutChipQueryFunction();

    // #####################
    // # Resume monitoring #
    // #####################
    if(this->fDetectorMonitor != nullptr) this->fDetectorMonitor->resumeMonitoring();
}

void VoltageTuning::draw(bool saveData)
{
#ifdef __USE_ROOT__
    TApplication* myApp = nullptr;

    if(doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    CalibBase::bookHistoSaveMetadata(histos);
    VoltageTuning::fillHisto();
    histos->process();

    if(doDisplay == true) myApp->Run(true);
#endif
}

void VoltageTuning::analyze()
{
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                    LOG(INFO) << GREEN << "VDDD and VDDA for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/"
                              << +cChip->getId() << RESET << GREEN << "] are: VDDD = " << BOLDYELLOW
                              << theDigContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() << RESET
                              << GREEN << ", VDDA = " << BOLDYELLOW
                              << theAnaContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() << RESET;
}

void VoltageTuning::fillHisto()
{
#ifdef __USE_ROOT__
    histos->fillDig(theDigContainer);
    histos->fillAna(theAnaContainer);
#endif
}

std::vector<int> VoltageTuning::createScanRange(Chip* pChip, const std::string regName, float target, float initial)
{
    std::vector<int> scanRange;

    if(initial <= target)
        for(int vTrim = (RD53Shared::setBits(pChip->getNumberOfBits(regName)) + 1) / 2; vTrim <= static_cast<int>(RD53Shared::setBits(pChip->getNumberOfBits(regName))); vTrim++)
            scanRange.push_back(vTrim);
    else if(initial > target)
        for(int vTrim = (RD53Shared::setBits(pChip->getNumberOfBits(regName)) + 1) / 2; vTrim >= 0; vTrim--) scanRange.push_back(vTrim);

    return scanRange;
}
