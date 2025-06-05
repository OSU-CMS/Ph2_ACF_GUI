#include "DQMUtils/DQMHistogramOTalignStubPackage.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"

#include "TFile.h"
#include "TH2I.h"

//========================================================================================================================
DQMHistogramOTalignStubPackage::DQMHistogramOTalignStubPackage() {}

//========================================================================================================================
DQMHistogramOTalignStubPackage::~DQMHistogramOTalignStubPackage() {}

//========================================================================================================================
void DQMHistogramOTalignStubPackage::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorData ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    HistContainer<TH2I> bestStubPackageDelayHistogram("Board_BestStubPackageDelay", "Board best stub package delay", 8, -0.5, 7.5, 2, -0.5, 1.5);
    bestStubPackageDelayHistogram.fTheHistogram->SetStats(false);
    bestStubPackageDelayHistogram.fTheHistogram->GetXaxis()->SetTitle("Stub package delay");
    bestStubPackageDelayHistogram.fTheHistogram->GetYaxis()->SetTitle("Hybrid");
    bestStubPackageDelayHistogram.fTheHistogram->GetYaxis()->SetBinLabel(1, "FEHR");
    bestStubPackageDelayHistogram.fTheHistogram->GetYaxis()->SetBinLabel(2, "FEHL");
    RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fBestStubPackageDelayHistogramContainer, bestStubPackageDelayHistogram);
}

//========================================================================================================================
void DQMHistogramOTalignStubPackage::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTalignStubPackage::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
void DQMHistogramOTalignStubPackage::fillBestStubPackageDelay(DetectorDataContainer& theBestStubPackageDelayContainer)
{
    for(auto theBoard: theBestStubPackageDelayContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            auto theBestStubPackageDelayHystogram =
                fBestStubPackageDelayHistogramContainer.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<HistContainer<TH2I>>().fTheHistogram;
            for(auto theHybrid: *theOpticalGroup)
            {
                if(!theHybrid->hasSummary()) continue;
                auto theBestStubPackageDelayVector = theHybrid->getSummary<std::vector<bool>>();
                for(size_t packageDelay = 0; packageDelay < 8; ++packageDelay)
                {
                    theBestStubPackageDelayHystogram->SetBinContent(packageDelay + 1, theHybrid->getId() % 2 + 1, theBestStubPackageDelayVector[packageDelay] ? 1 : 0);
                }
            }
        }
    }
}

//========================================================================================================================
bool DQMHistogramOTalignStubPackage::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    ContainerSerialization theBestStubPackageDelayContainerSerialization("OTalignStubPackageBestStubPackageDelay");

    if(theBestStubPackageDelayContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTalignStubPackage BestStubPackageDelay!!!!\n";
        DetectorDataContainer theDetectorData =
            theBestStubPackageDelayContainerSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, std::vector<bool>, EmptyContainer>(fDetectorContainer);
        fillBestStubPackageDelay(theDetectorData);
        return true;
    }

    return false;
    // SoC utilities only - END
}
