/*!
        \file                DQMHistogramCalibrationExample.cc
        \brief               DQM class for Calibration example -> use it as a templare
        \author              Fabio Ravera
        \date                25/7/19
        Support :            mail to : fabio.ravera@cern.ch
*/

#include "DQMUtils/DQMHistogramCalibrationExample.h"
#include "RootUtils/HistContainer.h"
#include "RootUtils/RootContainerFactory.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TH1F.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"

//========================================================================================================================
DQMHistogramCalibrationExample::DQMHistogramCalibrationExample() {}

//========================================================================================================================
DQMHistogramCalibrationExample::~DQMHistogramCalibrationExample() {}

//========================================================================================================================
void DQMHistogramCalibrationExample::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    // creating the histograms fo all the chips:
    // create the HistContainer<TH1F> as you would create a TH1F (it implements some feature needed to avoid memory
    // leaks in copying histograms like the move constructor)
    HistContainer<TH1F> theTH1FPedestalContainer("HitPerChannel", "Hit Per Channel", 254, -0.5, 253.5);
    // create Histograms for all the chips, they will be automatically accosiated to the output file, no need to save
    // them, change the name for every chip or set their directory
    RootContainerFactory::bookChipHistograms<HistContainer<TH1F>>(theOutputFile, theDetectorStructure, fDetectorHitHistograms, theTH1FPedestalContainer);
}

//========================================================================================================================
void DQMHistogramCalibrationExample::fillCalibrationExamplePlots(DetectorDataContainer& theHitContainer)
{
    for(auto board: theHitContainer) // for on boards - begin
    {
        size_t boardId = board->getId();
        for(auto opticalGroup: *board) // for on opticalGroup - begin
        {
            size_t opticalGroupId = opticalGroup->getId();
            for(auto hybrid: *opticalGroup) // for on hybrid - begin
            {
                size_t hybridId = hybrid->getId();
                for(auto chip: *hybrid) // for on chip - begin
                {
                    size_t chipId = chip->getId();
                    // Retreive the corresponging chip histogram:
                    TH1F* chipHitHistogram =
                        fDetectorHitHistograms.getObject(boardId)->getObject(opticalGroupId)->getObject(hybridId)->getObject(chipId)->getSummary<HistContainer<TH1F>>().fTheHistogram;
                    uint channelBin = 1;
                    // Check if the chip data are there (it is needed in the case of the SoC when data may be sent chip
                    // by chip and not in one shot)
                    if(chip->hasChannelContainer() == false) continue;
                    // Get channel data and fill the histogram
                    for(auto channel: *chip->getChannelContainer<uint32_t>()) // for on channel - begin
                    {
                        chipHitHistogram->SetBinContent(channelBin++, channel);
                    } // for on channel - end
                } // for on chip - end
            } // for on hybrid - end
        } // for on opticalGroup - end
    } // for on boards - end
}

//========================================================================================================================
void DQMHistogramCalibrationExample::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
    for(auto board: fDetectorHitHistograms) // for on boards - begin
    {
        size_t boardId = board->getId();
        for(auto opticalGroup: *board) // for on opticalGroup - begin
        {
            size_t opticalGroupId = opticalGroup->getId();
            for(auto hybrid: *opticalGroup) // for on hybrid - begin
            {
                size_t hybridId = hybrid->getId();

                std::string cCanvasName = "Hits_B_" + std::to_string(board->getId()) + "_O_" + std::to_string(opticalGroup->getId()) + "_H_" + std::to_string(hybrid->getId());
                // Create a canvas do draw the plots
                TCanvas* cValidation = new TCanvas(cCanvasName.c_str(), cCanvasName.c_str(), 0, 0, 650, 650);
                cValidation->Divide(hybrid->size());

                for(auto chip: *hybrid) // for on chip - begin
                {
                    size_t chipId = chip->getId();
                    cValidation->cd(chipId + 1);
                    // Retreive the corresponging chip histogram:
                    TH1F* chipHitHistogram =
                        fDetectorHitHistograms.getObject(boardId)->getObject(opticalGroupId)->getObject(hybridId)->getObject(chipId)->getSummary<HistContainer<TH1F>>().fTheHistogram;

                    // Format the histogram (here you are outside from the SoC so you can use all the ROOT functions you
                    // need)
                    chipHitHistogram->SetStats(false);
                    chipHitHistogram->SetLineColor(kRed);
                    chipHitHistogram->DrawCopy();
                } // for on chip - end
            } // for on hybrid - end
        } // for on opticalGroup - end
    } // for on boards - end
}

//========================================================================================================================
void DQMHistogramCalibrationExample::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramCalibrationExample::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES

    // I'm expecting to receive a data stream from an uint32_t contained from calibration "CalibrationExample"
    ContainerSerialization theHitSerialization("CalibrationExampleHits");

    if(theHitSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched CalibrationExample Hits!!!!!\n";
        DetectorDataContainer fDetectorData = theHitSerialization.deserializeHybridContainer<uint32_t, uint32_t, uint32_t>(fDetectorContainer);
        fillCalibrationExamplePlots(fDetectorData);
        return true;
    }
    // the stream does not match, the expected (DQM interface will try to check if other DQM istogrammers are looking
    // for this stream)
    return false;
    // SoC utilities only - END
}
