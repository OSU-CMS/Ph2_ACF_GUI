/*!
        \file                DQMHistogramPedestalEqualization.h
        \brief               base class to create and fill monitoring histograms
        \author              Fabio Ravera, Lorenzo Uplegger
        \version             1.0
        \date                6/5/19
        Support :            mail to : fabio.ravera@cern.ch
 */

#include "DQMUtils/DQMHistogramPedestalEqualization.h"
#include "RootUtils/RootContainerFactory.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH1I.h"
#include "TH2F.h"
#include "TH2I.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/EmptyContainer.h"
#include "Utils/Occupancy.h"
#include "Utils/ThresholdAndNoise.h"
#include "Utils/Utilities.h"

using namespace Ph2_HwDescription;

//========================================================================================================================
DQMHistogramPedestalEqualization::DQMHistogramPedestalEqualization() {}

//========================================================================================================================
DQMHistogramPedestalEqualization::~DQMHistogramPedestalEqualization() {}

//========================================================================================================================
void DQMHistogramPedestalEqualization::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    fDetectorContainer = &theDetectorStructure;

    auto        selectCBCfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::CBC3); };
    std::string selectCBCfunctionName = "SelectCBCfunction";

    auto        selectSSAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
    std::string selectSSAfunctionName = "SelectSSAfunction";

    auto        selectMPAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string selectMPAfunctionName = "SelectMPAfunction";

    // Vplus was relevant for CBC2, now it is saving the threshold... not relevant anymore
    // HistContainer<TH1I> hVplus("VplusValue", "Vplus value", 1, 0, 1);
    // RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fDetectorVplusHistograms, hVplus);

    fDetectorContainer->addReadoutChipQueryFunction(selectCBCfunction, selectCBCfunctionName);
    HistContainer<TH1I> hOffsetCBC("ChannelOffsetValues", "Channel offset values", NCHANNELS, -0.5, NCHANNELS - 0.5);
    hOffsetCBC.fTheHistogram->GetXaxis()->SetTitle("Channel");
    hOffsetCBC.fTheHistogram->GetYaxis()->SetTitle("Offset");
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fDetectorOffsetHistograms, hOffsetCBC);

    HistContainer<TH1F> hOccupancyCBC("ChannelOccupancyAfterOffsetEqualization", "Channel occupancy after offset equalization", NCHANNELS, -0.5, NCHANNELS - 0.5);
    hOccupancyCBC.fTheHistogram->GetXaxis()->SetTitle("Channel");
    hOccupancyCBC.fTheHistogram->GetYaxis()->SetTitle("Occupancy");
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fDetectorOccupancyHistograms, hOccupancyCBC);
    fDetectorContainer->removeReadoutChipQueryFunction(selectCBCfunctionName);

    fDetectorContainer->addReadoutChipQueryFunction(selectSSAfunction, selectSSAfunctionName);
    HistContainer<TH1I> hOffsetSSA("ChannelOffsetValues", "Channel offset values", NSSACHANNELS, -0.5, NSSACHANNELS - 0.5);
    hOffsetSSA.fTheHistogram->GetXaxis()->SetTitle("Channel");
    hOffsetSSA.fTheHistogram->GetYaxis()->SetTitle("Offset");
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fDetectorOffsetHistograms, hOffsetSSA);

    HistContainer<TH1F> hOccupancySSA("ChannelOccupancyAfterOffsetEqualization", "Channel occupancy after offset equalization", NSSACHANNELS, -0.5, NSSACHANNELS - 0.5);
    hOccupancySSA.fTheHistogram->GetXaxis()->SetTitle("Channel");
    hOccupancySSA.fTheHistogram->GetYaxis()->SetTitle("Occupancy");
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fDetectorOccupancyHistograms, hOccupancySSA);
    fDetectorContainer->removeReadoutChipQueryFunction(selectSSAfunctionName);

    fDetectorContainer->addReadoutChipQueryFunction(selectMPAfunction, selectMPAfunctionName);
    HistContainer<TH2I> hOffsetMPA("2DChannelOffsetValues", "2D channel offset values", NSSACHANNELS, -0.5, NSSACHANNELS - 0.5, NMPAROWS, -0.5, NMPAROWS - 0.5);
    hOffsetMPA.fTheHistogram->GetXaxis()->SetTitle("Row");
    hOffsetMPA.fTheHistogram->GetYaxis()->SetTitle("Column");
    hOffsetMPA.fTheHistogram->SetStats(false);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fDetectorOffsetHistograms, hOffsetMPA);

    HistContainer<TH2F> hOccupancyMPA(
        "2DChannelOccupancyAfterOffsetEqualization", "2D channel occupancy after offset equalization", NSSACHANNELS, -0.5, NSSACHANNELS - 0.5, NMPAROWS, -0.5, NMPAROWS - 0.5);
    hOffsetMPA.fTheHistogram->GetXaxis()->SetTitle("Row");
    hOffsetMPA.fTheHistogram->GetYaxis()->SetTitle("Column");
    hOccupancyMPA.fTheHistogram->SetStats(false);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fDetectorOccupancyHistograms, hOccupancyMPA);

    fDetectorContainer->removeReadoutChipQueryFunction(selectMPAfunctionName);
}

//========================================================================================================================
bool DQMHistogramPedestalEqualization::fill(std::string& inputStream)
{
    ContainerSerialization theVCthSerialization("PedestalEqualizationVCth");
    ContainerSerialization theOccupancySerialization("PedestalEqualizationOccupancy");
    ContainerSerialization theOffsetSerialization("PedestalEqualizationOffset");

    if(theVCthSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched PedestalEqualization Vcth!!!!!\n";
        DetectorDataContainer theDetectorData = theVCthSerialization.deserializeHybridContainer<EmptyContainer, uint16_t, EmptyContainer>(fDetectorContainer);
        // fillVplusPlots(theDetectorData);
        return true;
    }
    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched PedestalEqualization Occupancy!!!!!\n";
        DetectorDataContainer theDetectorData = theOccupancySerialization.deserializeHybridContainer<Occupancy, Occupancy, Occupancy>(fDetectorContainer);
        fillOccupancyPlots(theDetectorData);
        return true;
    }
    if(theOffsetSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched PedestalEqualization Offset!!!!!\n";
        DetectorDataContainer theDetectorData = theOffsetSerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, uint8_t>(fDetectorContainer);
        fillOffsetPlots(theDetectorData);
        return true;
    }

    return false;
}

//========================================================================================================================
void DQMHistogramPedestalEqualization::process()
{
    for(auto board: fDetectorOffsetHistograms)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                std::string offsetCanvasName    = "Offset_B_" + std::to_string(board->getId()) + "_O_" + std::to_string(opticalGroup->getId()) + "_H_" + std::to_string(hybrid->getId());
                std::string occupancyCanvasName = "Occupancy_B_" + std::to_string(board->getId()) + "_O_" + std::to_string(opticalGroup->getId()) + "_H_" + std::to_string(hybrid->getId());

                TCanvas* offsetCanvas    = new TCanvas(offsetCanvasName.c_str(), offsetCanvasName.c_str(), 10, 0, 500, 500);
                TCanvas* occupancyCanvas = new TCanvas(occupancyCanvasName.c_str(), occupancyCanvasName.c_str(), 10, 525, 500, 500);

                offsetCanvas->DivideSquare(hybrid->size());
                occupancyCanvas->DivideSquare(hybrid->size());

                for(auto chip: *hybrid)
                {
                    bool is2Dplot = chip->getId() >= 8;
                    offsetCanvas->cd(chip->getId() + 1);
                    if(is2Dplot)
                        processOccupancy<TH2I>(chip);
                    else
                        processOccupancy<TH1I>(chip);

                    occupancyCanvas->cd(chip->getId() + 1);
                    auto occupancyChip = fDetectorOccupancyHistograms.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId());
                    if(is2Dplot)
                        processOccupancy<TH2F>(occupancyChip);
                    else
                        processOccupancy<TH1F>(occupancyChip);
                }
            }
        }
    }
}

//========================================================================================================================

void DQMHistogramPedestalEqualization::reset(void) {}

//========================================================================================================================
// void DQMHistogramPedestalEqualization::fillVplusPlots(DetectorDataContainer& theVthr)
// {
//     for(auto board: theVthr)
//     {
//         for(auto opticalGroup: *board)
//         {
//             for(auto hybrid: *opticalGroup)
//             {
//                 for(auto chip: *hybrid)
//                 {
//                     if(!chip->hasSummary()) continue;
//                     TH1I* chipVplusHistogram = fDetectorVplusHistograms.getObject(board->getId())
//                                                    ->getObject(opticalGroup->getId())
//                                                    ->getObject(hybrid->getId())
//                                                    ->getObject(chip->getId())
//                                                    ->getSummary<HistContainer<TH1I>>()
//                                                    .fTheHistogram;
//                     chipVplusHistogram->SetBinContent(1, chip->getSummary<uint16_t>());
//                 }
//             }
//         }
//     }
// }

//========================================================================================================================

void DQMHistogramPedestalEqualization::fillOccupancyPlots(DetectorDataContainer& theOccupancy)
{
    for(auto board: theOccupancy)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    if(chip->hasChannelContainer() == false) continue;
                    ReadoutChip*             theReadoutChip   = fDetectorContainer->getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId());
                    const ChipDataContainer* theChipContainer = fDetectorOccupancyHistograms.getChip(board->getId(), opticalGroup->getId(), hybrid->getId(), chip->getId());
                    // using TH1F and TH2F inheritance from TH1
                    TH1* chipOccupancyHistogram;
                    if(theReadoutChip->getFrontEndType() == FrontEndType::MPA2)
                        chipOccupancyHistogram = theChipContainer->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    else
                        chipOccupancyHistogram = theChipContainer->getSummary<HistContainer<TH1F>>().fTheHistogram;
                    for(uint16_t row = 0; row < chip->getNumberOfRows(); ++row)
                    {
                        for(uint16_t col = 0; col < chip->getNumberOfCols(); ++col)
                        {
                            chipOccupancyHistogram->SetBinContent(linearizeRowAndCols(row, col, chip->getNumberOfCols()) + 1, chip->getChannel<Occupancy>(row, col).fOccupancy);
                            if(theReadoutChip->getFrontEndType() == FrontEndType::MPA2)
                            {
                                chipOccupancyHistogram->SetBinContent(col + 1, row + 1, chip->getChannel<Occupancy>(row, col).fOccupancy);
                                chipOccupancyHistogram->SetBinError(col + 1, row + 1, chip->getChannel<Occupancy>(row, col).fOccupancyError);
                            }
                            else
                            {
                                chipOccupancyHistogram->SetBinContent(linearizeRowAndCols(row, col, chip->getNumberOfCols()) + 1, chip->getChannel<Occupancy>(row, col).fOccupancy);
                                chipOccupancyHistogram->SetBinError(linearizeRowAndCols(row, col, chip->getNumberOfCols()) + 1, chip->getChannel<Occupancy>(row, col).fOccupancyError);
                            }
                        }
                    }
                }
            }
        }
    }
}

//========================================================================================================================

void DQMHistogramPedestalEqualization::fillOffsetPlots(DetectorDataContainer& theOffsets)
{
    for(auto board: theOffsets)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    if(chip->hasChannelContainer() == false) continue;
                    ReadoutChip*             theReadoutChip   = fDetectorContainer->getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId());
                    const ChipDataContainer* theChipContainer = fDetectorOffsetHistograms.getChip(board->getId(), opticalGroup->getId(), hybrid->getId(), chip->getId());
                    // using TH1F and TH2F inheritance from TH1
                    TH1* chipOffsetHistogram;
                    if(theReadoutChip->getFrontEndType() == FrontEndType::MPA2) { chipOffsetHistogram = theChipContainer->getSummary<HistContainer<TH2I>>().fTheHistogram; }
                    else { chipOffsetHistogram = theChipContainer->getSummary<HistContainer<TH1I>>().fTheHistogram; }
                    for(uint16_t row = 0; row < chip->getNumberOfRows(); ++row)
                    {
                        for(uint16_t col = 0; col < chip->getNumberOfCols(); ++col)
                        {
                            if(theReadoutChip->getFrontEndType() == FrontEndType::MPA2) { chipOffsetHistogram->SetBinContent(col + 1, row + 1, chip->getChannel<uint8_t>(row, col)); }
                            else { chipOffsetHistogram->SetBinContent(linearizeRowAndCols(row, col, chip->getNumberOfCols()) + 1, chip->getChannel<uint8_t>(row, col)); }
                        }
                    }
                }
            }
        }
    }
}

//========================================================================================================================
