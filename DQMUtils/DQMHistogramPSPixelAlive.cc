/*!
        \file                DQMHistogramPSPixelAlive.h
        \brief               base class to create and fill monitoring histograms
        \author              Adam Kobert
        \version             1.0
        \date                8/21/23
        Support :            mail to : adam.albert.kobert@cern.ch
 */

#include "DQMUtils/DQMHistogramPSPixelAlive.h"
#include "HWDescription/ReadoutChip.h"
#include "RootUtils/HistContainer.h"
#include "RootUtils/RootContainerFactory.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH1I.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/EmptyContainer.h"
#include "Utils/Occupancy.h"
#include "Utils/ThresholdAndNoise.h"
#include "Utils/Utilities.h"

using namespace Ph2_HwDescription;

//========================================================================================================================
DQMHistogramPSPixelAlive::DQMHistogramPSPixelAlive() {}

//========================================================================================================================
DQMHistogramPSPixelAlive::~DQMHistogramPSPixelAlive() {}

//========================================================================================================================
void DQMHistogramPSPixelAlive::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    fDetectorContainer = &theDetectorStructure;

    // find front-end types
    for(auto cBoard: *fDetectorContainer)
    {
        auto cFrontEndTypes = cBoard->connectedFrontEndTypes();
        fWithCBC            = std::find(cFrontEndTypes.begin(), cFrontEndTypes.end(), FrontEndType::CBC3) != cFrontEndTypes.end();
        fWithSSA            = std::find(cFrontEndTypes.begin(), cFrontEndTypes.end(), FrontEndType::SSA2) != cFrontEndTypes.end();
        fWithMPA            = std::find(cFrontEndTypes.begin(), cFrontEndTypes.end(), FrontEndType::MPA2) != cFrontEndTypes.end();
    }

    std::vector<FrontEndType> cStripTypes             = {FrontEndType::CBC3, FrontEndType::SSA2};
    std::vector<FrontEndType> cPixelTypes             = {FrontEndType::MPA2};
    auto                      selectStripChipFunction = [cStripTypes](const ChipContainer* pChip)
    { return (std::find(cStripTypes.begin(), cStripTypes.end(), static_cast<const ReadoutChip*>(pChip)->getFrontEndType()) != cStripTypes.end()); };
    auto selectPixelChipFunction = [cPixelTypes](const ChipContainer* pChip)
    { return (std::find(cPixelTypes.begin(), cPixelTypes.end(), static_cast<const ReadoutChip*>(pChip)->getFrontEndType()) != cPixelTypes.end()); };

    // find maximum number of channels
    std::vector<size_t> cNPixelChannels(0), cNStripChannels(0);
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    auto cNChannels = theDetectorStructure.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->size();
                    auto cType      = cChip->getFrontEndType();
                    if(cType == FrontEndType::CBC3 || cType == FrontEndType::SSA2) { cNStripChannels.push_back(cNChannels); }
                    else if(cType == FrontEndType::MPA2) { cNPixelChannels.push_back(cNChannels); }
                }
            }
        }
    }
    if(fWithCBC || fWithSSA) { fNStripChannels = *std::max_element(std::begin(cNStripChannels), std::end(cNStripChannels)); }
    if(fWithMPA) { fNPixelChannels = *std::max_element(std::begin(cNPixelChannels), std::end(cNPixelChannels)); }

    //   NCH = theDetectorStructure.getFirstObject()->getFirstObject()->getFirstObject()->getFirstObject()->size();

    std::string queryFunctionName = "ChipType";
    if(fWithCBC || fWithSSA)
    {
        fDetectorContainer->addReadoutChipQueryFunction(selectStripChipFunction, queryFunctionName);
        HistContainer<TH1F> hOccupancy_SSA("PSPixelAliveOccupancy", "Channel Occupancy", fNStripChannels, -0.5, float(fNStripChannels) - 0.5);
        RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fDetectorStripOccupancyHistograms, hOccupancy_SSA);
        fDetectorContainer->removeReadoutChipQueryFunction(queryFunctionName);
    }
    if(fWithMPA)
    {
        fDetectorContainer->addReadoutChipQueryFunction(selectPixelChipFunction, queryFunctionName);
        HistContainer<TH1F> hOccupancy_MPA("PSPixelAliveOccupancy", "Channel Occupancy", fNPixelChannels, -0.5, float(fNPixelChannels) - 0.5);
        RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fDetectorPixelOccupancyHistograms, hOccupancy_MPA);
        fDetectorContainer->removeReadoutChipQueryFunction(queryFunctionName);
    }
}

//========================================================================================================================
bool DQMHistogramPSPixelAlive::fill(std::string& inputStream)
{
    ContainerSerialization theOccupancySerialization("PSPixelAliveOccupancy");

    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        std::cout << "Matched PixelAlive Occupancy!!!!!\n";
        DetectorDataContainer theDetectorData = theOccupancySerialization.deserializeHybridContainer<Occupancy, Occupancy, Occupancy>(fDetectorContainer);
        fillOccupancyPlots(theDetectorData);
        return true;
    }
    return false;
}

//========================================================================================================================
void DQMHistogramPSPixelAlive::process()
{
    for(auto board: *fDetectorContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                std::string occupancyCanvasName = "Occupancy_B_" + std::to_string(board->getId()) + "_O_" + std::to_string(opticalGroup->getId()) + "_H_" + std::to_string(hybrid->getId());

                TCanvas* occupancyCanvas = new TCanvas(occupancyCanvasName.c_str(), occupancyCanvasName.c_str(), 0, 0, 650, 650);

                occupancyCanvas->Divide(hybrid->size());

                for(auto chip: *hybrid)
                {
                    TH1F* occupancyHistogram = nullptr;
                    auto  cType              = chip->getFrontEndType();
                    if(cType == FrontEndType::CBC3 || cType == FrontEndType::SSA2)
                    {
                        occupancyCanvas->cd(chip->getId() + 1 + hybrid->size() * 0);
                        occupancyHistogram = fDetectorStripOccupancyHistograms.getObject(board->getId())
                                                 ->getObject(opticalGroup->getId())
                                                 ->getObject(hybrid->getId())
                                                 ->getObject(chip->getId())
                                                 ->getSummary<HistContainer<TH1F>>()
                                                 .fTheHistogram;
                        // occupancyHistogram->SetStats(false);
                        occupancyHistogram->GetXaxis()->SetTitle("Channel");
                        occupancyHistogram->GetYaxis()->SetTitle("Occupancy");
                        occupancyHistogram->DrawCopy();
                    }
                    else if(cType == FrontEndType::MPA2)
                    {
                        occupancyCanvas->cd(chip->getId() + 1 + hybrid->size() * 0);
                        occupancyHistogram = fDetectorPixelOccupancyHistograms.getObject(board->getId())
                                                 ->getObject(opticalGroup->getId())
                                                 ->getObject(hybrid->getId())
                                                 ->getObject(chip->getId())
                                                 ->getSummary<HistContainer<TH1F>>()
                                                 .fTheHistogram;
                        // occupancyHistogram->SetStats(false);
                        occupancyHistogram->GetXaxis()->SetTitle("Channel");
                        occupancyHistogram->GetYaxis()->SetTitle("Occupancy");
                        occupancyHistogram->DrawCopy();
                    }
                }
            }
        }
    }
}

//========================================================================================================================

void DQMHistogramPSPixelAlive::reset(void) {}

void DQMHistogramPSPixelAlive::fillOccupancyPlots(DetectorDataContainer& theOccupancy)
{
    for(auto board: *fDetectorContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    TH1F* chipOccupancyHistogram = nullptr;
                    auto  cType                  = chip->getFrontEndType();
                    if(cType == FrontEndType::CBC3 || cType == FrontEndType::SSA2)
                    {
                        chipOccupancyHistogram = fDetectorStripOccupancyHistograms.getObject(board->getId())
                                                     ->getObject(opticalGroup->getId())
                                                     ->getObject(hybrid->getId())
                                                     ->getObject(chip->getId())
                                                     ->getSummary<HistContainer<TH1F>>()
                                                     .fTheHistogram;
                        for(uint16_t row = 0; row < chip->getNumberOfRows(); ++row)
                        {
                            for(uint16_t col = 0; col < chip->getNumberOfCols(); ++col)
                            {
                                float occupancy = theOccupancy.getObject(board->getId())
                                                      ->getObject(opticalGroup->getId())
                                                      ->getObject(hybrid->getId())
                                                      ->getObject(chip->getId())
                                                      ->getChannel<Occupancy>(row, col)
                                                      .fOccupancy;
                                float occupancyError = theOccupancy.getObject(board->getId())
                                                           ->getObject(opticalGroup->getId())
                                                           ->getObject(hybrid->getId())
                                                           ->getObject(chip->getId())
                                                           ->getChannel<Occupancy>(row, col)
                                                           .fOccupancyError;
                                chipOccupancyHistogram->SetBinContent(linearizeRowAndCols(row, col, chip->getNumberOfCols()) + 1, occupancy);
                                chipOccupancyHistogram->SetBinError(linearizeRowAndCols(row, col, chip->getNumberOfCols()) + 1, occupancyError);
                            }
                        }
                    }
                    else if(cType == FrontEndType::MPA2)
                    {
                        chipOccupancyHistogram = fDetectorPixelOccupancyHistograms.getObject(board->getId())
                                                     ->getObject(opticalGroup->getId())
                                                     ->getObject(hybrid->getId())
                                                     ->getObject(chip->getId())
                                                     ->getSummary<HistContainer<TH1F>>()
                                                     .fTheHistogram;
                        for(uint16_t row = 0; row < chip->getNumberOfRows(); ++row)
                        {
                            for(uint16_t col = 0; col < chip->getNumberOfCols(); ++col)
                            {
                                float occupancy = theOccupancy.getObject(board->getId())
                                                      ->getObject(opticalGroup->getId())
                                                      ->getObject(hybrid->getId())
                                                      ->getObject(chip->getId())
                                                      ->getChannel<Occupancy>(row, col)
                                                      .fOccupancy;
                                float occupancyError = theOccupancy.getObject(board->getId())
                                                           ->getObject(opticalGroup->getId())
                                                           ->getObject(hybrid->getId())
                                                           ->getObject(chip->getId())
                                                           ->getChannel<Occupancy>(row, col)
                                                           .fOccupancyError;

                                chipOccupancyHistogram->SetBinContent(linearizeRowAndCols(row, col, chip->getNumberOfCols()) + 1, occupancy);
                                chipOccupancyHistogram->SetBinError(linearizeRowAndCols(row, col, chip->getNumberOfCols()) + 1, occupancyError);
                            }
                        }
                    }
                }
            }
        }
    }
}
