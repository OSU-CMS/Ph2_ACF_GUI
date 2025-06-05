#include "tools/OTverifyMPASSAdataWord.h"
#include "HWInterface/D19cFWInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"
#include "Utils/PatternMatcher.h"
#include "Utils/Utilities.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTverifyMPASSAdataWord::fCalibrationDescription = "Inject L1 and stubs for each MPA + SSA and verify that CIC output corresponds to the expected pattern (calibration skipped for 2S)";

OTverifyMPASSAdataWord::OTverifyMPASSAdataWord() : OTverifyCICdataWord() {}

OTverifyMPASSAdataWord::~OTverifyMPASSAdataWord() {}

void OTverifyMPASSAdataWord::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any
    fNumberOfStubBits     = findValueInSettings<double>("OTverifyMPASSAdataWord_NumberOfTestedStubBits", 1e8);
    fNumberOfL1Bits       = findValueInSettings<double>("OTverifyMPASSAdataWord_NumberOfTestedL1Bits", 1e6);
    fDoMatchingInFirmware = findValueInSettings<double>("OTverifyMPASSAdataWord_DoMatchingInFirmware", 1) > 0;

    ContainerFactory::copyAndInitHybrid<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9, 2>>(*fDetectorContainer, fPatternMatchingEfficiencyContainer);

    setUpPatternMatching();
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTverifyMPASSAdataWord.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTverifyMPASSAdataWord::ConfigureCalibration() {}

void OTverifyMPASSAdataWord::Running()
{
    // Assumes 1 board per Ph2_ACF instance
    if(fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTracker2S) return;
    LOG(INFO) << "Starting OTverifyMPASSAdataWord measurement.";
    Initialise();
    runIntegrityTest();
    fillHistograms();
    LOG(INFO) << "Done with OTverifyMPASSAdataWord.";
    Reset();
}

void OTverifyMPASSAdataWord::Stop(void)
{
    LOG(INFO) << "Stopping OTverifyMPASSAdataWord measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTverifyMPASSAdataWord.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTverifyMPASSAdataWord stopped.";
}

void OTverifyMPASSAdataWord::Pause() {}

void OTverifyMPASSAdataWord::Resume() {}

void OTverifyMPASSAdataWord::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTverifyMPASSAdataWord::fillHistograms()
{
#ifdef __USE_ROOT__
    fDQMHistogramOTverifyMPASSAdataWord.fillPatternMatchingEfficiencyResults(fPatternMatchingEfficiencyContainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization thePatternMatchinEfficiencyContainerSerialization("OTverifyMPASSAdataWordPatternMatchingEfficiency");
        thePatternMatchinEfficiencyContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, fPatternMatchingEfficiencyContainer);
    }
#endif
}

PatternMatcher OTverifyMPASSAdataWord::injectL1PS(ReadoutChip* theMPA, uint8_t chipIdForCIC, D19cFWInterface* theFWInterface, uint8_t numberOfBytesInSinglePacket)
{
    ReadoutChip* theSSA = nullptr;
    try
    {
        theSSA = fDetectorContainer->getObject(theMPA->getBeBoardId())->getObject(theMPA->getOpticalGroupId())->getObject(theMPA->getHybridId())->getObject(theMPA->getId() % 8);
    }
    catch(const std::exception& e)
    {
        LOG(INFO) << YELLOW << "            skipping MPA Id " << +theMPA->getId() << " since corresponding SSA is not enabled" << RESET;
        return PatternMatcher();
    }

    LOG(INFO) << BOLDBLUE << "            injecting clusters on SSA " << +theSSA->getId() << RESET;

    std::vector<Cluster> theStripClusterList;
    for(uint16_t clusterNumber = 0; clusterNumber < 24; ++clusterNumber) { theStripClusterList.push_back(Cluster(0x0, clusterNumber * 3, 2)); }
    static_cast<PSInterface*>(fReadoutChipInterface)->injectNoiseClusters(theSSA, theStripClusterList);

    std::vector<Cluster> thePixelClusterList;
    static_cast<PSInterface*>(fReadoutChipInterface)->injectNoiseClusters(theMPA, thePixelClusterList);
    PatternMatcher thePatternMatcher = produceL1PatternMatcher(thePixelClusterList, theStripClusterList, numberOfBytesInSinglePacket, chipIdForCIC);

    return thePatternMatcher;
}

void OTverifyMPASSAdataWord::setStubLogicParameters(ReadoutChip* theMPA)
{
    fReadoutChipInterface->WriteChipReg(theMPA, "StubWindow", 0);
    fReadoutChipInterface->WriteChipReg(theMPA, "StubMode", 0);
    fReadoutChipInterface->WriteChipReg(theMPA, "CodeM10", fBendingToCode.at(0));
    fReadoutChipInterface->WriteChipReg(theMPA, "CodeDM8", 0);
    fReadoutChipInterface->WriteChipReg(theMPA, "CodeM76", 0);
    fReadoutChipInterface->WriteChipReg(theMPA, "CodeM54", 0);
    fReadoutChipInterface->WriteChipReg(theMPA, "CodeM32", 0);
    fReadoutChipInterface->WriteChipReg(theMPA, "CodeP12", 0);
    fReadoutChipInterface->WriteChipReg(theMPA, "CodeP34", 0);
    fReadoutChipInterface->WriteChipReg(theMPA, "CodeP56", 0);
    fReadoutChipInterface->WriteChipReg(theMPA, "CodeP78", 0);
}

std::vector<std::vector<Stub>> OTverifyMPASSAdataWord::createPSstubList()
{
    std::vector<std::vector<Stub>> theListOfStubInjections;

    for(const auto& theStripCluster: fListOfInjectedStrips)
    {
        std::vector<Stub> theSubList{Stub(theStripCluster.fFirstCol * 2 + theStripCluster.fColWidth / 2, fBendingToCode.at(0), 0xf)};
        theListOfStubInjections.push_back(theSubList);
    }

    return theListOfStubInjections;
}

void OTverifyMPASSAdataWord::setStripOffsetParameters(Ph2_HwDescription::ReadoutChip* theSSA)
{
    std::vector<std::pair<std::string, uint16_t>> stripOffsetRegisters{{"StripOffset_byte0", 0}, {"StripOffset_byte1", 0}, {"StripOffset_byte2", 0}, {"StripOffset_byte3", 0}};

    fReadoutChipInterface->WriteChipMultReg(theSSA, stripOffsetRegisters);
}

void OTverifyMPASSAdataWord::prepareForStubInjection(Ph2_HwDescription::BeBoard* theBoard)
{
    fListOfInjectedStrips = produceStripClusterList();

    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            for(auto theChip: *theHybrid)
            {
                if(theChip->getFrontEndType() == FrontEndType::SSA2)
                {
                    setStripOffsetParameters(theChip);
                    static_cast<PSInterface*>(fReadoutChipInterface)->injectNoiseClusters(theChip, fListOfInjectedStrips);
                }
                else if(theChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    fReadoutChipInterface->WriteChipReg(theChip, "StubMode", 0); // Use normal stub mode
                    setStubLogicParameters(theChip);
                }
            }
        }
    }
}

void OTverifyMPASSAdataWord::injectStubsPS(Ph2_HwDescription::ReadoutChip* theMPA, uint8_t numberOfBytesInSinglePacket, const std::vector<Stub>& listOfStubs)
{
    if(listOfStubs.size() != 1)
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] listOfStubs must be exactly one to test SSA to MPA cluster lines! Aborting..." << std::endl;
        abort();
    }

    uint8_t rowCoordinate = listOfStubs.at(0).fZ;
    uint8_t colCoordinate = listOfStubs.at(0).fSeed;

    std::vector<Cluster> thePixelClusterList = produceMatchingPixelClusterList(rowCoordinate, colCoordinate);
    static_cast<PSInterface*>(fReadoutChipInterface)->injectNoiseClusters(theMPA, thePixelClusterList);
}

PatternMatcher OTverifyMPASSAdataWord::producePatternMatcherPS(uint8_t chipIdForCIC, uint8_t numberOfBytesInSinglePacket, const std::vector<Stub>& listOfStubs)
{
    if(listOfStubs.size() != 1)
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] listOfStubs must be exactly one to test SSA to MPA cluster lines! Aborting..." << std::endl;
        abort();
    }
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
            thePattern.addToPattern(0x0, 0x0, 3);                // BX offset
            thePattern.addToPattern(chipIdForCIC, 0x7, 3);       // Chip ID
            thePattern.addToPattern(theStub.fSeed + 2, 0xFF, 8); // seed
            thePattern.addToPattern(theStub.fBend, 0x7, 3);      // bending
            thePattern.addToPattern(theStub.fZ, 0xF, 4);         // z
        }
    }

    thePattern.addTrailingZeros(numberOfBytesInSinglePacket * 64 * 6);
    return thePattern;
}

std::vector<Cluster> OTverifyMPASSAdataWord::produceStripClusterList()
{
    size_t               numberOfSSAstubClusterLines = 8;
    std::vector<Cluster> theStripClusterList;
    for(size_t stripIt = 0; stripIt < numberOfSSAstubClusterLines; ++stripIt) // injecting 8 clusters of size 1 15 strips spaced
    {
        theStripClusterList.push_back(Cluster(0, fFirstStrip + fStripGap * stripIt, 1));
    }

    return theStripClusterList;
}

std::vector<Cluster> OTverifyMPASSAdataWord::produceMatchingPixelClusterList(uint8_t stubRow, uint8_t stubSeed)
{
    std::vector<Cluster> thePixelClusterList;
    thePixelClusterList.push_back(Cluster(stubRow, stubSeed / 2, 1 + stubSeed % 2));
    return thePixelClusterList;
}

PatternMatcher OTverifyMPASSAdataWord::produceStubPatternMatcher(const std::vector<Stub>& theStubVector, uint8_t numberOfBytesInSinglePacket, uint8_t chipIdForCIC)
{
    size_t numberOfStubs     = 8 * theStubVector.size();
    size_t maximumStubNumber = (numberOfBytesInSinglePacket == 1) ? 16 : 35; // 16 if a 5G, 35 if a 10G
    if(numberOfStubs > maximumStubNumber)                                    // CIC aligns stubs by bending, but in pixel-pixel mode bending is 0 and it is not possible to know what the CIC will drop
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] PS stube injected using pixel-pixel mode, more stubs than the maximum allowed!" << std::endl;
        abort();
    }

    PatternMatcher thePattern;
    thePattern.addToPattern(0x1, 0x1, 1);      // is PS flag
    thePattern.addToPattern(0x0, 0x1FF, 9);    // status bits
    thePattern.addToPattern(0x000, 0x000, 12); // Bx ID
    thePattern.addToPattern(numberOfStubs, 0x3F, 6);

    uint8_t stubSize = 21;
    for(uint8_t bxOffset = 0; bxOffset < 8; ++bxOffset)
    {
        for(auto theStub: theStubVector)
        {
            thePattern.addToPattern(0x0, 0x0, 3);                              // BX offset
            thePattern.addToPattern(chipIdForCIC, 0x7, 3);                     // Chip ID
            thePattern.addToPattern(theStub.fSeed + 2, 0xFF, 8);               // seed
            thePattern.addToPattern(fBendingToCode.at(theStub.fBend), 0x7, 3); // bending
            thePattern.addToPattern(theStub.fZ, 0xF, 4);                       // z
        }
    }

    for(uint8_t emptyStubCounter = 0; emptyStubCounter < maximumStubNumber - numberOfStubs; ++emptyStubCounter)
    {
        thePattern.addToPattern(0x0, 0x1FFFFF, stubSize); // empty stubs
    }

    // padding 0s
    if(numberOfBytesInSinglePacket == 1)
        thePattern.addToPattern(0x0, 0xFFFFF, 20); // 5G case
    else
        thePattern.addToPattern(0x0, 0x1F, 5); // 10G case

    return thePattern;
}

PatternMatcher OTverifyMPASSAdataWord::produceL1PatternMatcher(const std::vector<Cluster>& thePixelClusterList,
                                                               const std::vector<Cluster>& theStripClusterList,
                                                               uint8_t                     numberOfBytesInSinglePacket,
                                                               uint8_t                     chipIdForCIC)
{
    uint8_t numberOfPixelClusters = thePixelClusterList.size();
    uint8_t numberOfStripClusters = theStripClusterList.size();

    // Create expected pattern
    PatternMatcher thePatternMatcher;
    thePatternMatcher.addToPattern(0x0ffffffe, 0xffffffff, 32); // CIC header plus 0 in front added in the transmission
    thePatternMatcher.addToPattern(0x0, 0x1ff, 9);
    thePatternMatcher.addToPattern(0x0, 0x0, 9);
    thePatternMatcher.addToPattern(numberOfStripClusters, 0x7F, 7);
    thePatternMatcher.addToPattern(0x0, 0x1, 1);
    thePatternMatcher.addToPattern(numberOfPixelClusters, 0x7F, 7);

    std::map<uint8_t, std::pair<uint8_t, uint8_t>> orderedStripClusterList;
    for(const auto& theCluster: theStripClusterList) { orderedStripClusterList[theCluster.fFirstCol] = {theCluster.fRow, theCluster.fColWidth}; }
    // wierd MPA packing of SSA clusters
    std::vector<Cluster> packerOrderedStripClusterList(numberOfStripClusters);
    size_t               orderedClusterCount = 0;
    for(const auto& theCluster: orderedStripClusterList)
    {
        size_t vectorPosition = (orderedClusterCount <= size_t(numberOfStripClusters - 1) / 2) ? (2 * orderedClusterCount) : (numberOfStripClusters * 2 - 1 - 2 * orderedClusterCount);
        packerOrderedStripClusterList.at(vectorPosition) = Cluster(theCluster.second.first, theCluster.first, theCluster.second.second);
        ++orderedClusterCount;
    }

    for(const auto& theCluster: packerOrderedStripClusterList)
    {
        thePatternMatcher.addToPattern(chipIdForCIC, 0x7, 3);
        thePatternMatcher.addToPattern(theCluster.fFirstCol + 1, 0x7F, 7); // pixel column address starts from 1
        thePatternMatcher.addToPattern(theCluster.fColWidth - 1, 0x7, 3);
        thePatternMatcher.addToPattern(0x0, 0x0, 1);
    }

    std::map<uint8_t, std::pair<uint8_t, uint8_t>> orderedPixelClusterList;
    for(const auto& theCluster: thePixelClusterList) { orderedPixelClusterList[theCluster.fFirstCol] = {theCluster.fRow, theCluster.fColWidth}; }

    // CIC ouputs cluster with lower address first
    for(const auto& theCluster: orderedPixelClusterList)
    {
        thePatternMatcher.addToPattern(chipIdForCIC, 0x7, 3);
        thePatternMatcher.addToPattern(theCluster.first + 1, 0x7F, 7); // pixel column address starts from 1
        thePatternMatcher.addToPattern(theCluster.second.second - 1, 0x7, 3);
        thePatternMatcher.addToPattern(theCluster.second.first, 0xF, 4);
    }

    return thePatternMatcher;
}

GenericDataArray<float, 2>& OTverifyMPASSAdataWord::getStorageForStubErrorRate(Hybrid* theHybrid, uint8_t chipId, uint8_t line, size_t stubPatternCounter)
{
    return fPatternMatchingEfficiencyContainer.getHybrid(theHybrid->getBeBoardId(), theHybrid->getOpticalGroupId(), theHybrid->getId())
        ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9, 2>>()
        .at(chipId)
        .at(1 + stubPatternCounter);
}

GenericDataArray<float, 2>& OTverifyMPASSAdataWord::getStorageForL1ErrorRate(Chip* theChip)
{
    return fPatternMatchingEfficiencyContainer.getHybrid(theChip->getBeBoardId(), theChip->getOpticalGroupId(), theChip->getHybridId())
        ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9, 2>>()
        .at(theChip->getId() % 8)
        .at(0);
}
