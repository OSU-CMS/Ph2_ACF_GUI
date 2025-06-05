#include "tools/OTverifyCICdataWord.h"
#include "HWDescription/BeBoard.h"
#include "HWInterface/CbcInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/PSInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"
#include "Utils/PatternMatcher.h"
#include "Utils/Utilities.h"
#include "tools/OTPatternCheckerHelper.h"
#include "tools/OTalignBoardDataWord.h"
#include <bitset>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTverifyCICdataWord::fCalibrationDescription = "Inject L1 and stubs for each CBC/MPA and verify that CIC output corresponds to the expected pattern";

OTverifyCICdataWord::OTverifyCICdataWord() : Tool() {}

OTverifyCICdataWord::~OTverifyCICdataWord() {}

void OTverifyCICdataWord::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any

    fNumberOfStubBits     = findValueInSettings<double>("OTverifyCICdataWord_NumberOfTestedStubBits", 1e5);
    fNumberOfL1Bits       = findValueInSettings<double>("OTverifyCICdataWord_NumberOfTestedL1Bits", 1e5);
    fDoMatchingInFirmware = findValueInSettings<double>("OTverifyCICdataWord_DoMatchingInFirmware", 1) > 0;
    fIsKickoff            = findValueInSettings<double>("isKickoff", 0) > 0;

    setUpPatternMatching();

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTverifyCICdataWord.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTverifyCICdataWord::setUpPatternMatching()
{
    GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2> theInitialBitAndError;
    for(size_t chipId = 0; chipId < NUMBER_OF_CIC_PORTS; ++chipId)
    {
        for(size_t lineId = 0; lineId < 2; ++lineId)
        {
            theInitialBitAndError.at(chipId).at(lineId).at(0) = 0.;
            theInitialBitAndError.at(chipId).at(lineId).at(1) = 0.;
        }
    }

    ContainerFactory::copyAndInitHybrid<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>>(*fDetectorContainer, fPatternMatchingEfficiencyContainer, theInitialBitAndError);

    fPatternCheckerHelper = new OTPatternCheckerHelper();
    fPatternCheckerHelper->Inherit(this);
    fPatternCheckerHelper->prepareCalibration();
    fPatternCheckerHelper->setSuppressErrorPrintout(true);
}

void OTverifyCICdataWord::ConfigureCalibration() {}

void OTverifyCICdataWord::Running()
{
    LOG(INFO) << "Starting OTverifyCICdataWord measurement.";
    Initialise();
    runIntegrityTest();
    fillHistograms();
    LOG(INFO) << "Done with OTverifyCICdataWord.";
    Reset();
}

void OTverifyCICdataWord::Stop(void)
{
    LOG(INFO) << "Stopping OTverifyCICdataWord measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTverifyCICdataWord.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTverifyCICdataWord stopped.";
}

void OTverifyCICdataWord::Pause() {}

void OTverifyCICdataWord::Resume() {}

void OTverifyCICdataWord::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTverifyCICdataWord::realignBoardDataWords()
{
    OTalignBoardDataWord theAlignBoardDataWord;
    theAlignBoardDataWord.Inherit(this);
    theAlignBoardDataWord.setProducePlots(false);
    theAlignBoardDataWord.Initialise();
    theAlignBoardDataWord.wordAlignBEdata();
    theAlignBoardDataWord.Reset();
}

void OTverifyCICdataWord::runIntegrityTest()
{
    LOG(INFO) << BOLDYELLOW << "OTverifyCICdataWord::runIntegrityTest ... start integrity test" << RESET;

    for(auto theBoard: *fDetectorContainer)
    {
        auto theFWInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard));
        runStubIntegrityTest(theBoard, theFWInterface);
        runL1IntegrityTest(theBoard, theFWInterface);
    }
}

void OTverifyCICdataWord::fillHistograms()
{
#ifdef __USE_ROOT__
    fDQMHistogramOTverifyCICdataWord.fillPatternMatchingEfficiencyResults(fPatternMatchingEfficiencyContainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization thePatternMatchinEfficiencyContainerSerialization("OTverifyCICdataWordPatternMatchingEfficiency");
        thePatternMatchinEfficiencyContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, fPatternMatchingEfficiencyContainer);
    }
#endif
}

void OTverifyCICdataWord::runL1IntegrityTest(BeBoard* theBoard, D19cFWInterface* theFWInterface)
{
    bool isA2Smodule = theBoard->getFirstObject()->getFrontEndType() == FrontEndType::OuterTracker2S; // only 1 module type per board

    LOG(INFO) << BOLDMAGENTA << "Running runL1IntegrityTest" << RESET;
    // // Set board trigger configuration for L1 alignment
    std::vector<std::pair<std::string, uint32_t>> cVecReg;
    cVecReg.push_back({"fc7_daq_cnfg.fast_command_block.triggers_to_accept", 0});
    cVecReg.push_back({"fc7_daq_cnfg.fast_command_block.misc.backpressure_enable", 0});
    cVecReg.push_back({"fc7_daq_cnfg.fast_command_block.user_trigger_frequency", 100});
    cVecReg.push_back({"fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", 0});
    cVecReg.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    cVecReg.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", 0x0});
    cVecReg.push_back({"fc7_daq_cnfg.readout_block.global.data_handshake_enable", 0x1});
    fBeBoardInterface->WriteBoardMultReg(theBoard, cVecReg);

    for(auto theOpticalGroup: *theBoard)
    {
        LOG(INFO) << BOLDMAGENTA << "    Optical Group " << +theOpticalGroup->getId() << RESET;
        uint8_t numberOfBytesInSinglePacket = (static_cast<D19clpGBTInterface*>(flpGBTInterface)->GetChipRate(theOpticalGroup->flpGBT) == 10) ? 2 : 1;
        for(auto theHybrid: *theOpticalGroup)
        {
            LOG(INFO) << BOLDMAGENTA << "        Hybrid " << +theHybrid->getId() << RESET;
            auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
            fCicInterface->SetSparsification(cCic, true);

            fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select", theHybrid->getId());
            fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.physical_interface_block.slvs_debug.chip_select", 0);
            auto theChipToCICMapping = cCic->getMapping();

            for(auto theChip: *theHybrid)
            {
                if(theChip->getFrontEndType() == FrontEndType::SSA2) continue;
                auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fCicInterface->EnableFEs(cCic, {0, 1, 2, 3, 4, 5, 6, 7}, false);
                fCicInterface->SelectOutput(cCic, false);

                uint8_t chipIdForCIC = theChipToCICMapping.at(theChip->getId() % 8);
                fCicInterface->EnableFEs(cCic, {uint8_t(theChip->getId() % 8)}, true);
                PatternMatcher thePatternMatcher;
                if(isA2Smodule)
                    thePatternMatcher = injectL12S(theChip, chipIdForCIC, theFWInterface, numberOfBytesInSinglePacket);
                else
                    thePatternMatcher = injectL1PS(theChip, chipIdForCIC, theFWInterface, numberOfBytesInSinglePacket);

                runL1Interations(theFWInterface, thePatternMatcher, theChip, numberOfBytesInSinglePacket);
            }
        }
    }
}

GenericDataArray<float, 2>& OTverifyCICdataWord::getStorageForL1ErrorRate(Chip* theChip)
{
    return fPatternMatchingEfficiencyContainer.getObject(theChip->getBeBoardId())
        ->getObject(theChip->getOpticalGroupId())
        ->getObject(theChip->getHybridId())
        ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>>()
        .at(theChip->getId() % 8)
        .at(0);
}

PatternMatcher OTverifyCICdataWord::injectL12S(ReadoutChip* theChip, uint8_t chipIdForCIC, D19cFWInterface* theFWInterface, uint8_t numberOfBytesInSinglePacket)
{
    LOG(INFO) << BOLDBLUE << "            injecting clusters on CBC Id " << +theChip->getId() << RESET;

    std::vector<std::pair<uint8_t, uint8_t>> theClusterList;

    for(uint16_t clusterNumber = 0; clusterNumber < 31; ++clusterNumber) { theClusterList.push_back(std::make_pair<uint8_t, uint8_t>(clusterNumber * 6, 2)); }

    fReadoutChipInterface->WriteChipReg(theChip, "HitOr", 1);
    static_cast<CbcInterface*>(fReadoutChipInterface)->selectLogicMode(theChip, "Sampled", true, true);
    static_cast<CbcInterface*>(fReadoutChipInterface)->injectClusters(theChip, theClusterList);

    uint8_t numberOfClusters = theClusterList.size();

    PatternMatcher thePatternMatcher;
    thePatternMatcher.addToPattern(0x0ffffffe, 0xffffffff, 32); // CIC header plus 0 in front added in the transmission
    thePatternMatcher.addToPattern(0x0, 0x1ff, 9);
    thePatternMatcher.addToPattern(0x0, 0x0, 9);
    thePatternMatcher.addToPattern(numberOfClusters, 0x7F, 7);
    thePatternMatcher.addToPattern(0x0, 0x1, 1);

    std::map<uint8_t, uint8_t> orderedClusterList;
    for(const auto& theCluster: theClusterList) { orderedClusterList[theCluster.first] = theCluster.second; }

    // CIC ouputs cluster with loower address first
    for(const auto& theCluster: orderedClusterList)
    {
        thePatternMatcher.addToPattern(chipIdForCIC, 0x7, 3);
        thePatternMatcher.addToPattern(theCluster.first, 0xFF, 8);
        thePatternMatcher.addToPattern(theCluster.second - 1, 0x7, 3);
    }

    return thePatternMatcher;
}

PatternMatcher OTverifyCICdataWord::injectL1PS(ReadoutChip* theMPA, uint8_t chipIdForCIC, D19cFWInterface* theFWInterface, uint8_t numberOfBytesInSinglePacket)
{
    LOG(INFO) << BOLDBLUE << "            injecting clusters on MPA Id " << +theMPA->getId() << RESET;

    std::vector<Cluster> theClusterList;

    for(uint16_t clusterNumber = 0; clusterNumber < 31; ++clusterNumber) { theClusterList.push_back(Cluster(clusterNumber / 2, clusterNumber * 3, 2)); }

    static_cast<PSInterface*>(fReadoutChipInterface)->injectNoiseClusters(theMPA, theClusterList);
    try
    {
        ReadoutChip* theSSA = fDetectorContainer->getObject(theMPA->getBeBoardId())->getObject(theMPA->getOpticalGroupId())->getObject(theMPA->getHybridId())->getObject(theMPA->getId() - 8);
        fReadoutChipInterface->MaskAllChannels(theSSA, true);
    }
    catch(const std::exception& e)
    {
        // do nothing, matching SSA is disabled
    }

    uint8_t numberOfPixelClusters = theClusterList.size();
    uint8_t numberOfStripClusters = 0;

    // Create expected pattern
    PatternMatcher thePatternMatcher;
    thePatternMatcher.addToPattern(0x0ffffffe, 0xffffffff, 32); // CIC header plus 0 in front added in the transmission
    thePatternMatcher.addToPattern(0x0, 0x1ff, 9);
    thePatternMatcher.addToPattern(0x0, 0x0, 9);
    thePatternMatcher.addToPattern(numberOfStripClusters, 0x7F, 7);
    thePatternMatcher.addToPattern(0x0, 0x1, 1);
    thePatternMatcher.addToPattern(numberOfPixelClusters, 0x7F, 7);

    std::map<uint8_t, std::pair<uint8_t, uint8_t>> orderedClusterList;
    for(const auto& theCluster: theClusterList) { orderedClusterList[theCluster.fFirstCol] = {theCluster.fRow, theCluster.fColWidth}; }

    // CIC ouputs cluster with loower address first
    for(const auto& theCluster: orderedClusterList)
    {
        thePatternMatcher.addToPattern(chipIdForCIC, 0x7, 3);
        thePatternMatcher.addToPattern(theCluster.first + 1, 0x7F, 7); // pixel column address starts from 1
        thePatternMatcher.addToPattern(theCluster.second.second - 1, 0x7, 3);
        thePatternMatcher.addToPattern(theCluster.second.first, 0xF, 4);
    }

    return thePatternMatcher;
}

void OTverifyCICdataWord::runL1Interations(Ph2_HwInterface::D19cFWInterface* theFWInterface,
                                           PatternMatcher&                   thePatternMatcher,
                                           Ph2_HwDescription::ReadoutChip*   theChip,
                                           uint8_t                           numberOfBytesInSinglePacket)
{
    float testedBitNumber = thePatternMatcher.getNumberOfMaskedBits();
    if(testedBitNumber == 0) return;

    auto&  theL1Efficiency         = getStorageForL1ErrorRate(theChip);
    size_t numberOfIterations      = std::ceil(fNumberOfL1Bits / testedBitNumber);
    size_t numberOfIgnoredPatterns = 0;
    for(size_t iteration = 0; iteration < numberOfIterations;)
    {
        auto lineOutputVector        = theFWInterface->L1ADebug(1, false);
        auto orderedLineOutputVector = reorderPattern(lineOutputVector, numberOfBytesInSinglePacket);
        // std::cout << "L1 Line -> " << getPatternPrintout(orderedLineOutputVector, numberOfBytesInSinglePacket) << std::endl;
        float errorBitNumber = testedBitNumber - thePatternMatcher.countMatchingBits(orderedLineOutputVector);
        if(errorBitNumber > 0)
        {
            if(std::all_of(orderedLineOutputVector.begin(), orderedLineOutputVector.end(), [](int i) { return i == 0; }))
            {
                ++numberOfIgnoredPatterns;
                continue;
            }
            size_t numberOfEmpyWords = 0;
            for(auto theWord: orderedLineOutputVector)
            {
                if(theWord == 0) ++numberOfEmpyWords;
            }
            if(numberOfIgnoredPatterns < numberOfIterations / 10 && numberOfEmpyWords > 1)
            {
                ++numberOfIgnoredPatterns;
                continue; // means very likely the fifo did not save properly the data
            }
            LOG(DEBUG) << BOLDRED << "runL1Interations - Error, expected L1 pattern not found for Board " << +theChip->getBeBoardId() << " OpticalGroup " << +theChip->getOpticalGroupId() << " Hybrid "
                       << +theChip->getHybridId() << " " << FrontEndDescription::getFrontEndName(theChip->getFrontEndType()) << " " << +theChip->getId() << " on iteration number " << +iteration
                       << RESET;
            LOG(DEBUG) << BOLDRED << "L1 data received    " << getPatternPrintout(orderedLineOutputVector, numberOfBytesInSinglePacket) << RESET;
            LOG(DEBUG) << BOLDRED << "L1 pattern expected " << getPatternPrintout(thePatternMatcher.getPattern(), numberOfBytesInSinglePacket) << RESET;
            LOG(DEBUG) << BOLDRED << "L1 pattern mask     " << getPatternPrintout(thePatternMatcher.getMask(), numberOfBytesInSinglePacket) << RESET;
        }
        theL1Efficiency.at(0) += testedBitNumber;
        theL1Efficiency.at(1) += errorBitNumber;
        iteration++;
    }
    return;
}

uint8_t OTverifyCICdataWord::prepareCICforStubIntegrityTest(Hybrid* theHybrid, uint8_t chipId)
{
    auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
    fCicInterface->SelectOutput(cCic, false);
    auto theFeConfigRegisterValue = fCicInterface->ReadChipReg(cCic, "FE_CONFIG");
    theFeConfigRegisterValue |= 0x04; // Force bending to be sent out in the stub stream
    fCicInterface->WriteChipReg(cCic, "FE_CONFIG", theFeConfigRegisterValue);
    auto theChipToCICMapping = cCic->getMapping();
    fCicInterface->EnableFEs(cCic, {0, 1, 2, 3, 4, 5, 6, 7}, false);
    uint8_t chipIdForCIC = theChipToCICMapping.at(chipId);
    fCicInterface->EnableFEs(cCic, {uint8_t(chipId)}, true);

    return chipIdForCIC;
}

void OTverifyCICdataWord::injectStubsPS(Ph2_HwDescription::ReadoutChip* theMPA, uint8_t numberOfBytesInSinglePacket, const std::vector<Stub>& listOfStubs)
{
    if(listOfStubs.size() > 1) // CIC aligns stubs by bending, but in pixel-pixel mode bending is 0 and it is not possible to know what the CIC will drop
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] PS stub injected using pixel-pixel mode, consistent injections require maximum 1 stub per BX!" << std::endl;
        abort();
    }

    const auto&          theStub     = listOfStubs.at(0);
    uint8_t              clusterSeed = theStub.fSeed / 2 - 1;
    uint8_t              clusterSize = theStub.fSeed % 2 + 1;
    uint8_t              zPosition   = theStub.fZ;
    uint8_t              theBending  = theStub.fBend;
    std::vector<Cluster> theClusterList{Cluster(zPosition, clusterSeed, clusterSize)};

    static_cast<PSInterface*>(fReadoutChipInterface)->injectNoiseClusters(theMPA, theClusterList);
    fReadoutChipInterface->WriteChipReg(theMPA, "StubMode", 2); // Use pixel mode to exclude possible SSA communication issues
    fReadoutChipInterface->WriteChipReg(theMPA, "StubWindow", 31);
    fReadoutChipInterface->WriteChipReg(theMPA, "CodeM10", theBending); // bending = 0 will ouput theBending
}

void OTverifyCICdataWord::injectStubs2S(Ph2_HwDescription::ReadoutChip* theCBC, const std::vector<Stub>& listOfStubs)
{
    std::vector<std::pair<std::string, uint16_t>> theRegisterVector;
    std::vector<std::pair<uint8_t, int>>          stubSeedAndBending;
    int                                           bendingCode = 0;
    for(const auto& theStub: listOfStubs)
    {
        stubSeedAndBending.push_back(std::make_pair(theStub.fSeed, bendingCode * 2));
        theRegisterVector.push_back({"Bend" + std::to_string(7 + bendingCode), theStub.fBend});
        ++bendingCode;
    }

    fReadoutChipInterface->WriteChipReg(theCBC, "PtCut", 14);
    fReadoutChipInterface->WriteChipReg(theCBC, "ClusterCut", 4);
    static_cast<CbcInterface*>(fReadoutChipInterface)->selectLogicMode(theCBC, "Sampled", true, true);
    theRegisterVector.push_back({"CoincWind&Offset12", 0x00}); // set stub window offset to 0
    theRegisterVector.push_back({"CoincWind&Offset34", 0x00}); // set stub window offset to 0
    fReadoutChipInterface->WriteChipMultReg(theCBC, theRegisterVector);

    // inject stubs on CBC to CIC stub lines 0 (first stub address) lines 1 (second stub address), line 3 (first and second stub bend)
    fReadoutChipInterface->MaskAllChannels(theCBC, true);
    static_cast<CbcInterface*>(fReadoutChipInterface)->injectStubs(theCBC, stubSeedAndBending);
}

PatternMatcher OTverifyCICdataWord::producePatternMatcher2S(uint8_t chipIdForCIC, uint8_t hybridId, const std::vector<Stub>& listOfStubs)
{
    uint8_t maximumStubNumber = 16;

    // Order stub by bending
    std::map<uint8_t, uint8_t> orderedStubBendingCodeAndSeedVector;
    for(const auto& stubSeedAndBending: listOfStubs) { orderedStubBendingCodeAndSeedVector[stubSeedAndBending.fBend] = stubSeedAndBending.fSeed; }
    if(orderedStubBendingCodeAndSeedVector.size() != listOfStubs.size())
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] orderedStubBendingCodeAndSeedVector and stubSeedAndBendingVector sizes to not match!" << std::endl;
        abort();
    }

    uint8_t  numberOfStubs = listOfStubs.size() * 8; // times 8 because the packet contains stubs from 8 BXs
    uint16_t status        = 0x0;
    if(numberOfStubs > maximumStubNumber)
    {
        status        = 0x1;
        numberOfStubs = maximumStubNumber;
    }

    PatternMatcher thePattern;
    thePattern.addToPattern(0x0, 0x1, 1);      // is PS flag
    thePattern.addToPattern(status, 0x1FF, 9); // status bits
    thePattern.addToPattern(0x000, 0x000, 12); // Bx ID
    thePattern.addToPattern(numberOfStubs, 0x3F, 6);

    size_t totalNumberOfStubs = 0;
    for(auto theStub: orderedStubBendingCodeAndSeedVector)
    {
        for(uint8_t bxOffset = 0; bxOffset < 8; ++bxOffset)
        {
            thePattern.addToPattern(0x0, totalNumberOfStubs == 0 ? 0x1 : 0x0, 3); // BX offset
            thePattern.addToPattern(chipIdForCIC, 0x7, 3);                        // Chip ID
            thePattern.addToPattern(theStub.second, 0xFF, 8);                     // seed
            thePattern.addToPattern(theStub.first, 0xF, 4);                       // bending
            ++totalNumberOfStubs;
            if(totalNumberOfStubs >= maximumStubNumber) break;
        }
        if(totalNumberOfStubs >= maximumStubNumber) break;
    }

    thePattern.addTrailingZeros(64 * 5);

    if(fIsKickoff && hybridId % 2 == 0) thePattern.maskStubFor2Skickoff();

    return thePattern;
}

PatternMatcher OTverifyCICdataWord::producePatternMatcherPS(uint8_t chipIdForCIC, uint8_t numberOfBytesInSinglePacket, const std::vector<Stub>& listOfStubs)
{
    size_t numberOfStubs = 8 * listOfStubs.size();

    PatternMatcher thePattern;
    thePattern.addToPattern(0x1, 0x1, 1);      // is PS flag
    thePattern.addToPattern(0x0, 0x1FF, 9);    // status bits
    thePattern.addToPattern(0x000, 0x000, 12); // Bx ID
    thePattern.addToPattern(numberOfStubs, 0x3F, 6);

    for(uint8_t bxOffset = 0; bxOffset < 8; ++bxOffset)
    {
        for(auto theStub: listOfStubs)
        {
            thePattern.addToPattern(0x0, 0x0, 3);            // BX offset
            thePattern.addToPattern(chipIdForCIC, 0x7, 3);   // Chip ID
            thePattern.addToPattern(theStub.fSeed, 0xFF, 8); // seed
            thePattern.addToPattern(theStub.fBend, 0x7, 3);  // bending
            thePattern.addToPattern(theStub.fZ, 0xF, 4);     // z
        }
    }

    thePattern.addTrailingZeros(numberOfBytesInSinglePacket * 64 * 6);

    return thePattern;
}

void OTverifyCICdataWord::runStubInterationsSoftwareMatching(D19cFWInterface*    theFWInterface,
                                                             BoardDataContainer& thePatternContainer,
                                                             BeBoard*            theBoard,
                                                             uint8_t             numberOfBytesInSinglePacket,
                                                             uint8_t             chipId,
                                                             size_t              stubPatternCounter)
{
    fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.physical_interface_block.slvs_debug.chip_select", 0);

    for(auto theOpticalGroup: *theBoard)
    {
        bool is2Smodule = theOpticalGroup->getFrontEndType() == FrontEndType::OuterTracker2S;
        for(auto theHybrid: *theOpticalGroup)
        {
            ReadoutChip* theChip;
            try
            {
                theChip = theHybrid->getObject(chipId + (is2Smodule ? 0 : 8));
            }
            catch(const std::exception& e)
            {
                continue;
            }
            auto& thePatternMatcher = thePatternContainer.getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<PatternMatcher>();

            auto& theStubEfficiency = getStorageForStubErrorRate(theHybrid, chipId, 0, stubPatternCounter);

            fBeBoardInterface->WriteBoardReg(fDetectorContainer->getObject(theBoard->getId()), "fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select", theHybrid->getId());

            uint8_t numberOfLines      = is2Smodule ? 5 : 6;
            float   testedBitNumber    = thePatternMatcher.getNumberOfMaskedBits();
            size_t  numberOfIterations = std::ceil(fNumberOfStubBits / testedBitNumber);
            for(size_t iteration = 0; iteration < numberOfIterations; ++iteration)
            {
                auto                  lineOutputVector        = theFWInterface->StubDebug(true, numberOfLines, false);
                std::vector<uint32_t> concatenatedStubPackage = mergeCICStubOuput(lineOutputVector, numberOfBytesInSinglePacket);
                float                 testedBitNumber         = thePatternMatcher.getNumberOfMaskedBits();
                float                 errorBitNumber          = testedBitNumber - matchStubPattern(concatenatedStubPackage, thePatternMatcher, numberOfBytesInSinglePacket, numberOfLines);
                theStubEfficiency.at(0) += testedBitNumber;
                theStubEfficiency.at(1) += errorBitNumber;
                if(errorBitNumber > 0)
                {
                    LOG(DEBUG) << BOLDRED << "OTverifyCICdataWord::runStubInterations - Error, expected stub pattern not found for Board " << +theBoard->getId() << " OpticalGroup "
                               << +theOpticalGroup->getId() << " Hybrid " << +theHybrid->getId() << " " << FrontEndDescription::getFrontEndName(theChip->getFrontEndType()) << " " << +theChip->getId()
                               << RESET;
                    LOG(DEBUG) << BOLDRED << "Stub data received    " << getPatternPrintout(concatenatedStubPackage, numberOfBytesInSinglePacket) << RESET;
                    LOG(DEBUG) << BOLDRED << "Stub pattern expected " << getPatternPrintout(thePatternMatcher.getPattern(), numberOfBytesInSinglePacket) << RESET;
                    LOG(DEBUG) << BOLDRED << "Stub pattern mask     " << getPatternPrintout(thePatternMatcher.getMask(), numberOfBytesInSinglePacket) << RESET;
                }
                // LOG(INFO) << BOLDRED << "Stub data received    " << getPatternPrintout(concatenatedStubPackage, numberOfBytesInSinglePacket) << RESET;
                // LOG(INFO) << BOLDRED << "Stub pattern expected " << getPatternPrintout(thePatternMatcher.getPattern(), numberOfBytesInSinglePacket) << RESET;
                // LOG(INFO) << BOLDRED << "Stub pattern mask     " << getPatternPrintout(thePatternMatcher.getMask(), numberOfBytesInSinglePacket) << RESET;
            }
        }
    }
}

void OTverifyCICdataWord::runStubInterationsFirmwareMatching(BoardDataContainer& thePatternContainer, BeBoard* theBoard, uint8_t chipId, uint8_t numberOfLines, bool is10G, size_t stubPatternCounter)
{
    std::map<uint8_t, BoardDataContainer> thePatternAndMaskContainerMap;
    std::map<uint8_t, BoardDataContainer> theAlignmentPatternAndMaskContainerMap;
    for(uint8_t line = 0; line < numberOfLines; ++line)
    {
        ContainerFactory::copyAndInitHybrid<std::pair<std::vector<uint32_t>, std::vector<uint32_t>>>(*theBoard, thePatternAndMaskContainerMap[line]);
        ContainerFactory::copyAndInitHybrid<std::pair<uint16_t, uint16_t>>(*theBoard, theAlignmentPatternAndMaskContainerMap[line]);
    }
    // split pattern in lines
    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            const auto& thePattern = thePatternContainer.getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<PatternMatcher>();

            auto thePatternVector        = thePattern.getPattern();
            auto thePatternVectorPerLine = splitBits(thePatternVector, numberOfLines);

            auto thePatternMask              = thePattern.getMask();
            auto thePatternMaskVectorPerLine = splitBits(thePatternMask, numberOfLines);

            auto theAlignmentPattern(thePattern);
            theAlignmentPattern.updatePattern(0xffc, 0xfff, 12, 11);

            auto theAlignmentPatternVector        = theAlignmentPattern.getPattern();
            auto theAlignmentPatternVectorPerLine = splitBits(theAlignmentPatternVector, numberOfLines);

            auto theAlignmentPatternMask              = theAlignmentPattern.getMask();
            auto theAlignmentPatternMaskVectorPerLine = splitBits(theAlignmentPatternMask, numberOfLines);

            for(uint8_t line = 0; line < numberOfLines; ++line)
            {
                auto& thePatternAndMask =
                    thePatternAndMaskContainerMap[line].getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<std::pair<std::vector<uint32_t>, std::vector<uint32_t>>>();
                thePatternAndMask.first  = thePatternVectorPerLine.at(line);
                thePatternAndMask.second = thePatternMaskVectorPerLine.at(line);
                if(!is10G)
                {
                    thePatternAndMask.first.insert(thePatternAndMask.first.end(), thePatternVectorPerLine.at(line).begin(), thePatternVectorPerLine.at(line).end());
                    thePatternAndMask.second.insert(thePatternAndMask.second.end(), thePatternMaskVectorPerLine.at(line).begin(), thePatternMaskVectorPerLine.at(line).end());
                }

                auto& theAlignmentPatternAndMask  = theAlignmentPatternAndMaskContainerMap[line].getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<std::pair<uint16_t, uint16_t>>();
                theAlignmentPatternAndMask.first  = theAlignmentPatternVectorPerLine.at(line).at(0) >> 16;
                theAlignmentPatternAndMask.second = theAlignmentPatternMaskVectorPerLine.at(line).at(0) >> 16;
            }
        }
    }

    for(uint8_t line = 0; line < numberOfLines; ++line)
    {
        BoardDataContainer thePatternCounterCountainer;
        ContainerFactory::copyAndInitHybrid<GenericDataArray<uint64_t, 2>>(*theBoard, thePatternCounterCountainer);
        fPatternCheckerHelper->patternCheckerTest(
            &thePatternCounterCountainer, line + 1, thePatternAndMaskContainerMap[line], fNumberOfStubBits / numberOfLines, false, &theAlignmentPatternAndMaskContainerMap[line]);
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto& theStubEfficiency = getStorageForStubErrorRate(theHybrid, chipId, line, stubPatternCounter);

                const auto& theRecorderdErroInfo = thePatternCounterCountainer.getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<GenericDataArray<uint64_t, 2>>();
                theStubEfficiency.at(0) += theRecorderdErroInfo.at(0);
                theStubEfficiency.at(1) += theRecorderdErroInfo.at(1);
            }
        }
    }
}

GenericDataArray<float, 2>& OTverifyCICdataWord::getStorageForStubErrorRate(Hybrid* theHybrid, uint8_t chipId, uint8_t line, size_t stubPatternCounter)
{
    return fPatternMatchingEfficiencyContainer.getHybrid(theHybrid->getBeBoardId(), theHybrid->getOpticalGroupId(), theHybrid->getId())
        ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>>()
        .at(chipId)
        .at(1);
}

float OTverifyCICdataWord::matchStubPattern(const std::vector<uint32_t>& theWordVector, const PatternMatcher& thePatternMatcher, uint8_t numberOfBytesInSinglePacket, size_t numberOfLines)
{
    float maxNumberOfMatchedBits = 0;
    for(uint8_t numberOfPacketsToSkip = 0; numberOfPacketsToSkip < numberOfLines * 8; ++numberOfPacketsToSkip)
    {
        std::vector<uint32_t> theShiftedWordVector       = applyByteShift(theWordVector, numberOfBytesInSinglePacket, numberOfPacketsToSkip);
        float                 currentNumberOfMatchedBits = thePatternMatcher.countMatchingBits(theShiftedWordVector);
        if(currentNumberOfMatchedBits > maxNumberOfMatchedBits)
        {
            maxNumberOfMatchedBits = currentNumberOfMatchedBits;
            if(thePatternMatcher.isMatched(theShiftedWordVector)) break;
        }
    }
    return maxNumberOfMatchedBits;
}

std::vector<uint32_t> OTverifyCICdataWord::mergeCICStubOuput(const std::vector<std::vector<uint32_t>>& stubLineDataList, uint8_t numberOfBytesInSinglePacket)
{
    std::vector<std::vector<uint32_t>> orderedStubLineDataList;

    for(const auto& stubLineData: stubLineDataList) { orderedStubLineDataList.push_back(reorderPattern(stubLineData, numberOfBytesInSinglePacket)); }

    // all stublines have the same number of bits
    size_t numberOfBitsPerWord                    = 8 * sizeof(uint32_t);
    size_t numberOfBitsPerStubLine                = orderedStubLineDataList.at(0).size() * numberOfBitsPerWord;
    size_t numberOfBitsInConcatenatedStubPackage  = numberOfBitsPerStubLine * orderedStubLineDataList.size();
    size_t numberOfWordsInConcatenatedStubPackage = numberOfBitsInConcatenatedStubPackage / numberOfBitsPerWord;
    if(numberOfBitsInConcatenatedStubPackage % numberOfBitsPerWord != 0) ++numberOfWordsInConcatenatedStubPackage;
    std::vector<uint32_t> concatenatedStubPackage(numberOfWordsInConcatenatedStubPackage);

    size_t currentConcatenatedBit = 0;

    for(size_t bitNumber = 0; bitNumber < numberOfBitsPerStubLine; ++bitNumber)
    {
        for(const auto& stubLineData: orderedStubLineDataList)
        {
            uint32_t stubBit = (stubLineData.at(bitNumber / numberOfBitsPerWord) >> (numberOfBitsPerWord - 1 - bitNumber % numberOfBitsPerWord)) & 0x1;
            concatenatedStubPackage.at(currentConcatenatedBit / numberOfBitsPerWord) |= (stubBit << (numberOfBitsPerWord - 1 - currentConcatenatedBit % numberOfBitsPerWord));
            ++currentConcatenatedBit;
        }
    }

    return concatenatedStubPackage;
}

std::vector<std::vector<Stub>> OTverifyCICdataWord::createPSstubList()
{
    std::vector<std::vector<Stub>> stubInformationList;
    std::vector<Stub>              theStubList{Stub(0xAA, 0x5, 0xA)};
    stubInformationList.push_back(theStubList);
    return stubInformationList;
}

std::vector<std::vector<Stub>> OTverifyCICdataWord::create2SstubList()
{
    std::vector<std::vector<Stub>> stubInformationList;

    std::vector<Stub> stubSeedAndBendingVectorFirstPattern;
    stubSeedAndBendingVectorFirstPattern.push_back(Stub(0x5A, 0x9, 0x0));
    stubSeedAndBendingVectorFirstPattern.push_back(Stub(0xA0, 0xB, 0x0));
    stubSeedAndBendingVectorFirstPattern.push_back(Stub(0xAA, 0xF, 0x0));
    stubInformationList.push_back(stubSeedAndBendingVectorFirstPattern);

    std::vector<Stub> stubSeedAndBendingVectorSecondPattern;
    stubSeedAndBendingVectorSecondPattern.push_back(Stub(0xA0, 0x9, 0x0));
    stubSeedAndBendingVectorSecondPattern.push_back(Stub(0xAA, 0xB, 0x0));
    stubSeedAndBendingVectorSecondPattern.push_back(Stub(0x0A, 0xF, 0x0));
    stubInformationList.push_back(stubSeedAndBendingVectorSecondPattern);

    return stubInformationList;
}

void OTverifyCICdataWord::runStubIntegrityTest(BeBoard* theBoard, D19cFWInterface* theFWInterface)
{
    LOG(INFO) << BOLDMAGENTA << "Running runStubIntegrityTest" << RESET;

    prepareForStubInjection(theBoard);

    uint8_t                        numberOfLines;
    bool                           is2Smodule = theBoard->getFirstObject()->getFrontEndType() == FrontEndType::OuterTracker2S;
    bool                           is10G      = (static_cast<D19clpGBTInterface*>(flpGBTInterface)->GetChipRate(theBoard->getFirstObject()->flpGBT) == 10);
    std::vector<std::vector<Stub>> stubInformationList;
    uint8_t                        numberOfBytesInSinglePacket;
    if(is2Smodule)
    {
        stubInformationList         = create2SstubList();
        numberOfLines               = 5;
        numberOfBytesInSinglePacket = 1;
    }
    else
    {
        stubInformationList         = createPSstubList();
        numberOfLines               = 6;
        numberOfBytesInSinglePacket = is10G ? 2 : 1;
    }

    size_t stubPatternCounter = 0;
    size_t numberOfPatterns   = stubInformationList.size();
    for(auto theStubList: stubInformationList)
    {
        LOG(INFO) << BOLDMAGENTA << "    Stub pattern " << stubPatternCounter + 1 << " out of " << numberOfPatterns << RESET;
        for(size_t chipId = 0; chipId < NUMBER_OF_CIC_PORTS; ++chipId)
        {
            LOG(INFO) << BOLDBLUE << "        Injecting stubs on " << (is2Smodule ? "CBC" : "MPA") << " " << chipId + (is2Smodule ? 0 : 8) << " for all hybrids" << RESET;
            BoardDataContainer thePatternMatcherContainer;
            ContainerFactory::copyAndInitHybrid<PatternMatcher>(*theBoard, thePatternMatcherContainer);

            for(auto theOpticalGroup: *theBoard)
            {
                for(auto theHybrid: *theOpticalGroup)
                {
                    uint8_t chipIdForCIC = prepareCICforStubIntegrityTest(theHybrid, chipId);

                    auto& thePattern = thePatternMatcherContainer.getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<PatternMatcher>();
                    if(is2Smodule)
                        thePattern = producePatternMatcher2S(chipIdForCIC, theHybrid->getId(), theStubList);
                    else
                        thePattern = producePatternMatcherPS(chipIdForCIC, numberOfBytesInSinglePacket, theStubList);

                    try
                    {
                        if(is2Smodule)
                            injectStubs2S(theHybrid->getObject(chipId), theStubList);
                        else
                            injectStubsPS(theHybrid->getObject(chipId + 8), numberOfBytesInSinglePacket, theStubList);
                    }
                    catch(const std::exception& e)
                    {
                        // chip not present
                    }
                }
            }

            if(fDoMatchingInFirmware) { runStubInterationsFirmwareMatching(thePatternMatcherContainer, theBoard, chipId, numberOfLines, is10G, stubPatternCounter); }
            else { runStubInterationsSoftwareMatching(theFWInterface, thePatternMatcherContainer, theBoard, numberOfBytesInSinglePacket, chipId, stubPatternCounter); }
        }
        ++stubPatternCounter;
    }
}