/*!
        \file                DQMHistogramKira.h
        \brief               class to create and fill monitoring histograms for KIRA tests
        \author              Roland Koppenhoefer
        \version             1.0
        \date                27/7/22
        Support :            mail to : rkoppenh@cern.ch
 */

#include "DQMUtils/DQMHistogramKira.h"
#include "RootUtils/RootContainerFactory.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/EmptyContainer.h"
#include "Utils/GenericDataArray.h"
#include "Utils/Occupancy.h"
#include "Utils/ThresholdAndNoise.h"
#include "Utils/Utilities.h"

//========================================================================================================================
DQMHistogramKira::DQMHistogramKira()
{
    fStartLatency                  = 999;
    fLatencyRange                  = 999;
    fKiraCalibrationIntensityStart = 29000;
    fKiraCalibrationIntensityStop  = 31000;
    fKiraCalibrationIntensityStep  = 1000;
}

//========================================================================================================================
DQMHistogramKira::~DQMHistogramKira() {}

//========================================================================================================================
void DQMHistogramKira::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    uint32_t cNCh         = 0;
    uint32_t cNSeedChsS0  = 0;
    uint32_t cNSeedChsS1  = 0;
    uint32_t cNChannelsS0 = 0;
    uint32_t cNChannelsS1 = 0;
    uint32_t cNLinks      = 0;
    for(auto board: theDetectorStructure)
    {
        uint32_t cLinks = 0;
        for(auto opticalGroup: *board)
        {
            cLinks++;
            for(auto hybrid: *opticalGroup)
            {
                uint32_t cN       = 0;
                uint32_t cNS0     = 0;
                uint32_t cNS1     = 0;
                uint32_t cNSeedS0 = 0;
                uint32_t cNSeedS1 = 0;

                uint32_t cMaxS0      = 0;
                uint32_t cMaxS1      = 0;
                uint32_t cMaxSeedsS0 = 0;
                uint32_t cMaxSeedsS1 = 0;
                for(auto chip: *hybrid)
                {
                    cN += chip->size();
                    // only account for seeds in MPAs/CBCs
                    if(chip->size() == NMPAROWS * NSSACHANNELS)
                    {
                        cNSeedS0 = chip->size() / NMPAROWS;
                        cNS0     = chip->size() / NMPAROWS;
                    }
                    else if(chip->size() == NCHANNELS)
                    {
                        cNSeedS0 = chip->size() / 2; // either bottom/top CBC row can be a seed
                        cNS0     = chip->size() / 2;
                        cNS1     = cNS0;
                        cNSeedS1 = cNSeedS0;
                    }
                    else
                    {
                        cNSeedS1 = chip->size();
                        cNS1     = chip->size();
                    }

                    if(cNS0 > cMaxS0) cMaxS0 = cNS0;
                    if(cNS1 > cMaxS1) cMaxS1 = cNS1;
                    if(cNSeedS0 > cMaxSeedsS0) cMaxSeedsS0 = cNSeedS0;
                    if(cNSeedS1 > cMaxSeedsS1) cMaxSeedsS1 = cNSeedS1;
                } // chip

                if(8 * cNS0 > cNChannelsS0) cNChannelsS0 = 8 * cMaxS0;
                if(8 * cNS1 > cNChannelsS1) cNChannelsS1 = 8 * cMaxS1;
                if(8 * cNSeedS0 > cNSeedChsS0) cNSeedChsS0 = 8 * cNSeedS0;
                if(8 * cNSeedS1 > cNSeedChsS1) cNSeedChsS1 = 8 * cNSeedS1;
                if(cN > cNCh) cNCh = cN;

            } // hybrid
        } // OG
        if(cLinks >= cNLinks) cNLinks = cLinks;
    } // board

    LOG(INFO) << BOLDYELLOW << "Total number of channels in S0s connected to this BeBoard " << cNChannelsS0 * cNLinks * 2 << RESET;
    LOG(INFO) << BOLDYELLOW << "Number of channels in S0 " << cNChannelsS0 << RESET;
    LOG(INFO) << BOLDYELLOW << "Number of channels in S1 " << cNChannelsS1 << RESET;
    LOG(INFO) << BOLDYELLOW << "Number of seeds in S0 " << cNSeedChsS0 << RESET;
    LOG(INFO) << BOLDYELLOW << "Number of seeds in S1 " << cNSeedChsS1 << RESET;
    // need to get settings from settings map
    parseSettings(pSettingsMap);

    ContainerFactory::copyStructure(theDetectorStructure, fDetectorData);

    float               cBinSize = (1.0);
    HistContainer<TH1F> hLatency("LatencyValue", "Latency Value", fLatencyRange / cBinSize, fStartLatency, fStartLatency + fLatencyRange);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fLatencyHistograms, hLatency);

    // hit count for latency
    HistContainer<TH1F> hLatencyChip("LatencyChip", "Latency per Chip", fLatencyRange / cBinSize, fStartLatency, fStartLatency + fLatencyRange);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fLatencyTDCHistograms, hLatencyChip);

    // hit count per chip during KIRA test
    HistContainer<TH1F> hKIRAHitsBottom("KIRAHitsBottom", "KIRA Hits in Bottom Sensor", NCHANNELS / 2, 0, NCHANNELS / 2);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fKIRAHitsBottomSensor, hKIRAHitsBottom);
    HistContainer<TH1F> hKIRAHitsTop("KIRAHitsTop", "KIRA Hits in Top Sensor", NCHANNELS / 2, 0, NCHANNELS / 2);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fKIRAHitsTopSensor, hKIRAHitsTop);
    // hit count per hybrid during KIRA test
    HistContainer<TH1F> hKIRAHitsHybridTop("KIRAHitsTop", "Defect Module Channels in Top Sensor", 8 * NCHANNELS / 2, 0, 8 * NCHANNELS / 2);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fKIRAHitsHybridTopSensor, hKIRAHitsHybridTop);
    HistContainer<TH1F> hKIRAHitsHybridBottom("KIRAHitsBottom", "Defect Module Channels in Bottom Sensor", 8 * NCHANNELS / 2, 0, 8 * NCHANNELS / 2);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fKIRAHitsHybridBottomSensor, hKIRAHitsHybridBottom);

    // Hit map per chip (= LED) during KIRA Calibration
    HistContainer<TH2F> hKIRACalibrationBottom("KIRACalibrationBottom",
                                               "KIRA LED Hitmap per Intensity Setting in Bottom Sensor",
                                               (fKiraCalibrationIntensityStop - fKiraCalibrationIntensityStart) / fKiraCalibrationIntensityStep + 1,
                                               fKiraCalibrationIntensityStart - fKiraCalibrationIntensityStep / 2.,
                                               fKiraCalibrationIntensityStop + fKiraCalibrationIntensityStep / 2.,
                                               8 * NCHANNELS / 2,
                                               0,
                                               8 * NCHANNELS / 2);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fKIRACalibrationBottomSensor, hKIRACalibrationBottom);
    HistContainer<TH2F> hKIRACalibrationTop("KIRACalibrationTop",
                                            "KIRA LED Hitmap per Intensity Setting in Top Sensor",
                                            (fKiraCalibrationIntensityStop - fKiraCalibrationIntensityStart) / fKiraCalibrationIntensityStep + 1,
                                            fKiraCalibrationIntensityStart - fKiraCalibrationIntensityStep / 2.,
                                            fKiraCalibrationIntensityStop + fKiraCalibrationIntensityStep / 2.,
                                            8 * NCHANNELS / 2,
                                            0,
                                            8 * NCHANNELS / 2);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fKIRACalibrationTopSensor, hKIRACalibrationTop);
}

//========================================================================================================================
bool DQMHistogramKira::fill(std::string& inputStream) { return false; }

//========================================================================================================================
void DQMHistogramKira::process() {}

//========================================================================================================================

void DQMHistogramKira::reset(void) {}

void DQMHistogramKira::fillLatencyPlots(uint16_t pLatency, uint16_t pTriggerId, DetectorDataContainer& pTDCsummary, uint32_t pNevents)
{
    LOG(DEBUG) << BOLDMAGENTA << "Filling chip latency plots .." << RESET;
    for(auto board: pTDCsummary)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    TH1F* cLatencyTDC = fLatencyTDCHistograms.getObject(board->getId())
                                            ->getObject(opticalGroup->getId())
                                            ->getObject(hybrid->getId())
                                            ->getObject(chip->getId())
                                            ->getSummary<HistContainer<TH1F>>()
                                            .fTheHistogram;
                    cLatencyTDC->GetXaxis()->SetTitle("Latency");
                    cLatencyTDC->GetYaxis()->SetTitle("Illuminated Chip Hit Occupancy");
                    TH1F* cLatencyTDCHybrid =
                        fLatencyHistograms.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
                    cLatencyTDCHybrid->GetXaxis()->SetTitle("Latency");
                    cLatencyTDCHybrid->GetYaxis()->SetTitle("Illuminated Chip Hit Occupancy");
                    int      cBin   = cLatencyTDC->FindBin((float)pLatency);
                    uint32_t cNhits = pTDCsummary.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId())->getSummary<uint16_t>();
                    LOG(INFO) << BOLDMAGENTA << "\t\t..Latency of " << pLatency << " bin of " << +cBin << " OG" << +opticalGroup->getId() << " Hybrid" << +hybrid->getId() << " Chip" << +chip->getId()
                              << " - on average have found " << cNhits << " channels with a hit [per chip per event]." << RESET;
                    cLatencyTDC->SetBinContent(cBin, cNhits / 127. / pNevents);
                    cLatencyTDCHybrid->SetBinContent(cBin, cLatencyTDCHybrid->GetBinContent(cBin) + cNhits / 127. / pNevents);
                }
            }
        }
    }
}

void DQMHistogramKira::fillBottomSensorPlots(DetectorDataContainer& pHitContainer, uint32_t pNevents, uint16_t pLED)
{
    LOG(DEBUG) << BOLDMAGENTA << "Filling bottom sensor chip plots .." << RESET;
    for(auto board: pHitContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                TH1F* cKIRAHitsHybridBottomSensor =
                    fKIRAHitsHybridBottomSensor.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
                cKIRAHitsHybridBottomSensor->GetXaxis()->SetTitle("Bottom Sensor Channel");
                cKIRAHitsHybridBottomSensor->GetYaxis()->SetTitle("1 - Channel Hit Occupancy");
                for(auto chip: *hybrid)
                {
                    // skip all chips that are not directly illuminated by the LED
                    if(hybrid->getId() % 2 == 0 && chip->getId() != 7 - pLED) continue;
                    if(hybrid->getId() % 2 == 1 && chip->getId() != pLED) continue;
                    TH1F* cKIRAHitsBottomSensor = fKIRAHitsBottomSensor.getObject(board->getId())
                                                      ->getObject(opticalGroup->getId())
                                                      ->getObject(hybrid->getId())
                                                      ->getObject(chip->getId())
                                                      ->getSummary<HistContainer<TH1F>>()
                                                      .fTheHistogram;
                    cKIRAHitsBottomSensor->GetXaxis()->SetTitle("Bottom Sensor CBC Channel");
                    cKIRAHitsBottomSensor->GetYaxis()->SetTitle("Channel Hit Occupancy");
                    auto cNhits =
                        pHitContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId())->getSummary<GenericDataArray<float, VECSIZE>>();
                    for(uint32_t cIndx = 0; cIndx < 127; cIndx++)
                    {
                        cKIRAHitsBottomSensor->SetBinContent(cIndx + 1, cNhits.at(cIndx) / (1.0 * pNevents));
                        cKIRAHitsHybridBottomSensor->SetBinContent(127 * chip->getId() + cIndx + 1, 1 - cNhits.at(cIndx) / (1.0 * pNevents));
                    }
                }
            }
        }
    }
}

void DQMHistogramKira::fillTopSensorPlots(DetectorDataContainer& pHitContainer, uint32_t pNevents, uint16_t pLED)
{
    LOG(DEBUG) << BOLDMAGENTA << "Filling top sensor chip plots .." << RESET;
    for(auto board: pHitContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                TH1F* cKIRAHitsHybridTopSensor =
                    fKIRAHitsHybridTopSensor.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
                cKIRAHitsHybridTopSensor->GetXaxis()->SetTitle("Top Sensor Channel");
                cKIRAHitsHybridTopSensor->GetYaxis()->SetTitle("1 - Channel Hit Occupancy");
                for(auto chip: *hybrid)
                {
                    // skip all chips that are not directly illuminated by the LED
                    if(hybrid->getId() % 2 == 0 && chip->getId() != 7 - pLED) continue;
                    if(hybrid->getId() % 2 == 1 && chip->getId() != pLED) continue;
                    TH1F* cKIRAHitsTopSensor = fKIRAHitsTopSensor.getObject(board->getId())
                                                   ->getObject(opticalGroup->getId())
                                                   ->getObject(hybrid->getId())
                                                   ->getObject(chip->getId())
                                                   ->getSummary<HistContainer<TH1F>>()
                                                   .fTheHistogram;
                    cKIRAHitsTopSensor->GetXaxis()->SetTitle("Top Sensor CBC Channel");
                    cKIRAHitsTopSensor->GetYaxis()->SetTitle("Channel Hit Occupancy");
                    auto cNhits =
                        pHitContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId())->getSummary<GenericDataArray<float, VECSIZE>>();
                    for(uint32_t cIndx = 0; cIndx < 127; cIndx++)
                    {
                        cKIRAHitsTopSensor->SetBinContent(cIndx + 1, cNhits.at(cIndx) / (1.0 * pNevents));
                        cKIRAHitsHybridTopSensor->SetBinContent(127 * chip->getId() + cIndx + 1, 1 - cNhits.at(cIndx) / (1.0 * pNevents));
                    }
                }
            }
        }
    }
}

void DQMHistogramKira::fillSensorPlotsCalibration(DetectorDataContainer& pHitContainer, uint32_t pNevents, uint16_t pLED, uint32_t cIntensity, uint16_t pSensor)
{
    for(auto board: pHitContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    // skip all chips that are not directly illuminated by the LED
                    if(hybrid->getId() % 2 == 0 && chip->getId() != 7 - pLED) continue;
                    if(hybrid->getId() % 2 == 1 && chip->getId() != pLED) continue;
                    TH2F* cHitMap = fKIRACalibrationTopSensor.getObject(board->getId())
                                        ->getObject(opticalGroup->getId())
                                        ->getObject(hybrid->getId())
                                        ->getObject(chip->getId())
                                        ->getSummary<HistContainer<TH2F>>()
                                        .fTheHistogram;
                    cHitMap->GetXaxis()->SetTitle("LED Intensity Setting");
                    cHitMap->GetYaxis()->SetTitle("Top Sensor CBC Channel");
                    cHitMap->GetZaxis()->SetTitle("Channel Hit Occupancy");
                    if(pSensor == 0)
                    {
                        cHitMap = fKIRACalibrationBottomSensor.getObject(board->getId())
                                      ->getObject(opticalGroup->getId())
                                      ->getObject(hybrid->getId())
                                      ->getObject(chip->getId())
                                      ->getSummary<HistContainer<TH2F>>()
                                      .fTheHistogram;
                        cHitMap->GetYaxis()->SetTitle("Bottom Sensor CBC Channel");
                    }

                    // Find correct bin
                    auto cBinX = cHitMap->GetXaxis()->FindBin(cIntensity);
                    // Loop over all chips in the DataContainer to fill hitmap
                    for(auto cChip: *hybrid)
                    {
                        auto cNhits = pHitContainer.getObject(board->getId())
                                          ->getObject(opticalGroup->getId())
                                          ->getObject(hybrid->getId())
                                          ->getObject(cChip->getId())
                                          ->getSummary<GenericDataArray<float, VECSIZE>>();
                        for(uint32_t cIndx = 0; cIndx < 127; cIndx++) { cHitMap->SetBinContent(cBinX, 127 * cChip->getId() + cIndx + 1, cNhits.at(cIndx) / (1.0 * pNevents)); }
                    }
                }
            }
        }
    }
}

void DQMHistogramKira::parseSettings(const Ph2_Parser::SettingsMap& pSettingsMap)
{
    auto cSetting = pSettingsMap.find("StartLatency");
    if(cSetting != std::end(pSettingsMap))
        fStartLatency = static_cast<uint16_t>(boost::any_cast<double>(cSetting->second));
    else
        fStartLatency = 0;

    cSetting = pSettingsMap.find("LatencyRange");
    if(cSetting != std::end(pSettingsMap))
        fLatencyRange = static_cast<uint16_t>(boost::any_cast<double>(cSetting->second));
    else
        fLatencyRange = 512;

    cSetting = pSettingsMap.find("KiraCalibrationIntensityStart");
    if(cSetting != std::end(pSettingsMap))
        fKiraCalibrationIntensityStart = static_cast<uint16_t>(boost::any_cast<double>(cSetting->second));
    else
        fKiraCalibrationIntensityStart = 29000;

    cSetting = pSettingsMap.find("KiraCalibrationIntensityStop");
    if(cSetting != std::end(pSettingsMap))
        fKiraCalibrationIntensityStop = static_cast<uint16_t>(boost::any_cast<double>(cSetting->second));
    else
        fKiraCalibrationIntensityStop = 31000;

    cSetting = pSettingsMap.find("KiraCalibrationIntensityStep");
    if(cSetting != std::end(pSettingsMap))
        fKiraCalibrationIntensityStep = static_cast<uint16_t>(boost::any_cast<double>(cSetting->second));
    else
        fKiraCalibrationIntensityStep = 1000;
}
