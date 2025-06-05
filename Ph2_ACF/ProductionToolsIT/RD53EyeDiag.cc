/*!
  \file                  RD53EyeDiag.h
  \brief                 Implementaion of Bit Error Rate test
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53EyeDiag.h"
#include "Utils/ContainerFactory.h"
#ifdef __POWERSUPPLY__
// Libraries
#include "Scope.h"
#include "ScopeAgilent.h"
#endif

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

EyeDiag::~EyeDiag() {}

void EyeDiag::ConfigureCalibration()
{
    // #######################
    // # Retrieve parameters #
    // #######################
    chain2test     = this->findValueInSettings<double>("chain2Test");
    given_time     = this->findValueInSettings<double>("byTime");
    frames_or_time = this->findValueInSettings<double>("framesORtime");
    doDisplay      = this->findValueInSettings<double>("DisplayHisto");

    if(fPowerSupplyClient == nullptr)
    {
        LOG(ERROR) << BOLDRED << "Not connected to the oscilloscope!!! EyeDiag cannot be executed" << RESET;
        throw std::runtime_error("RampPowerSupply cannot be executed");
    }
    std::stringstream observablesstring;
    for(auto it = observables.begin(); it != observables.end(); ++it)
    {
        observablesstring << *it;
        if(std::next(it) != observables.end()) observablesstring << "|";
    }
    fPowerSupplyClient->sendAndReceivePacket(std::string("Scope:main:setEOM=") + observablesstring.str());
}

void EyeDiag::Running()
{
    theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[EyeDiag::Running] Starting run: " << BOLDYELLOW << theCurrentRun << RESET;

    EyeDiag::run();
    EyeDiag::sendData();
}

void EyeDiag::sendData()
{
    // for (auto & obs : observables){
    //   histos[obs]->SetBinContent(1, fResult[obs].at(0));
    //   histos[obs]->SetBinError(1, fResult[obs].at(5));
    // }
}

void EyeDiag::Stop()
{
    LOG(INFO) << GREEN << "[EyeDiag::Stop] Stopping" << RESET;

    Tool::Stop();

    EyeDiag::draw();
    this->closeFileHandler();

    RD53RunProgress::reset();
}

void EyeDiag::localConfigure(const std::string fileRes_, int currentRun)
{
#ifdef __USE_ROOT__
    histos.clear();
#endif

    if(currentRun >= 0)
    {
        theCurrentRun = currentRun;
        LOG(INFO) << GREEN << "[EyeDiag::localConfigure] Starting run: " << BOLDYELLOW << theCurrentRun << RESET;
    }
    EyeDiag::ConfigureCalibration();
    this->CreateResultDirectory(RD53Shared::RESULTDIR, false, false);
    EyeDiag::initializeFiles(fileRes_, currentRun);
}

void EyeDiag::initializeFiles(const std::string fileRes_, int currentRun)
{
    fileRes = fileRes_;

#ifdef __USE_ROOT__
    for(auto& obs: observables) { histos[obs] = new TH1F(obs.c_str(), "", 1, 0, 1); }
#endif
}

void EyeDiag::run(std::string runName)
{
    std::unordered_map<std::string, std::string> kMeasurementMap = {
        {"Crossing %(cg)", "CROSsing"},
        {"Q-factor(cg)", "QFACtor"},
        {"Eye jit RMS(cg)", "JITTer RMS"},
        {"Eye height(cg)", "EHEight"},
        {"Eye width(cg)", "EWIDth"},
        {"Eye jit p-p(cg)", "JITTer PP"},
        {"rossing %(cg)", "CROSsing"},
        {"-factor(cg)", "QFACtor"},
        {"ye jit RMS(cg)", "JITTer RMS"},
        {"ye height(cg)", "EHEight"},
        {"ye width(cg)", "EWIDth"},
        {"ye jit p-p(cg)", "JITTer PP"},

    };

    ContainerFactory::copyAndInitChip<std::unordered_map<std::string, std::array<float, 7>>>(*fDetectorContainer, theEyeDiagContainer);

    for(const auto cBoard: *fDetectorContainer)
    {
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    std::unordered_map<std::string, std::array<float, 7>> value;
                    static_cast<RD53Interface*>(fReadoutChipInterface)->StartPRBSpattern(cBoard);
                    std::string results = fPowerSupplyClient->sendAndReceivePacket("Scope:main:acquireEOM=" + runName);

                    // now parse the output from the scope
                    std::vector<std::string> measurements;
                    boost::split(measurements, results, boost::is_any_of(","));
                    for(unsigned int i = 0; i < measurements.size() - 7; i = i + 8)
                    {
                        std::array<float, 7> obs_result;
                        for(int k = 1; k < 8; ++k) { obs_result.at(k - 1) = std::stof(measurements[i + k]); }
                        if(kMeasurementMap.find(measurements[i]) == kMeasurementMap.end())
                        {
                            std::stringstream error;
                            error << "Measurement " << measurements[i] << " not defiend as an available observable. Please add it to kMeasurementMap";
                            throw std::runtime_error(error.str());
                        }
                        else { value[kMeasurementMap[measurements.at(i)]] = obs_result; }
                    }
                    theEyeDiagContainer.getObject(cBoard->getId())
                        ->getObject(cOpticalGroup->getId())
                        ->getObject(cHybrid->getId())
                        ->getObject(cChip->getId())
                        ->getSummary<std::unordered_map<std::string, std::array<float, 7>>>() = value;
                    for(auto& obs: observables)
                    {
                        if(value[obs].at(1) != 0 && value[obs].at(1) != 1)
                        {
                            LOG(WARNING) << BOLDBLUE << "EyeMonitor did not converge. Error status " << value[obs].at(1) << std::endl;
                            continue;
                        }
                        if(value[obs].at(1) == 1) { LOG(WARNING) << BOLDBLUE << "EyeMonitor result for observable " << obs << " not reliable. Error status " << value[obs].at(1) << std::endl; }
                    }
                }
    }
}

void EyeDiag::draw()
{
#ifdef __USE_ROOT__
    this->InitResultFile(fileRes);

    for(auto& obs: observables) { fResultFile->WriteTObject(histos[obs]); }
    this->WriteRootFile();
    this->CloseResultFile();

#endif
}

void EyeDiag::fillHisto() {}
