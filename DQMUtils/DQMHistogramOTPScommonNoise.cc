#include "DQMUtils/DQMHistogramOTPScommonNoise.h"
#include "HWDescription/ReadoutChip.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"

using namespace Ph2_HwDescription;

//========================================================================================================================
DQMHistogramOTPScommonNoise::DQMHistogramOTPScommonNoise() {}

//========================================================================================================================
DQMHistogramOTPScommonNoise::~DQMHistogramOTPScommonNoise() {}

//========================================================================================================================
void DQMHistogramOTPScommonNoise::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;

    auto listOfSigma = convertStringToFloatList(findValueInSettings<std::string>(pSettingsMap, "OTPScommonNoise_ListOfSigma", "0, 3"));

    auto        selectSSAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
    std::string selectSSAfunctionName = "SelectSSAfunction";

    auto        selectMPAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string selectMPAfunctionName = "SelectMPAfunction";

    for(auto numberOfSigma: listOfSigma)
    {
        auto getName = [numberOfSigma](std::string name)
        {
            if(numberOfSigma == 0) return Form("%s_OccupancyDriven", name.c_str());
            return Form("%s_SigmaNoise_%.2f", name.c_str(), numberOfSigma);
        };

        auto getTitle = [numberOfSigma](std::string title)
        {
            if(numberOfSigma == 0) return Form("%s - Occupancy Driven", title.c_str());
            return Form("%s - Sigma Noise = %.2f", title.c_str(), numberOfSigma);
        };

        fDetectorContainer->addReadoutChipQueryFunction(selectSSAfunction, selectSSAfunctionName);

        HistContainer<TH1F> hSSAHits(getName("CommonNoiseHits"), getTitle("Common noise hits"), MAXCICCHANNELS + 2, -0.5, MAXCICCHANNELS + 1 + 0.5);
        hSSAHits.fTheHistogram->GetXaxis()->SetTitle("Number of hits");
        hSSAHits.fTheHistogram->GetYaxis()->SetTitle("Number of events");
        hSSAHits.fTheHistogram->GetXaxis()->SetRangeUser(0, 120);
        RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fStripHitHistograms[numberOfSigma], hSSAHits);

        HistContainer<TH1F> hStripHybridHits(getName("CommonNoiseHitsStrip"), getTitle("Common noise hits strip"), MAXCICCHANNELS + 2, -0.5, MAXCICCHANNELS + 1 + 0.5);
        hStripHybridHits.fTheHistogram->GetXaxis()->SetTitle("Number of hits");
        hStripHybridHits.fTheHistogram->GetYaxis()->SetTitle("Number of events");
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fStripHybridHitHistograms[numberOfSigma], hStripHybridHits);

        HistContainer<TH1F> hStripModuleHits(getName("CommonNoiseHitsStrip"), getTitle("Common noise hits strip"), MAXCICCHANNELS + 2, -0.5, MAXCICCHANNELS + 1 + 0.5);
        hStripModuleHits.fTheHistogram->GetXaxis()->SetTitle("Number of hits");
        hStripModuleHits.fTheHistogram->GetYaxis()->SetTitle("Number of events");
        RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fStripModuleHitHistograms[numberOfSigma], hStripModuleHits);

        fDetectorContainer->removeReadoutChipQueryFunction(selectSSAfunctionName);

        fDetectorContainer->addReadoutChipQueryFunction(selectMPAfunction, selectMPAfunctionName);

        HistContainer<TH1F> hMPAHits(getName("CommonNoiseHits"), getTitle("Common noise hits"), MAXCICCHANNELS + 2, -0.5, MAXCICCHANNELS + 1 + 0.5);
        hMPAHits.fTheHistogram->GetXaxis()->SetTitle("Number of hits");
        hMPAHits.fTheHistogram->GetYaxis()->SetTitle("Number of events");
        hMPAHits.fTheHistogram->GetXaxis()->SetRangeUser(0, 120);
        RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fPixelHitHistograms[numberOfSigma], hMPAHits);

        HistContainer<TH1F> hPixelHybridHits(getName("CommonNoiseHitsPixel"), getTitle("Common noise hits pixel"), MAXCICCHANNELS + 2, -0.5, MAXCICCHANNELS + 1 + 0.5);
        hPixelHybridHits.fTheHistogram->GetXaxis()->SetTitle("Number of hits");
        hPixelHybridHits.fTheHistogram->GetYaxis()->SetTitle("Number of events");
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPixelHybridHitHistograms[numberOfSigma], hPixelHybridHits);

        HistContainer<TH1F> hPixelModuleHits(getName("CommonNoiseHitsPixel"), getTitle("Common noise hits pixel"), MAXCICCHANNELS + 2, -0.5, MAXCICCHANNELS + 1 + 0.5);
        hPixelModuleHits.fTheHistogram->GetXaxis()->SetTitle("Number of hits");
        hPixelModuleHits.fTheHistogram->GetYaxis()->SetTitle("Number of events");
        RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fPixelModuleHitHistograms[numberOfSigma], hPixelModuleHits);

        for(uint8_t chip = NCHIPS_OT; chip < 2 * NCHIPS_OT; chip++)
        {
            HistContainer<TH2F> hSSAtoMPAcorrelation(getName(Form("SSA(%d)toMPA(%d)_CommonNoiseCorrelation", chip - NCHIPS_OT, chip)),
                                                     getTitle(Form("SSA(%d) to MPA(%d) common noise correlation", chip - NCHIPS_OT, chip)),
                                                     MAXCICCHANNELS + 2,
                                                     -0.5,
                                                     MAXCICCHANNELS + 1 + 0.5,
                                                     MAXCICCHANNELS + 2,
                                                     -0.5,
                                                     MAXCICCHANNELS + 1 + 0.5);
            hSSAtoMPAcorrelation.fTheHistogram->GetYaxis()->SetTitle("Number of hits MPA");
            hSSAtoMPAcorrelation.fTheHistogram->GetXaxis()->SetTitle("Number of hits SSA");
            hSSAtoMPAcorrelation.fTheHistogram->GetXaxis()->SetRangeUser(0, 120);
            hSSAtoMPAcorrelation.fTheHistogram->GetYaxis()->SetRangeUser(0, 120);
            RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fSSAtoMPAcorrelation[numberOfSigma][chip], hSSAtoMPAcorrelation);
        }

        fDetectorContainer->removeReadoutChipQueryFunction(selectMPAfunctionName);

        HistContainer<TH2F> hStripPixelHybridHits(getName("CommonNoiseStripPixelCorrelation"),
                                                  getTitle("Common noise strip pixel correlation"),
                                                  MAXCICCHANNELS + 2,
                                                  -0.5,
                                                  MAXCICCHANNELS + 1 + 0.5,
                                                  MAXCICCHANNELS + 2,
                                                  -0.5,
                                                  MAXCICCHANNELS + 1 + 0.5);
        hStripPixelHybridHits.fTheHistogram->GetYaxis()->SetTitle("Number of hits pixels");
        hStripPixelHybridHits.fTheHistogram->GetXaxis()->SetTitle("Number of hits strips");
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fStripPixelHybridHistograms[numberOfSigma], hStripPixelHybridHits);

        HistContainer<TH2F> hStripPixelModuleHits(getName("CommonNoiseStripPixelCorrelation"),
                                                  getTitle("Common noise strip pixel correlation"),
                                                  MAXCICCHANNELS + 2,
                                                  -0.5,
                                                  MAXCICCHANNELS + 1 + 0.5,
                                                  MAXCICCHANNELS + 2,
                                                  -0.5,
                                                  MAXCICCHANNELS + 1 + 0.5);
        hStripPixelModuleHits.fTheHistogram->GetYaxis()->SetTitle("Number of hits pixels");
        hStripPixelModuleHits.fTheHistogram->GetXaxis()->SetTitle("Number of hits strips");
        RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fStripPixelModuleHistograms[numberOfSigma], hStripPixelModuleHits);
        // SoC utilities only - END
    }
}

//========================================================================================================================
void DQMHistogramOTPScommonNoise::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTPScommonNoise::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTPScommonNoise::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    float numberOfSigma;

    ContainerSerialization theStripChipHitContainerSerialization("OTPScommonNoiseStripChipHit");
    if(theStripChipHitContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTPScommonNoiseStripChipHit!!!!!\n";
        DetectorDataContainer theDetectorData =
            theStripChipHitContainerSerialization.deserializeChipContainer<EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS + 1)>>(fDetectorContainer, numberOfSigma);
        // Filling the histograms
        fillChipHitPlots(theDetectorData, numberOfSigma);
        return true;
    }

    ContainerSerialization thePixelChipHitContainerSerialization("OTPScommonNoisePixelChipHit");
    if(thePixelChipHitContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTPScommonNoisePixelChipHit!!!!!\n";
        DetectorDataContainer theDetectorData =
            thePixelChipHitContainerSerialization.deserializeChipContainer<EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS + 1)>>(fDetectorContainer, numberOfSigma);
        // Filling the histograms
        fillChipHitPlots(theDetectorData, numberOfSigma);
        return true;
    }

    ContainerSerialization theStripHybridHitContainerSerialization("OTPScommonNoiseStripHybridHit");
    if(theStripHybridHitContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTPScommonNoiseStripHybridHit!!!!!\n";
        bool                  isSSA;
        DetectorDataContainer theDetectorData = theStripHybridHitContainerSerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS + 1)>>(
            fDetectorContainer, isSSA, numberOfSigma);
        // Filling the histograms
        fillHybridHitPlots(theDetectorData, isSSA, numberOfSigma);
        return true;
    }

    ContainerSerialization thePixelHybridHitContainerSerialization("OTPScommonNoisePixelHybridHit");
    if(thePixelHybridHitContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTPScommonNoisePixelHybridHit!!!!!\n";
        bool                  isSSA;
        DetectorDataContainer theDetectorData = thePixelHybridHitContainerSerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS + 1)>>(
            fDetectorContainer, isSSA, numberOfSigma);
        // Filling the histograms
        fillHybridHitPlots(theDetectorData, isSSA, numberOfSigma);
        return true;
    }

    ContainerSerialization theStripModuleHitContainerSerialization("OTPScommonNoiseStripModuleHit");
    if(theStripModuleHitContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTPScommonNoiseStripModuleHit!!!!!\n";
        bool                  isSSA;
        DetectorDataContainer theDetectorData =
            theStripModuleHitContainerSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS * 2 + 1)>>(
                fDetectorContainer, isSSA, numberOfSigma);
        // Filling the histograms
        fillModuleHitPlots(theDetectorData, isSSA, numberOfSigma);
        return true;
    }

    ContainerSerialization thePixelModuleHitContainerSerialization("OTPScommonNoisePixelModuleHit");
    if(thePixelModuleHitContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTPScommonNoisePixelModuleHit!!!!!\n";
        bool                  isSSA;
        DetectorDataContainer theDetectorData =
            thePixelModuleHitContainerSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS * 2 + 1)>>(
                fDetectorContainer, isSSA, numberOfSigma);
        // Filling the histograms
        fillModuleHitPlots(theDetectorData, isSSA, numberOfSigma);
        return true;
    }

    ContainerSerialization theSSAMPACorrelationContainerSerialization("OTPScommonNoiseSSAMPACorrelation");
    if(theSSAMPACorrelationContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTPScommonNoiseSSAMPACorrelation!!!!!\n";
        DetectorDataContainer theDetectorData =
            theSSAMPACorrelationContainerSerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS + 1), (MAXCICCHANNELS + 1)>>(
                fDetectorContainer, numberOfSigma);
        // Filling the histograms
        fillSSAtoMPACorrelationPlots(theDetectorData, numberOfSigma);
        return true;
    }

    ContainerSerialization theStripPixelHybridContainerSerialization("OTPScommonNoiseStripPixelHybridCorrelation");
    if(theStripPixelHybridContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTPScommonNoiseStripPixelHybridCorrelation!!!!!\n";
        DetectorDataContainer theDetectorData =
            theStripPixelHybridContainerSerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS + 1), (MAXCICCHANNELS + 1)>>(
                fDetectorContainer, numberOfSigma);
        // Filling the histograms
        fillStripPixelHybridCorrelationPlots(theDetectorData, numberOfSigma);
        return true;
    }

    ContainerSerialization theStripPixelModuleContainerSerialization("OTPScommonNoiseStripPixelModuleCorrelation");
    if(theStripPixelModuleContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTPScommonNoiseStripPixelModuleCorrelation!!!!!\n";
        DetectorDataContainer theDetectorData =
            theStripPixelModuleContainerSerialization
                .deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, (MAXCICCHANNELS * 2 + 1), (MAXCICCHANNELS * 2 + 1)>>(fDetectorContainer,
                                                                                                                                                                                  numberOfSigma);
        // Filling the histograms
        fillStripPixelModuleCorrelationPlots(theDetectorData, numberOfSigma);
        return true;
    }

    // the stream does not match, the expected (DQM interface will try to check if other DQM istogrammers are looking
    //  for this stream)
    return false;
    // SoC utilities only - END
}

void DQMHistogramOTPScommonNoise::fillChipHitPlots(DetectorDataContainer& theHitData, float numberOfSigma)
{
    for(auto board: theHitData)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    if(!chip->hasSummary()) continue;
                    ReadoutChip* theReadoutChip = fDetectorContainer->getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId());
                    if(theReadoutChip->getFrontEndType() == FrontEndType::SSA2)
                    {
                        TH1F* theHistogram = fStripHitHistograms.at(numberOfSigma)
                                                 .getObject(board->getId())
                                                 ->getObject(opticalGroup->getId())
                                                 ->getObject(hybrid->getId())
                                                 ->getObject(chip->getId())
                                                 ->getSummary<HistContainer<TH1F>>()
                                                 .fTheHistogram;

                        fillEventsVsHitsHist<MAXCICCHANNELS + 1>(chip, *theHistogram);
                    }
                    else if(theReadoutChip->getFrontEndType() == FrontEndType::MPA2)
                    {
                        TH1F* theHistogram = fPixelHitHistograms.at(numberOfSigma)
                                                 .getObject(board->getId())
                                                 ->getObject(opticalGroup->getId())
                                                 ->getObject(hybrid->getId())
                                                 ->getObject(chip->getId())
                                                 ->getSummary<HistContainer<TH1F>>()
                                                 .fTheHistogram;

                        fillEventsVsHitsHist<MAXCICCHANNELS + 1>(chip, *theHistogram);
                    }
                }
            }
        }
    }
}

void DQMHistogramOTPScommonNoise::fillHybridHitPlots(DetectorDataContainer& theHitData, bool isStrip, float numberOfSigma)
{
    for(auto board: theHitData)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;
                if(isStrip)
                {
                    TH1F* theHistogram = fStripHybridHitHistograms.at(numberOfSigma)
                                             .getObject(board->getId())
                                             ->getObject(opticalGroup->getId())
                                             ->getObject(hybrid->getId())
                                             ->getSummary<HistContainer<TH1F>>()
                                             .fTheHistogram;
                    fillEventsVsHitsHist<MAXCICCHANNELS + 1>(hybrid, *theHistogram);
                }
                else
                {
                    TH1F* theHistogram = fPixelHybridHitHistograms.at(numberOfSigma)
                                             .getObject(board->getId())
                                             ->getObject(opticalGroup->getId())
                                             ->getObject(hybrid->getId())
                                             ->getSummary<HistContainer<TH1F>>()
                                             .fTheHistogram;
                    fillEventsVsHitsHist<MAXCICCHANNELS + 1>(hybrid, *theHistogram);
                }
            }
        }
    }
}

void DQMHistogramOTPScommonNoise::fillModuleHitPlots(DetectorDataContainer& theHitData, bool isStrip, float numberOfSigma)
{
    for(auto board: theHitData)
    {
        for(auto opticalGroup: *board)
        {
            if(!opticalGroup->hasSummary()) continue;
            if(isStrip)
            {
                TH1F* theHistogram = fStripModuleHitHistograms.at(numberOfSigma).getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
                fillEventsVsHitsHist<MAXCICCHANNELS * 2 + 1>(opticalGroup, *theHistogram);
            }
            else
            {
                TH1F* theHistogram = fPixelModuleHitHistograms.at(numberOfSigma).getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
                fillEventsVsHitsHist<MAXCICCHANNELS * 2 + 1>(opticalGroup, *theHistogram);
            }
        }
    }
}

void DQMHistogramOTPScommonNoise::fillSSAtoMPACorrelationPlots(DetectorDataContainer& theSensorData, float numberOfSigma)
{
    for(auto board: theSensorData)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    if(!chip->hasSummary()) continue;
                    ReadoutChip* theReadoutChip = fDetectorContainer->getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId());
                    if(theReadoutChip->getFrontEndType() == FrontEndType::MPA2)
                    {
                        TH2F* h2DChipSensorCorrelation = fSSAtoMPAcorrelation.at(numberOfSigma)[chip->getId()]
                                                             .getObject(board->getId())
                                                             ->getObject(opticalGroup->getId())
                                                             ->getObject(hybrid->getId())
                                                             ->getSummary<HistContainer<TH2F>>()
                                                             .fTheHistogram;
                        fillCorrelationHist<MAXCICCHANNELS + 1, MAXCICCHANNELS + 1>(chip, h2DChipSensorCorrelation);
                    }
                }
            }
        }
    }
}

void DQMHistogramOTPScommonNoise::fillStripPixelHybridCorrelationPlots(DetectorDataContainer& theSensorData, float numberOfSigma)
{
    for(auto board: theSensorData)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;
                TH2F* h2DHybridCorrelation = fStripPixelHybridHistograms.at(numberOfSigma)
                                                 .getObject(board->getId())
                                                 ->getObject(opticalGroup->getId())
                                                 ->getObject(hybrid->getId())
                                                 ->getSummary<HistContainer<TH2F>>()
                                                 .fTheHistogram;
                fillCorrelationHist<MAXCICCHANNELS + 1, MAXCICCHANNELS + 1>(hybrid, h2DHybridCorrelation);
            }
        }
    }
}

void DQMHistogramOTPScommonNoise::fillStripPixelModuleCorrelationPlots(DetectorDataContainer& theSensorData, float numberOfSigma)
{
    for(auto board: theSensorData)
    {
        for(auto opticalGroup: *board)
        {
            if(!opticalGroup->hasSummary()) continue;
            TH2F* h2DModuleCorrelation = fStripPixelModuleHistograms.at(numberOfSigma).getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
            fillCorrelationHist<MAXCICCHANNELS * 2 + 1, MAXCICCHANNELS * 2 + 1>(opticalGroup, h2DModuleCorrelation);
        }
    }
}
