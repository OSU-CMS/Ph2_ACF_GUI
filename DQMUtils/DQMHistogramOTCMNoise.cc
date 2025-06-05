#include "DQMUtils/DQMHistogramOTCMNoise.h"
#include "RootUtils/RootContainerFactory.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/GenericDataArray.h"
#include "Utils/Utilities.h"

//========================================================================================================================
DQMHistogramOTCMNoise::DQMHistogramOTCMNoise() {}

//========================================================================================================================
DQMHistogramOTCMNoise::~DQMHistogramOTCMNoise() {}

//========================================================================================================================
void DQMHistogramOTCMNoise::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorData ready to receive the information fromm the stream
    // SoC utilities only - END
    fDetectorContainer = &theDetectorStructure;

    auto cSetting = pSettingsMap.find("CMNoise_Nevents");
    if(cSetting != std::end(pSettingsMap))
        fNevents = boost::any_cast<double>(cSetting->second);
    else
        fNevents = 200000; // this should never be the case,since we ran events to get here.

    cSetting = pSettingsMap.find("CMNoise_2DHistograms");
    if(cSetting != std::end(pSettingsMap))
    {
        if(boost::any_cast<double>(cSetting->second) == 1)
            f2DHistograms = true;
        else
            f2DHistograms = false;
    }
    else
        f2DHistograms = false;

    cSetting = pSettingsMap.find("CMNoise_2DHistogramsLight");
    if(cSetting != std::end(pSettingsMap))
    {
        if(boost::any_cast<double>(cSetting->second) == 1)
            f2DHistogramsLight = true;
        else
            f2DHistogramsLight = false;
    }
    else
        f2DHistogramsLight = false;

    cSetting = pSettingsMap.find("CMNoise_manualVcth");
    int VCTH = 0;
    if(cSetting != std::end(pSettingsMap)) { VCTH = boost::any_cast<double>(cSetting->second); }

    cSetting = pSettingsMap.find("CMNoise_nSigmas");
    if(cSetting != std::end(pSettingsMap)) { fListOfThresholds = convertStringToFloatList(findValueInSettings<std::string>(pSettingsMap, "CMNoise_nSigmas", "0")); }

    // If manual VCTH is set, ignore list of thresholds. Otherwise, use list of thresholds
    if(VCTH > 0)
    {
        fListOfThresholds.clear();
        fListOfThresholds.push_back(VCTH);
    }

    for(auto& thr: fListOfThresholds)
    {
        std::string suffixName  = "";
        std::string suffixTitle = "";
        if(VCTH > 0)
        {
            suffixName  = "_VCTH=" + std::to_string(VCTH);
            suffixTitle = " VCTH = " + std::to_string(VCTH);
        }
        else
        {
            if(thr == 0)
            {
                suffixName  = "_OccupancyDriven";
                suffixTitle = " - Occupancy Driven";
            }
            else
            {
                suffixName  = Form("_SigmaNoise_%.2f", thr);
                suffixTitle = Form(" - Sigma Noise = %.2f", thr);
            }
        }

        HistContainer<TH1F> hChipHits(("CommonNoiseHits" + suffixName).c_str(), ("Common noise hits" + suffixTitle).c_str(), NCHANNELS + 2, -0.5, NCHANNELS + 1 + 0.5);
        RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fChipHitHistograms[thr], hChipHits);

        HistContainer<TH1F> hChipHitsBottom(("CommonNoiseHitsBottom" + suffixName).c_str(), ("Common noise hits bottom" + suffixTitle).c_str(), 0.5 * NCHANNELS + 1, -0.5, 0.5 * NCHANNELS + 0.5);
        RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fChipHitHistogramsBottom[thr], hChipHitsBottom);

        HistContainer<TH1F> hChipHitsTop(("CommonNoiseHitsTop" + suffixName).c_str(), ("Common noise hits top" + suffixTitle).c_str(), 0.5 * NCHANNELS + 1, -0.5, 0.5 * NCHANNELS + 0.5);
        RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fChipHitHistogramsTop[thr], hChipHitsTop);

        HistContainer<TH1F> hHybridHits(("CommonNoiseHits" + suffixName).c_str(), ("Common noise hits" + suffixTitle).c_str(), NCHANNELS * NCHIPS_OT + 2, -0.5, NCHANNELS * NCHIPS_OT + 1 + 0.5);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fHybridHitHistograms[thr], hHybridHits);

        HistContainer<TH1F> hHybridHitsBottom(
            ("CommonNoiseHitsBottom" + suffixName).c_str(), ("Common noise hits bottom" + suffixTitle).c_str(), 0.5 * NCHANNELS * NCHIPS_OT + 1, -0.5, 0.5 * NCHANNELS * NCHIPS_OT + 0.5);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fHybridHitHistogramsBottom[thr], hHybridHitsBottom);

        HistContainer<TH1F> hHybridHitsTop(
            ("CommonNoiseHitsTop" + suffixName).c_str(), ("Common noise hits top" + suffixTitle).c_str(), 0.5 * NCHANNELS * NCHIPS_OT + 1, -0.5, 0.5 * NCHANNELS * NCHIPS_OT + 0.5);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fHybridHitHistogramsTop[thr], hHybridHitsTop);

        HistContainer<TH1F> hModuleHits(("CommonNoiseHits" + suffixName).c_str(), ("Common noise hits" + suffixTitle).c_str(), NCHANNELS * NCHIPS_OT * 2 + 1, -0.5, NCHANNELS * NCHIPS_OT * 2 + 0.5);
        RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fModuleHitHistograms[thr], hModuleHits);

        HistContainer<TH1F> hModuleHitsBottom(
            ("CommonNoiseHitsBottom" + suffixName).c_str(), ("Common noise hits bottom" + suffixTitle).c_str(), 0.5 * NCHANNELS * NCHIPS_OT * 2 + 1, -0.5, 0.5 * NCHANNELS * NCHIPS_OT * 2 + 0.5);
        RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fModuleHitHistogramsBottom[thr], hModuleHitsBottom);

        HistContainer<TH1F> hModuleHitsTop(
            ("CommonNoiseHitsTop" + suffixName).c_str(), ("Common noise hits top" + suffixTitle).c_str(), 0.5 * NCHANNELS * NCHIPS_OT * 2 + 1, -0.5, 0.5 * NCHANNELS * NCHIPS_OT * 2 + 0.5);
        RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fModuleHitHistogramsTop[thr], hModuleHitsTop);

        HistContainer<TH2F> h2DModuleSensorCorrelation(("CommonNoiseTopBottomCorrelation" + suffixName).c_str(),
                                                       ("Common noise top bottom correlation" + suffixTitle).c_str(),
                                                       (NCHANNELS * NCHIPS_OT * 2) / 2 + 2,
                                                       -0.5,
                                                       (NCHANNELS * NCHIPS_OT * 2) / 2 + 1 + 0.5,
                                                       (NCHANNELS * NCHIPS_OT * 2) / 2 + 2,
                                                       -0.5,
                                                       (NCHANNELS * NCHIPS_OT * 2) / 2 + 1 + 0.5);
        RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, f2DModuleSensorCorrelation[thr], h2DModuleSensorCorrelation);

        HistContainer<TH2F> h2DHybridSensorCorrelation(("CommonNoiseTopBottomCorrelation" + suffixName).c_str(),
                                                       ("Common noise top bottom correlation" + suffixTitle).c_str(),
                                                       NCHANNELS * NCHIPS_OT / 2 + 2,
                                                       -0.5,
                                                       NCHANNELS * NCHIPS_OT / 2 + 1 + 0.5,
                                                       NCHANNELS * NCHIPS_OT / 2 + 2,
                                                       -0.5,
                                                       NCHANNELS * NCHIPS_OT / 2 + 1 + 0.5);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, f2DHybridSensorCorrelation[thr], h2DHybridSensorCorrelation);

        HistContainer<TH2F> h2DChipSensorCorrelation(("CommonNoiseTopBottomCorrelation" + suffixName).c_str(),
                                                     ("Common noise top bottom correlation" + suffixTitle).c_str(),
                                                     NCHANNELS / 2 + 2,
                                                     -0.5,
                                                     NCHANNELS / 2 + 1 + 0.5,
                                                     NCHANNELS / 2 + 2,
                                                     -0.5,
                                                     NCHANNELS / 2 + 1 + 0.5);
        RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, f2DChipSensorCorrelation[thr], h2DChipSensorCorrelation);

        HistContainer<TH2F> h2DHybridCorrelation(("CommonNoiseCrossHybridCorrelation" + suffixName).c_str(),
                                                 ("Common noise cross hybrid correlation" + suffixTitle).c_str(),
                                                 NCHANNELS * NCHIPS_OT + 2,
                                                 -0.5,
                                                 NCHANNELS * NCHIPS_OT + 1 + 0.5,
                                                 NCHANNELS * NCHIPS_OT + 2,
                                                 -0.5,
                                                 NCHANNELS * NCHIPS_OT + 1 + 0.5);
        RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, f2DHybridCorrelation[thr], h2DHybridCorrelation);

        HistContainer<TH2F> h2DChipCorrelation(("CommonNoiseHybridCorrelation" + suffixName).c_str(),
                                               ("Common noise hybrid correlation" + suffixTitle).c_str(),
                                               NCHANNELS + 2,
                                               -0.5,
                                               NCHANNELS + 1 + 0.5,
                                               NCHANNELS * NCHIPS_OT + 2,
                                               -0.5,
                                               NCHANNELS * NCHIPS_OT + 1 + 0.5);
        RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, f2DChipCorrelation[thr], h2DChipCorrelation);

        if(f2DHistograms)
        {
            HistContainer<TH2F> h2DModuleHits(("2DModuleHits" + suffixName).c_str(),
                                              ("2DModuleHits" + suffixTitle).c_str(),
                                              NCHANNELS * NCHIPS_OT * 2 + 2,
                                              -0.5,
                                              NCHANNELS * NCHIPS_OT * 2 + 1 + 0.5,
                                              NCHANNELS * NCHIPS_OT * 2 + 2,
                                              -0.5,
                                              NCHANNELS * NCHIPS_OT * 2 + 1 + 0.5);
            RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, f2DModuleHitHistograms[thr], h2DModuleHits);

            HistContainer<TH2F> h2DModuleHitsBottom(("2DModuleHitsBottom" + suffixName).c_str(),
                                                    ("2DModuleHitsBottom" + suffixTitle).c_str(),
                                                    NCHANNELS * NCHIPS_OT * 2 + 2,
                                                    -0.5,
                                                    NCHANNELS * NCHIPS_OT * 2 + 1 + 0.5,
                                                    NCHANNELS * NCHIPS_OT * 2 + 2,
                                                    -0.5,
                                                    NCHANNELS * NCHIPS_OT * 2 + 1 + 0.5);
            RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, f2DModuleHitHistogramsBottom[thr], h2DModuleHitsBottom);

            HistContainer<TH2F> h2DModuleHitsTop(("2DModuleHitsTop" + suffixName).c_str(),
                                                 ("2DModuleHitsTop" + suffixTitle).c_str(),
                                                 NCHANNELS * NCHIPS_OT * 2 + 2,
                                                 -0.5,
                                                 NCHANNELS * NCHIPS_OT * 2 + 1 + 0.5,
                                                 NCHANNELS * NCHIPS_OT * 2 + 2,
                                                 -0.5,
                                                 NCHANNELS * NCHIPS_OT * 2 + 1 + 0.5);
            RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, f2DModuleHitHistogramsTop[thr], h2DModuleHitsTop);

            HistContainer<TH2F> h2DHybridHits(("2DHybridHits" + suffixName).c_str(),
                                              ("2DHybridHits" + suffixTitle).c_str(),
                                              NCHANNELS * NCHIPS_OT + 2,
                                              -0.5,
                                              NCHANNELS * NCHIPS_OT + 1 + 0.5,
                                              NCHANNELS * NCHIPS_OT + 2,
                                              -0.5,
                                              NCHANNELS * NCHIPS_OT + 1 + 0.5);
            RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, f2DHybridHitHistograms[thr], h2DHybridHits);

            HistContainer<TH2F> h2DChipHits(
                ("2DChipHits" + suffixName).c_str(), ("2DChipHits" + suffixTitle).c_str(), NCHANNELS + 2, -0.5, NCHANNELS + 1 + 0.5, NCHANNELS + 2, -0.5, NCHANNELS + 1 + 0.5);
            RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, f2DChipHitHistograms[thr], h2DChipHits);

            HistContainer<TH2F> h2DHybridHits_chip(
                ("2DHybridHits_chip" + suffixName).c_str(), ("2DHybridHits_chip" + suffixTitle).c_str(), NCHIPS_OT, 0, NCHANNELS * NCHIPS_OT, NCHIPS_OT, 0, NCHANNELS * NCHIPS_OT);
            RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, f2DHybridHitHistograms_chip[thr], h2DHybridHits_chip);

            HistContainer<TH2F> h2DModuleHits_chip(
                ("2DModuleHits_chip" + suffixName).c_str(), ("2DModuleHits_chip" + suffixTitle).c_str(), NCHIPS_OT * 2, 0, NCHANNELS * NCHIPS_OT * 2, NCHIPS_OT * 2, 0, NCHANNELS * NCHIPS_OT * 2);
            RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, f2DModuleHitHistograms_chip[thr], h2DModuleHits_chip);

            HistContainer<TH2F> h2DModuleHitsBottom_chip(("2DModuleHitsBottom_chip" + suffixName).c_str(),
                                                         ("2DModuleHitsBottom_chip" + suffixTitle).c_str(),
                                                         NCHIPS_OT * 2,
                                                         0,
                                                         NCHANNELS * NCHIPS_OT * 2,
                                                         NCHIPS_OT * 2,
                                                         0,
                                                         NCHANNELS * NCHIPS_OT * 2);
            RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, f2DModuleHitHistogramsBottom_chip[thr], h2DModuleHitsBottom_chip);

            HistContainer<TH2F> h2DModuleHitsTop_chip(("2DModuleHitsTop_chip" + suffixName).c_str(),
                                                      ("2DModuleHitsTop_chip" + suffixTitle).c_str(),
                                                      NCHIPS_OT * 2,
                                                      0,
                                                      NCHANNELS * NCHIPS_OT * 2,
                                                      NCHIPS_OT * 2,
                                                      0,
                                                      NCHANNELS * NCHIPS_OT * 2);
            RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, f2DModuleHitHistogramsTop_chip[thr], h2DModuleHitsTop_chip);
        }

        if(f2DHistogramsLight) // Only a few plots
        {
            HistContainer<TH2F> h2DChipHits(
                ("2DChipHits" + suffixName).c_str(), ("2DChipHits" + suffixTitle).c_str(), NCHANNELS + 2, -0.5, NCHANNELS + 1 + 0.5, NCHANNELS + 2, -0.5, NCHANNELS + 1 + 0.5);
            RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, f2DChipHitHistograms[thr], h2DChipHits);
        }
    }
}

//========================================================================================================================
void DQMHistogramOTCMNoise::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTCMNoise::reset(void)
{
    // Clear histograms if needed
}

bool DQMHistogramOTCMNoise::fill(std::string& inputStream)
{
    if(processInputStream<EmptyContainer, GenericDataArray<uint32_t, 3 * (NCHANNELS + 1)>, EmptyContainer, EmptyContainer>(
           "OTCMNoiseChipHitStream", inputStream, &DQMHistogramOTCMNoise::fillChipHitPlots))
        return true;
    if(processInputStream<EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, 3 * (NCHANNELS * NCHIPS_OT + 1)>, EmptyContainer>(
           "OTCMNoiseHybridHitStream", inputStream, &DQMHistogramOTCMNoise::fillHybridHitPlots))
        return true;
    if(processInputStream<EmptyContainer, EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, 3 * (NCHANNELS * NCHIPS_OT * 2 + 1)>>(
           "OTCMNoiseModuleHitStream", inputStream, &DQMHistogramOTCMNoise::fillModuleHitPlots))
        return true;
    if(processInputStream<EmptyContainer, EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT + 1, NCHANNELS * NCHIPS_OT + 1>>(
           "OTCMNoise2DHybridCorrelationStream", inputStream, &DQMHistogramOTCMNoise::fillHybridCorrelationPlots))
        return true;
    if(processInputStream<EmptyContainer, GenericDataArray<uint32_t, NCHANNELS + 1, NCHANNELS * NCHIPS_OT + 1>, EmptyContainer, EmptyContainer>(
           "OTCMNoise2DChipCorrelationStream", inputStream, &DQMHistogramOTCMNoise::fillChipCorrelationPlots))
        return true;
    if(processInputStream<EmptyContainer, GenericDataArray<uint32_t, (NCHANNELS / 2 + 1), (NCHANNELS / 2 + 1)>, EmptyContainer, EmptyContainer>(
           "OTCMNoise2DSensorChipCorrelationStream", inputStream, &DQMHistogramOTCMNoise::fillSensorChipCorrelationPlots))
        return true;
    if(processInputStream<EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, (NCHANNELS * NCHIPS_OT / 2 + 1), (NCHANNELS * NCHIPS_OT / 2 + 1)>, EmptyContainer>(
           "OTCMNoise2DSensorHybridCorrelationStream", inputStream, &DQMHistogramOTCMNoise::fillSensorHybridCorrelationPlots))
        return true;
    if(processInputStream<EmptyContainer, EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, ((NCHANNELS * NCHIPS_OT * 2) / 2 + 1), ((NCHANNELS * NCHIPS_OT * 2) / 2 + 1)>>(
           "OTCMNoise2DSensorModuleCorrelationStream", inputStream, &DQMHistogramOTCMNoise::fillSensorModuleCorrelationPlots))
        return true;

    if(processInputStream<EmptyContainer, EmptyContainer, EmptyContainer, GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>>(
           "OTCMNoise2DHitStream", inputStream, &DQMHistogramOTCMNoise::fill2DHitPlots))
        return true;

    if(processInputStreamChip<EmptyContainer, GenericDataArray<uint32_t, NCHANNELS, NCHANNELS>, EmptyContainer, EmptyContainer>(
           "OTCMNoise2DHitLightStream", inputStream, &DQMHistogramOTCMNoise::fill2DHitLightPlots))
        return true;

    return false;
}

//========================================================================================================================
void DQMHistogramOTCMNoise::fill2DHitPlots(DetectorDataContainer& the2DHitData, float threshold)
{
    // make a vector of the channel boundaries of each chip
    // checking later I will start with 1, so we can check that a channel is between two bins, add an extra for the last bin and an extra for 0
    std::vector<uint32_t> chipChannelBoundaries;
    for(size_t iChip = 0; iChip < (NCHIPS_OT * 2) + 2; iChip++) { chipChannelBoundaries.push_back(iChip * NCHANNELS); }

    for(auto board: the2DHitData)
    {
        for(auto opticalGroup: *board)
        {
            if(!opticalGroup->hasSummary()) continue;

            TH2F* moduleHitHistogram       = f2DModuleHitHistograms[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
            TH2F* moduleHitHistogramBottom = f2DModuleHitHistogramsBottom[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
            TH2F* moduleHitHistogramTop    = f2DModuleHitHistogramsTop[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
            TH2F* moduleHitHistogram_chip  = f2DModuleHitHistograms_chip[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
            TH2F* moduleHitHistogramBottom_chip =
                f2DModuleHitHistogramsBottom_chip[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
            TH2F* moduleHitHistogramTop_chip = f2DModuleHitHistogramsTop_chip[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;

            moduleHitHistogram->GetXaxis()->SetTitle("Hit 1 position");
            moduleHitHistogram->GetYaxis()->SetTitle("Hit 2 position");
            moduleHitHistogramBottom->GetXaxis()->SetTitle("Hit 1 position");
            moduleHitHistogramBottom->GetYaxis()->SetTitle("Hit 2 position");
            moduleHitHistogramTop->GetXaxis()->SetTitle("Hit 1 position");
            moduleHitHistogramTop->GetYaxis()->SetTitle("Hit 2 position");
            moduleHitHistogram_chip->GetXaxis()->SetTitle("Hit 1 position");
            moduleHitHistogram_chip->GetYaxis()->SetTitle("Hit 2 position");
            moduleHitHistogramBottom_chip->GetXaxis()->SetTitle("Hit 1 position");
            moduleHitHistogramBottom_chip->GetYaxis()->SetTitle("Hit 2 position");
            moduleHitHistogramTop_chip->GetXaxis()->SetTitle("Hit 1 position");
            moduleHitHistogramTop_chip->GetYaxis()->SetTitle("Hit 2 position");

            for(size_t iCh1 = 0; iCh1 < NCHANNELS * NCHIPS_OT * 2; iCh1++)
            {
                for(size_t iCh2 = 0; iCh2 < NCHANNELS * NCHIPS_OT * 2; iCh2++)
                {
                    moduleHitHistogram->SetBinContent(iCh1, iCh2, opticalGroup->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>>().at(iCh1).at(iCh2));

                    // to fill chip-level, need to sum up each bin
                    auto bin_x     = moduleHitHistogram_chip->GetXaxis()->FindBin(iCh1);
                    auto bin_y     = moduleHitHistogram_chip->GetYaxis()->FindBin(iCh2);
                    auto prev_hits = moduleHitHistogram_chip->GetBinContent(bin_x, bin_y);
                    moduleHitHistogram_chip->SetBinContent(
                        bin_x, bin_y, prev_hits + opticalGroup->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>>().at(iCh1).at(iCh2));

                    // Bottom/top
                    if(iCh1 % 2 == 0 && iCh2 % 2 == 0)
                    {
                        moduleHitHistogramBottom->SetBinContent(
                            iCh1, iCh2, opticalGroup->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>>().at(iCh1).at(iCh2));

                        auto prev_hits_bottom = moduleHitHistogramBottom_chip->GetBinContent(bin_x, bin_y);
                        moduleHitHistogramBottom_chip->SetBinContent(
                            bin_x, bin_y, prev_hits_bottom + opticalGroup->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>>().at(iCh1).at(iCh2));
                    }
                    else if(iCh1 % 2 == 1 && iCh2 % 2 == 1)
                    {
                        moduleHitHistogramTop->SetBinContent(
                            iCh1, iCh2, opticalGroup->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>>().at(iCh1).at(iCh2));

                        auto prev_hits_top = moduleHitHistogramTop_chip->GetBinContent(bin_x, bin_y);
                        moduleHitHistogramTop_chip->SetBinContent(
                            bin_x, bin_y, prev_hits_top + opticalGroup->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>>().at(iCh1).at(iCh2));
                    }
                }
            }

            for(auto hybrid: *opticalGroup)
            {
                TH2F* hybridHitHistogram =
                    f2DHybridHitHistograms[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                TH2F* hybridHitHistogram_chip =
                    f2DHybridHitHistograms_chip[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                uint32_t hybridOffset = NCHANNELS * NCHIPS_OT + 1;

                for(size_t iCh1 = 0; iCh1 < NCHANNELS * NCHIPS_OT * 2; iCh1++)
                {
                    for(size_t iCh2 = 0; iCh2 < NCHANNELS * NCHIPS_OT * 2; iCh2++)
                    {
                        // on hybrid 0
                        if(iCh1 < hybridOffset && iCh2 < hybridOffset && hybrid->getId() == 0)
                        {
                            auto bin_x     = hybridHitHistogram_chip->GetXaxis()->FindBin(iCh1);
                            auto bin_y     = hybridHitHistogram_chip->GetYaxis()->FindBin(iCh2);
                            auto prev_hits = hybridHitHistogram_chip->GetBinContent(bin_x, bin_y);
                            hybridHitHistogram->SetBinContent(
                                iCh1, iCh2, opticalGroup->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>>().at(iCh1).at(iCh2));
                            hybridHitHistogram_chip->SetBinContent(
                                bin_x, bin_y, prev_hits + opticalGroup->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>>().at(iCh1).at(iCh2));
                        }
                        // on hybrid 1
                        else if(iCh1 >= hybridOffset && iCh2 >= hybridOffset && hybrid->getId() == 1)
                        {
                            auto bin_x     = hybridHitHistogram_chip->GetXaxis()->FindBin(iCh1 - hybridOffset);
                            auto bin_y     = hybridHitHistogram_chip->GetYaxis()->FindBin(iCh2 - hybridOffset);
                            auto prev_hits = hybridHitHistogram_chip->GetBinContent(bin_x, bin_y);

                            hybridHitHistogram->SetBinContent(iCh1 - hybridOffset,
                                                              iCh2 - hybridOffset,
                                                              opticalGroup->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>>().at(iCh1).at(iCh2));
                            hybridHitHistogram_chip->SetBinContent(
                                bin_x, bin_y, prev_hits + opticalGroup->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>>().at(iCh1).at(iCh2));
                        }
                    }
                }

                for(auto chip: *hybrid)
                {
                    TH2F* chipHitHistogram = f2DChipHitHistograms[threshold]
                                                 .getObject(board->getId())
                                                 ->getObject(opticalGroup->getId())
                                                 ->getObject(hybrid->getId())
                                                 ->getObject(chip->getId())
                                                 ->getSummary<HistContainer<TH2F>>()
                                                 .fTheHistogram;
                    uint16_t iChan_high = chip->getId() + 1 + (hybrid->getId() * NCHIPS_OT);
                    uint16_t iChan_low  = chip->getId() + (hybrid->getId() * NCHIPS_OT);

                    uint32_t chipOffset = (hybrid->getId() * NCHANNELS * NCHIPS_OT) + (chip->getId() * (NCHANNELS));

                    for(size_t iCh1 = chipChannelBoundaries[iChan_low]; iCh1 < chipChannelBoundaries[iChan_high]; iCh1++)
                    {
                        for(size_t iCh2 = chipChannelBoundaries[iChan_low]; iCh2 < chipChannelBoundaries[iChan_high]; iCh2++)
                        {
                            chipHitHistogram->SetBinContent(
                                iCh1 - chipOffset, iCh2 - chipOffset, opticalGroup->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2, NCHANNELS * NCHIPS_OT * 2>>().at(iCh1).at(iCh2));
                        }
                    }
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTCMNoise::fill2DHitLightPlots(DetectorDataContainer& the2DHitData, float threshold)
{
    for(auto board: the2DHitData)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    if(!chip->hasSummary()) continue;

                    TH2F* chipHitHistogram = f2DChipHitHistograms[threshold]
                                                 .getObject(board->getId())
                                                 ->getObject(opticalGroup->getId())
                                                 ->getObject(hybrid->getId())
                                                 ->getObject(chip->getId())
                                                 ->getSummary<HistContainer<TH2F>>()
                                                 .fTheHistogram;

                    auto& chip_summary = chip->getSummary<GenericDataArray<uint32_t, NCHANNELS, NCHANNELS>>();
                    for(size_t iCh1 = 0; iCh1 < NCHANNELS; iCh1++)
                    {
                        for(size_t iCh2 = 0; iCh2 < NCHANNELS; iCh2++) { chipHitHistogram->SetBinContent(iCh1, iCh2, chip_summary.at(iCh1).at(iCh2)); }
                    }
                }
            }
        }
    }
}

//========================================================================================================================

void DQMHistogramOTCMNoise::fillChipHitPlots(DetectorDataContainer& theHitData, float threshold) { return fillChipHitPlots(theHitData, false, threshold); }

void DQMHistogramOTCMNoise::fillChipHitPlots(DetectorDataContainer& theHitData, bool pFitDistributions, float threshold)
{
    const int cNChannels = NCHANNELS;
    for(auto board: theHitData)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    if(!chip->hasSummary()) continue;

                    TH1F* theHistogramBottom = fChipHitHistogramsBottom[threshold]
                                                   .getObject(board->getId())
                                                   ->getObject(opticalGroup->getId())
                                                   ->getObject(hybrid->getId())
                                                   ->getObject(chip->getId())
                                                   ->getSummary<HistContainer<TH1F>>()
                                                   .fTheHistogram;
                    TH1F* theHistogramTop = fChipHitHistogramsTop[threshold]
                                                .getObject(board->getId())
                                                ->getObject(opticalGroup->getId())
                                                ->getObject(hybrid->getId())
                                                ->getObject(chip->getId())
                                                ->getSummary<HistContainer<TH1F>>()
                                                .fTheHistogram;
                    TH1F* theHistogramSum = fChipHitHistograms[threshold]
                                                .getObject(board->getId())
                                                ->getObject(opticalGroup->getId())
                                                ->getObject(hybrid->getId())
                                                ->getObject(chip->getId())
                                                ->getSummary<HistContainer<TH1F>>()
                                                .fTheHistogram;
                    theHistogramBottom->GetXaxis()->SetTitle("Number of hits");
                    theHistogramBottom->GetYaxis()->SetTitle("Number of events");
                    theHistogramTop->GetXaxis()->SetTitle("Number of hits");
                    theHistogramTop->GetYaxis()->SetTitle("Number of events");
                    theHistogramSum->GetXaxis()->SetTitle("Number of hits ");
                    theHistogramSum->GetYaxis()->SetTitle("Number of events");
                    // fill the histogram from the vector, deconvoluting Bottom/Top/Sum
                    auto cDataSummary = chip->getSummary<GenericDataArray<uint32_t, 3 * (cNChannels + 1)>>();
                    for(uint16_t iChan = 0; iChan < cNChannels + 1; iChan++)
                    {
                        theHistogramBottom->SetBinContent(iChan + 1, cDataSummary.at(iChan));
                        theHistogramTop->SetBinContent(iChan + 1, cDataSummary.at((cNChannels + 1) + iChan));
                        theHistogramSum->SetBinContent(iChan + 1, cDataSummary.at(2 * (cNChannels + 1) + iChan));
                    }
                    theHistogramBottom->Sumw2();
                    theHistogramTop->Sumw2();
                    theHistogramSum->Sumw2();

                    if(pFitDistributions)
                    {
                        // do fitting
                        TF1* cChipFit = new TF1("chipFit", hitProbabilityFunction, 0, cNChannels + 1, 4);
                        fitCMNoise(theHistogramBottom, cChipFit, cNChannels / 2);
                        LOG(INFO) << BOLDBLUE << "FE " << hybrid->getId() << " CBC " << chip->getId() << " bottom strip common mode is " << fabs(cChipFit->GetParameter(1)) << "+/-"
                                  << fabs(cChipFit->GetParError(1)) << "%" << RESET;
                        fitCMNoise(theHistogramTop, cChipFit, cNChannels / 2);
                        LOG(INFO) << BOLDBLUE << "FE " << hybrid->getId() << " CBC " << chip->getId() << " top strip common mode is " << fabs(cChipFit->GetParameter(1)) << "+/-"
                                  << fabs(cChipFit->GetParError(1)) << "%" << RESET;
                        fitCMNoise(theHistogramSum, cChipFit, cNChannels);
                        LOG(INFO) << BOLDBLUE << "FE " << hybrid->getId() << " CBC " << chip->getId() << " common mode is " << fabs(cChipFit->GetParameter(1)) << "+/-"
                                  << fabs(cChipFit->GetParError(1)) << "%" << RESET;
                    }
                }
            }
        }
    }
}

void DQMHistogramOTCMNoise::fillHybridHitPlots(DetectorDataContainer& theHitData, float threshold)
{
    const int cNChannels = NCHANNELS * NCHIPS_OT;
    for(auto board: theHitData)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;

                TH1F* theHistogramBottom =
                    fHybridHitHistogramsBottom[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
                TH1F* theHistogramTop =
                    fHybridHitHistogramsTop[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
                TH1F* theHistogramSum =
                    fHybridHitHistograms[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
                theHistogramBottom->GetXaxis()->SetTitle("Number of hits in bottom strips");
                theHistogramBottom->GetYaxis()->SetTitle("Number of events");
                theHistogramTop->GetXaxis()->SetTitle("Number of hits");
                theHistogramTop->GetYaxis()->SetTitle("Number of events");
                theHistogramSum->GetXaxis()->SetTitle("Number of hits ");
                theHistogramSum->GetYaxis()->SetTitle("Number of events");

                // fill the histogram from the vector, deconvoluting Bottom/Top/Sum
                auto cDataSummary = hybrid->getSummary<GenericDataArray<uint32_t, 3 * (cNChannels + 1)>>();
                for(uint16_t iChan = 0; iChan < cNChannels + 1; iChan++)
                {
                    theHistogramBottom->SetBinContent(iChan + 1, cDataSummary.at(iChan));
                    theHistogramTop->SetBinContent(iChan + 1, cDataSummary.at((cNChannels + 1) + iChan));
                    theHistogramSum->SetBinContent(iChan + 1, cDataSummary.at(2 * (cNChannels + 1) + iChan));
                }
                theHistogramBottom->Sumw2();
                theHistogramTop->Sumw2();
                theHistogramSum->Sumw2();
            }
        }
    }
}

void DQMHistogramOTCMNoise::fillModuleHitPlots(DetectorDataContainer& theHitData, float threshold)
{
    const int cNChannels = NCHANNELS * NCHIPS_OT * 2;
    for(auto board: theHitData)
    {
        for(auto opticalGroup: *board)
        {
            if(!opticalGroup->hasSummary()) continue;
            TH1F* theHistogramBottom = fModuleHitHistogramsBottom[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
            TH1F* theHistogramTop    = fModuleHitHistogramsTop[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
            TH1F* theHistogramSum    = fModuleHitHistograms[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
            theHistogramBottom->GetXaxis()->SetTitle("Number of hits in bottom strips");
            theHistogramBottom->GetYaxis()->SetTitle("Number of events");
            theHistogramTop->GetXaxis()->SetTitle("Number of hits");
            theHistogramTop->GetYaxis()->SetTitle("Number of events");
            theHistogramSum->GetXaxis()->SetTitle("Number of hits ");
            theHistogramSum->GetYaxis()->SetTitle("Number of events");

            // fill the histogram from the vector, deconvoluting Bottom/Top/Sum
            auto cDataSummary = opticalGroup->getSummary<GenericDataArray<uint32_t, 3 * (cNChannels + 1)>>();
            for(uint16_t iChan = 0; iChan < cNChannels + 1; iChan++)
            {
                theHistogramBottom->SetBinContent(iChan + 1, cDataSummary.at(iChan));
                theHistogramTop->SetBinContent(iChan + 1, cDataSummary.at((cNChannels + 1) + iChan));
                theHistogramSum->SetBinContent(iChan + 1, cDataSummary.at(2 * (cNChannels + 1) + iChan));
            }
            theHistogramBottom->Sumw2();
            theHistogramTop->Sumw2();
            theHistogramSum->Sumw2();
        }
    }
}

void DQMHistogramOTCMNoise::fillSensorChipCorrelationPlots(DetectorDataContainer& theSensorData, float threshold)
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
                    TH2F* h2DChipSensorCorrelation = f2DChipSensorCorrelation[threshold]
                                                         .getObject(board->getId())
                                                         ->getObject(opticalGroup->getId())
                                                         ->getObject(hybrid->getId())
                                                         ->getObject(chip->getId())
                                                         ->getSummary<HistContainer<TH2F>>()
                                                         .fTheHistogram;

                    h2DChipSensorCorrelation->GetXaxis()->SetTitle("Number of hits on bottom strips");
                    h2DChipSensorCorrelation->GetYaxis()->SetTitle("Number of hits on top strips");
                    for(uint16_t iCh1 = 0; iCh1 < NCHANNELS / 2 + 1; iCh1++)
                    {
                        for(uint16_t iCh2 = 0; iCh2 < NCHANNELS / 2 + 1; iCh2++)
                        {
                            h2DChipSensorCorrelation->SetBinContent(iCh1, iCh2, chip->getSummary<GenericDataArray<uint32_t, NCHANNELS / 2 + 1, NCHANNELS / 2 + 1>>().at(iCh1).at(iCh2));
                        }
                    }
                    h2DChipSensorCorrelation->Sumw2(0);
                }
            }
        }
    }
}

void DQMHistogramOTCMNoise::fillSensorHybridCorrelationPlots(DetectorDataContainer& theSensorData, float threshold)
{
    for(auto board: theSensorData)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;
                TH2F* h2DHybridSensorCorrelation =
                    f2DHybridSensorCorrelation[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                h2DHybridSensorCorrelation->GetXaxis()->SetTitle("Number of hits on bottom strips");
                h2DHybridSensorCorrelation->GetYaxis()->SetTitle("Number of hits on top strips");

                auto thisDataContainer = hybrid->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT / 2 + 1, NCHANNELS * NCHIPS_OT / 2 + 1>>();
                for(uint16_t iCh1 = 0; iCh1 < NCHANNELS * NCHIPS_OT / 2 + 1; iCh1++)
                {
                    for(uint16_t iCh2 = 0; iCh2 < NCHANNELS * NCHIPS_OT / 2 + 1; iCh2++) { h2DHybridSensorCorrelation->SetBinContent(iCh1, iCh2, thisDataContainer.at(iCh1).at(iCh2)); }
                }
                h2DHybridSensorCorrelation->Sumw2(0);
            }
        }
    }
}

void DQMHistogramOTCMNoise::fillSensorModuleCorrelationPlots(DetectorDataContainer& theSensorData, float threshold)
{
    for(auto board: theSensorData)
    {
        for(auto opticalGroup: *board)
        {
            if(!opticalGroup->hasSummary()) continue;
            TH2F* h2DModuleSensorCorrelation = f2DModuleSensorCorrelation[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
            h2DModuleSensorCorrelation->GetXaxis()->SetTitle("Number of hits on bottom strips");
            h2DModuleSensorCorrelation->GetYaxis()->SetTitle("Number of hits on top strips");
            for(uint16_t iCh1 = 0; iCh1 < (NCHANNELS * NCHIPS_OT * 2) / 2 + 1; iCh1++)
            {
                for(uint16_t iCh2 = 0; iCh2 < (NCHANNELS * NCHIPS_OT * 2) / 2 + 1; iCh2++)
                {
                    h2DModuleSensorCorrelation->SetBinContent(
                        iCh1, iCh2, opticalGroup->getSummary<GenericDataArray<uint32_t, (NCHANNELS * NCHIPS_OT * 2) / 2 + 1, (NCHANNELS * NCHIPS_OT * 2) / 2 + 1>>().at(iCh1).at(iCh2));
                }
            }
            h2DModuleSensorCorrelation->Sumw2(0);
        }
    }
}

void DQMHistogramOTCMNoise::fillHybridCorrelationPlots(DetectorDataContainer& theHybridData, float threshold)
{
    // Fill in hybrid Data:
    for(auto board: theHybridData)
    {
        for(auto opticalGroup: *board)
        {
            if(!opticalGroup->hasSummary()) continue;
            TH2F* h2DHybridCorrelation = f2DHybridCorrelation[threshold].getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
            h2DHybridCorrelation->GetXaxis()->SetTitle("Number of hits in right hybrid");
            h2DHybridCorrelation->GetYaxis()->SetTitle("Number of hits in left hybrid");

            for(uint16_t iCh1 = 0; iCh1 < NCHANNELS * NCHIPS_OT + 1; iCh1++)
            {
                for(uint16_t iCh2 = 0; iCh2 < NCHANNELS * NCHIPS_OT + 1; iCh2++)
                {
                    h2DHybridCorrelation->SetBinContent(iCh1, iCh2, opticalGroup->getSummary<GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT + 1, NCHANNELS * NCHIPS_OT + 1>>().at(iCh1).at(iCh2));
                }
            }
            h2DHybridCorrelation->Sumw2(0);
        }
    }
}

void DQMHistogramOTCMNoise::fillChipCorrelationPlots(DetectorDataContainer& theHybridData, float threshold)
{
    // Fill in hybrid Data:
    for(auto board: theHybridData)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    if(!chip->hasSummary()) continue;
                    TH2F* h2DChipCorrelation = f2DChipCorrelation[threshold]
                                                   .getObject(board->getId())
                                                   ->getObject(opticalGroup->getId())
                                                   ->getObject(hybrid->getId())
                                                   ->getObject(chip->getId())
                                                   ->getSummary<HistContainer<TH2F>>()
                                                   .fTheHistogram;
                    h2DChipCorrelation->GetXaxis()->SetTitle("Number of hits in chip");
                    h2DChipCorrelation->GetYaxis()->SetTitle("Number of hits in hybrid");
                    for(uint16_t iCh1 = 0; iCh1 < NCHANNELS + 1; iCh1++)
                    {
                        for(uint16_t iCh2 = 0; iCh2 < NCHANNELS * NCHIPS_OT + 1; iCh2++)
                        {
                            h2DChipCorrelation->SetBinContent(iCh1, iCh2, chip->getSummary<GenericDataArray<uint32_t, NCHANNELS + 1, NCHANNELS * NCHIPS_OT + 1>>().at(iCh1).at(iCh2));
                        }
                    }
                    h2DChipCorrelation->Sumw2(0);
                }
            }
        }
    }
}

void DQMHistogramOTCMNoise::fillHitProfile(DetectorDataContainer& theHitData, float threshold) {}

//========================================================================================================================

// this used to be in CMFits.h -- Written by G. Auzinger
void DQMHistogramOTCMNoise::fitCMNoise(TH1F* pHitCountHist, TF1* pFit, uint32_t pRange)
{
    // Reset uncertainties on input histogram
    pHitCountHist->Sumw2(0);
    pHitCountHist->Sumw2(1);
    // First-order approximation
    double prob = pHitCountHist->GetMean() * 1. / pRange; // pHitCountHist->GetNbinsX();
    // double prob = pHitCountHist->GetMean();

    // retrieve the threshold from the maximum of the actual nhit distribution
    double threshold = inverse_hitProbability(prob);
    // std::cout << "Prob is:" << prob << std::endl;
    // std::cout << "Threshold is:" << threshold << std::endl;

    // initialize cmnFraction to 0 anc later extract from fit
    double cmnFraction = 0.5;
    pFit->SetRange(0, pRange);

    // Set Parameters
    pFit->SetParameter(0, threshold);
    pFit->SetParameter(1, cmnFraction);

    // Fix Parameters nEvents & nActiveStrips as these I know
    pFit->FixParameter(2, fNevents);
    pFit->FixParameter(3, pRange);

    // Name Parameters
    pFit->SetParName(0, "threshold");
    pFit->SetParName(1, "cmnFraction");
    pFit->SetParName(2, "nEvents");
    pFit->SetParName(3, "nActiveStrips");

    // Fit and return
    pHitCountHist->Fit(pFit, "RQ+");
}

double DQMHistogramOTCMNoise::findMaximum(TH1F* pHistogram)
{
    int maxbin = pHistogram->GetMaximumBin();
    return pHistogram->GetXaxis()->GetBinCenter(maxbin);
}

double DQMHistogramOTCMNoise::inverse_hitProbability(double probability) { return sqrt(2) * TMath::ErfInverse(1 - 2 * probability); }
