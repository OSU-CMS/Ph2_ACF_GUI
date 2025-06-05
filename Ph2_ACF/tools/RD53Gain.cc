/*!OA
  \file                  RD53Gain.cc
  \brief                 Implementaion of Gain scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53Gain.h"

#include "Utils/ContainerSerialization.h"
#include <boost/multiprecision/number.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>

using namespace boost::numeric;
using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void Gain::ConfigureCalibration()
{
    // #######################
    // # Retrieve parameters #
    // #######################
    CalibBase::ConfigureCalibration();
    injType        = static_cast<RD53Shared::INJtype>(this->findValueInSettings<double>("INJtype"));
    startValue     = this->findValueInSettings<double>("VCalHstart");
    stopValue      = this->findValueInSettings<double>("VCalHstop");
    targetCharge   = RD53Shared::firstChip->Charge2VCal(this->findValueInSettings<double>("TargetCharge"));
    nSteps         = this->findValueInSettings<double>("VCalHnsteps", 1);
    offset         = this->findValueInSettings<double>("VCalMED");
    nHITxCol       = this->findValueInSettings<double>("nHITxCol");
    doOnlyNGroups  = this->findValueInSettings<double>("DoOnlyNGroups");
    doDisplay      = this->findValueInSettings<double>("DisplayHisto");
    doUpdateChip   = this->findValueInSettings<double>("UpdateChipCfg");
    saveBinaryData = this->findValueInSettings<double>("SaveBinaryData");
    frontEnd       = RD53Shared::firstChip->getFEtype(colStart, colStop);

    // ########################
    // # Custom channel group #
    // ########################
    auto groupType = CalibBase::assignGroupType(injType);
    theChnGroupHandler =
        std::make_shared<RD53ChannelGroupHandler>(rowStart, rowStop, colStart, colStop, RD53Shared::firstChip->getNRows(), RD53Shared::firstChip->getNCols(), groupType, nHITxCol, doOnlyNGroups);
    this->setChannelGroupHandler(theChnGroupHandler);

    // ##############################
    // # Initialize dac scan values #
    // ##############################
    const float step = (stopValue - startValue) / nSteps;
    for(auto i = 0u; i < nSteps; i++) dacList.push_back(startValue + step * i);

    // #################################
    // # Initialize container recycler #
    // #################################
    theRecyclingBin.setDetectorContainer(fDetectorContainer);

    // #######################
    // # Initialize progress #
    // #######################
    RD53RunProgress::total() += Gain::getNumberIterations();
}

void Gain::Running()
{
    CalibBase::theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[Gain::Running] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    if(saveBinaryData == true)
    {
        this->fDirectoryName = dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR;
        this->addFileHandler(std::string(fDirectoryName) + "/Run" + RD53Shared::fromInt2Str(CalibBase::theCurrentRun) + "_Gain.raw", 'w');
        this->initializeWriteFileHandler();
    }

    Gain::run();
    Gain::analyze();
    Gain::draw();
    Gain::sendData();
}

void Gain::sendData()
{
    if(fDQMStreamerEnabled)
    {
        if(theGainContainer != nullptr)
        {
            ContainerSerialization theGainSerialization("GainGain");
            theGainSerialization.streamByChipContainer(fDQMStreamer, *theGainContainer.get());
        }

        size_t                 index = 0;
        ContainerSerialization theOccupancySerialization("GainOccupancy");
        for(const auto theOccContainer: detectorContainerVector)
        {
            uint16_t deltaVcal = dacList[index++] - offset + (stopValue - startValue) / (2 * nSteps);
            theOccupancySerialization.streamByChipContainer(fDQMStreamer, *theOccContainer, deltaVcal);
        }
    }
}

void Gain::Stop()
{
    LOG(INFO) << GREEN << "[Gain::Stop] Stopping" << RESET;
    CalibBase::Stop();
}

void Gain::localConfigure(const std::string& histoFileName, int currentRun)
{
    // ############################
    // # CalibBase localConfigure #
    // ############################
    CalibBase::localConfigure(histoFileName, currentRun);

    histos = nullptr;

    LOG(INFO) << GREEN << "[Gain::localConfigure] Starting run: " << BOLDYELLOW << CalibBase::theCurrentRun << RESET;

    // ##########################
    // # Initialize calibration #
    // ##########################
    Gain::ConfigureCalibration();

    // ###############################
    // # Initialize output directory #
    // ###############################
    this->CreateResultDirectory(dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR, false, false);

    // #########################################
    // # Initialize histogram and binary files #
    // #########################################
    CalibBase::initializeFiles(histoFileName, "Gain", histos, currentRun, saveBinaryData);
}

void Gain::run()
{
    // ##########################
    // # Set new VCAL_MED value #
    // ##########################
    CalibBase::WriteBroadcastChipReg("VCAL_MED", offset);

    for(auto container: detectorContainerVector) theRecyclingBin.free(container);
    detectorContainerVector.clear();
    for(auto i = 0u; i < dacList.size(); i++) detectorContainerVector.push_back(theRecyclingBin.get(&ContainerFactory::copyAndInitStructure<OccupancyAndPh>, OccupancyAndPh()));

    auto groupType = CalibBase::assignGroupType(injType);
    this->SetBoardBroadcast(true);
    this->SetTestPulse(groupType != RD53GroupType::AllPixels);
    this->fMaskChannelsFromOtherGroups = true;
    this->scanDac("VCAL_HIGH", dacList, nEvents, detectorContainerVector);

    // #########################
    // # Mark enabled channels #
    // #########################
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                    for(auto row = 0u; row < RD53Shared::firstChip->getNRows(); row++)
                        for(auto col = 0u; col < RD53Shared::firstChip->getNCols(); col++)
                            if(!cChip->getChipOriginalMask()->isChannelEnabled(row, col) || !this->getChannelGroupHandlerContainer()
                                                                                                 ->getObject(cBoard->getId())
                                                                                                 ->getObject(cOpticalGroup->getId())
                                                                                                 ->getObject(cHybrid->getId())
                                                                                                 ->getObject(cChip->getId())
                                                                                                 ->getSummary<std::shared_ptr<ChannelGroupHandler>>()
                                                                                                 ->allChannelGroup()
                                                                                                 ->isChannelEnabled(row, col))
                                for(auto i = 0u; i < dacList.size(); i++)
                                    detectorContainerVector[i]
                                        ->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<OccupancyAndPh>(row, col)
                                        .fStatus = RD53Shared::ISDISABLED;

    // #################################
    // # Reset masks to default values #
    // #################################
    CalibBase::copyMaskFromDefault("en in");

    // ################
    // # Error report #
    // ################
    CalibBase::chipErrorReport();
}

void Gain::draw(bool saveData)
{
    if(saveData == true) CalibBase::saveChipRegisters(doUpdateChip);

#ifdef __USE_ROOT__
    TApplication* myApp = nullptr;

    if(doDisplay == true) myApp = new TApplication("myApp", nullptr, nullptr);

    CalibBase::bookHistoSaveMetadata(histos);
    Gain::fillHisto();
    histos->process();
    doSaveData = saveData;

    if(doDisplay == true) myApp->Run(true);
#endif

    // #####################
    // # @TMP@ : CalibFile #
    // #####################
    if(saveBinaryData == true) CalibBase::saveSCurveOrGaindValues(detectorContainerVector, dacList, offset, nEvents, "Gain");
}

std::shared_ptr<DetectorDataContainer> Gain::analyze()
{
    float highQslope, highQslopeErr, highQintercept, highQinterceptErr, lowQslope, lowQslopeErr, lowQintercept, lowQinterceptErr, chi2, DoF;

    std::vector<float> par(NGAINPAR, 0);
    std::vector<float> parErr(NGAINPAR, 0);

    std::vector<float> x(dacList.size(), 0);
    std::vector<float> y(dacList.size(), 0);
    std::vector<float> e(dacList.size(), 0);
    std::vector<float> o(dacList.size(), 0);

    theGainContainer = std::make_shared<DetectorDataContainer>();
    ContainerFactory::copyAndInitStructure<GainFit>(*fDetectorContainer, *theGainContainer);

    size_t index = 0;
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
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
                            {
                                for(auto i = 0u; i < dacList.size(); i++)
                                {
                                    x[i] = dacList[i] - offset;
                                    y[i] = detectorContainerVector[i]
                                               ->getObject(cBoard->getId())
                                               ->getObject(cOpticalGroup->getId())
                                               ->getObject(cHybrid->getId())
                                               ->getObject(cChip->getId())
                                               ->getChannel<OccupancyAndPh>(row, col)
                                               .fPh;
                                    e[i] = detectorContainerVector[i]
                                               ->getObject(cBoard->getId())
                                               ->getObject(cOpticalGroup->getId())
                                               ->getObject(cHybrid->getId())
                                               ->getObject(cChip->getId())
                                               ->getChannel<OccupancyAndPh>(row, col)
                                               .fPhError;
                                    o[i] = detectorContainerVector[i]
                                               ->getObject(cBoard->getId())
                                               ->getObject(cOpticalGroup->getId())
                                               ->getObject(cHybrid->getId())
                                               ->getObject(cChip->getId())
                                               ->getChannel<OccupancyAndPh>(row, col)
                                               .fOccupancy;
                                }

                                // ##################
                                // # Run regression #
                                // ##################
                                Gain::computeStats(x, y, e, o, par, parErr, chi2, DoF);
                                lowQintercept     = par[0];
                                lowQinterceptErr  = parErr[0];
                                lowQslope         = par[1];
                                lowQslopeErr      = parErr[1];
                                highQintercept    = par[2];
                                highQinterceptErr = parErr[2];
                                highQslope        = par[3];
                                highQslopeErr     = parErr[3];

                                if(chi2 == -1)
                                {
                                    theGainContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<GainFit>(row, col)
                                        .fChi2 = RD53Shared::ISFITERROR;
                                }
                                else
                                {
                                    theGainContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<GainFit>(row, col)
                                        .fInterceptHighQ = highQintercept;
                                    theGainContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<GainFit>(row, col)
                                        .fInterceptHighQError = highQinterceptErr;

                                    theGainContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<GainFit>(row, col)
                                        .fSlopeHighQ = highQslope;
                                    theGainContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<GainFit>(row, col)
                                        .fSlopeHighQError = highQslopeErr;

                                    theGainContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<GainFit>(row, col)
                                        .fInterceptLowQ = lowQintercept;
                                    theGainContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<GainFit>(row, col)
                                        .fInterceptLowQError = lowQinterceptErr;

                                    theGainContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<GainFit>(row, col)
                                        .fSlopeLowQ = lowQslope;
                                    theGainContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<GainFit>(row, col)
                                        .fSlopeLowQError = lowQslopeErr;

                                    theGainContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<GainFit>(row, col)
                                        .fChi2 = chi2;
                                    theGainContainer->getObject(cBoard->getId())
                                        ->getObject(cOpticalGroup->getId())
                                        ->getObject(cHybrid->getId())
                                        ->getObject(cChip->getId())
                                        ->getChannel<GainFit>(row, col)
                                        .fDoF = DoF;
                                }
                            }

                    index++;
                }

    theGainContainer->resetNormalizationStatus();
    theGainContainer->normalizeAndAverageContainers(fDetectorContainer, this->getChannelGroupHandlerContainer(), 1);

    for(const auto cBoard: *theGainContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    float ToTatTarget = Gain::gainFunction({cChip->getSummary<GainFit, GainFit>().fInterceptLowQ,
                                                            cChip->getSummary<GainFit, GainFit>().fSlopeLowQ,
                                                            cChip->getSummary<GainFit, GainFit>().fInterceptHighQ,
                                                            cChip->getSummary<GainFit, GainFit>().fSlopeHighQ},
                                                           targetCharge,
                                                           frontEnd);

                    if(ToTatTarget > frontEnd->maxToTvalue)
                        LOG(INFO) << GREEN << "Average ToT for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/"
                                  << +cChip->getId() << RESET << GREEN << "] at VCal = " << BOLDYELLOW << std::fixed << std::setprecision(2) << targetCharge << RESET << GREEN << " (" << BOLDYELLOW
                                  << RD53Shared::firstChip->VCal2Charge(targetCharge) << RESET << GREEN << " electrons) is greater than " << BOLDYELLOW << frontEnd->maxToTvalue << RESET << GREEN
                                  << " (ToT)" << std::setprecision(-1) << RESET;
                    else
                        LOG(INFO) << GREEN << "Average ToT for [board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/"
                                  << +cChip->getId() << RESET << GREEN << "] at VCal = " << BOLDYELLOW << std::fixed << std::setprecision(2) << targetCharge << RESET << GREEN << " (" << BOLDYELLOW
                                  << RD53Shared::firstChip->VCal2Charge(targetCharge) << RESET << GREEN << " electrons) is " << BOLDYELLOW << ToTatTarget << RESET << GREEN << " (ToT)"
                                  << std::setprecision(-1) << RESET;

                    RD53Shared::resetDefaultFloat();
                }

    return theGainContainer;
}

void Gain::fillHisto()
{
#ifdef __USE_ROOT__
    for(auto i = 0u; i < dacList.size(); i++) histos->fillOccupancy(*detectorContainerVector[i], dacList[i] - offset + (stopValue - startValue) / (2 * nSteps));
    histos->fillGain(*theGainContainer);
#endif
}

float Gain::gainFunction(const std::vector<float>& par, float VCal, const Ph2_HwDescription::RD53::FrontEnd* frontEnd)
// ############################################################
// # Given the input VCal returns the corresponding ToT value #
// ############################################################
{
    if(RD53Shared::firstChip->getUseGainDualSlope() == false)
        return par[0] + par[1] * VCal;
    else
    {
        if(VCal <= ((frontEnd->splitToTvalue - par[0]) / par[1]))
            return par[0] + par[1] * VCal;
        else
            return par[2] + par[3] * VCal;
    }
}

float Gain::gainInverseFunction(const std::vector<float>& par, float ToT, const Ph2_HwDescription::RD53::FrontEnd* frontEnd)
// ############################################################
// # Given the input ToT returns the corresponding VCal value #
// ############################################################
{
    if(ToT <= frontEnd->splitToTvalue)
        return (ToT - par[0]) / par[1];
    else
    {
        if(RD53Shared::firstChip->getUseGainDualSlope() == false)
            return (ToT - par[0]) / par[1];
        else
            return (ToT - par[2]) / par[3];
    }
}

void Gain::computeStats(const std::vector<float>& x,
                        const std::vector<float>& y,
                        const std::vector<float>& e,
                        const std::vector<float>& o,
                        std::vector<float>&       par,
                        std::vector<float>&       parErr,
                        float&                    chi2,
                        float&                    DoF)
// #######################################################
// # Linear regression with least-square method          #
// # Model for low charge range:  y = f(x) = [0] + [1]*x #
// # Model for high charge range: y = f(x) = [2] + [3]*x #
// # if chi2 = -1 --> fit problems                       #
// #######################################################
{
    // ##########################################
    // # Define and initialize needed variables #
    // ##########################################
    const int limitToT = (RD53Shared::firstChip->getUseGainDualSlope() == true ? frontEnd->splitToTvalue : frontEnd->maxToTvalue);
    chi2               = -1;
    for(auto i = 0; i < NGAINPAR; i++)
    {
        par[i]    = 0;
        parErr[i] = 0;
    }

    // ############################################
    // # Struct for ordering the vectors together #
    // ############################################
    struct ScanOutput
    {
        float x, y, e, o;
    };

    // ######################################################
    // # Order x, y, e and o together according to y values #
    // ######################################################
    std::vector<ScanOutput> scanOutputs;
    for(auto i = 0u; i < x.size(); i++)
        if((e[i] != 0) && (o[i] == 1)) scanOutputs.push_back({x[i], y[i], e[i], o[i]});
    std::sort(scanOutputs.begin(), scanOutputs.end(), [&](ScanOutput i, ScanOutput j) { return i.y < j.y; });

    // ###########################################
    // # Check to have enough points for the fit #
    // ###########################################
    const size_t nData = scanOutputs.size();
    DoF                = nData - NGAINPAR / 2;
    if(RD53Shared::firstChip->getUseGainDualSlope() == true)
    {
        const size_t nDataLowRange = std::count_if(scanOutputs.begin(), scanOutputs.end(), [&](ScanOutput val) { return val.y <= limitToT; });
        if(((nDataLowRange - NGAINPAR) < 1) || ((nData - nDataLowRange - NGAINPAR) < 1))
            return;
        else
            DoF = nData - NGAINPAR;
    }
    else if(DoF < 1)
        return;

    // ############################################
    // # Retreive oredered vectors for x, y and e #
    // ############################################
    std::vector<float> ordered_x;
    std::vector<float> ordered_y;
    std::vector<float> ordered_e;
    ordered_x.reserve(nData);
    ordered_y.reserve(nData);
    ordered_e.reserve(nData);
    for(auto& ele: scanOutputs)
    {
        ordered_x.push_back(ele.x);
        ordered_y.push_back(ele.y);
        ordered_e.push_back(ele.e);
    }

    // #############################################################################
    // # Find first y-element larger than limitToT which is the last true-ToT      #
    // # value where the gain slope does not change for the 6-to-4 bit compression #
    // #############################################################################
    auto         it            = std::find_if(ordered_y.begin(), ordered_y.end(), [&](float val) { return val > limitToT; });
    const size_t limitToTindex = it - ordered_y.begin();

    // ################################################
    // # Declare matrices and vector for minimization #
    // ################################################
    ublas::matrix<double> H(nData, NGAINPAR, 0);
    ublas::matrix<double> V(nData, nData, 0);
    ublas::vector<double> myY(nData);

    // ########################
    // # Declare columns of H #
    // ########################
    ublas::vector<double> col0(nData, 0);
    ublas::vector<double> col1(nData, 0);
    ublas::vector<double> col2(nData, 0);
    ublas::vector<double> col3(nData, 0);

    // #####################
    // # Fill columns of H #
    // #####################
    std::vector<double> ones(nData, 1);
    std::copy(ones.begin(), ones.begin() + limitToTindex, col0.begin());
    std::copy(ordered_x.begin(), ordered_x.begin() + limitToTindex, col1.begin());
    std::copy(ones.begin() + limitToTindex, ones.end(), col2.begin() + limitToTindex);
    std::copy(ordered_x.begin() + limitToTindex, ordered_x.end(), col3.begin() + limitToTindex);

    // #############
    // # Compose H #
    // #############
    column(H, 0) = col0;
    column(H, 1) = col1;
    column(H, 2) = col2;
    column(H, 3) = col3;

    // ##############################################################
    // # If single-gain slope, remove last two (empty) columns of H #
    // ##############################################################
    if(limitToTindex >= nData) H = ublas::project(H, ublas::range(0, nData), ublas::range(0, NGAINPAR / 2));

    // #############
    // # Compose V #
    // #############
    ublas::identity_matrix<double> identityMatrix(nData);
    ublas::vector<double>          identityVector(nData, 1);
    ublas::vector<double>          e2(ordered_e.size());
    std::copy(ordered_e.begin(), ordered_e.end(), e2.begin());
    std::transform(e2.begin(), e2.end(), e2.begin(), [](double x) { return x * x; });
    V = ublas::element_prod(ublas::outer_prod(identityVector, e2), identityMatrix);

    // ############
    // # Fill myY #
    // ############
    std::copy(ordered_y.begin(), ordered_y.end(), myY.begin());

    // ################
    // # Minimization #
    // ################
    auto invV(V);
    for(auto i = 0u; i < nData; i++) invV(i, i) = 1 / V(i, i);

    ublas::matrix<double> tmpMtx(ublas::prod(invV, H));
    ublas::matrix<double> invParCov(ublas::prod(ublas::trans(H), tmpMtx));
    auto                  parCov(invParCov);

    auto det = RD53Shared::mtxInversion<double>(invParCov, parCov);
    if((isnan(det) == false) && (det != 0))
    {
        ublas::vector<double> tmpVec1(ublas::prod(invV, myY));
        ublas::vector<double> tmpVec2(ublas::prod(ublas::trans(H), tmpVec1));
        ublas::vector<double> myPar(ublas::prod(parCov, tmpVec2));

        std::copy(myPar.begin(), myPar.end(), par.begin());
        for(auto i = 0u; i < NGAINPAR; i++) parErr[i] = (limitToTindex >= nData) && (i >= NGAINPAR / 2) ? 0.0 : sqrt(parCov(i, i));

        // ################
        // # Compute chi2 #
        // ################
        ublas::vector<double> num(myY - ublas::prod(H, myPar));
        ublas::vector<double> tmpNum(ublas::prod(invV, num));
        chi2 = ublas::inner_prod(num, tmpNum);
    }
}
