/*!
        \file                CBCHistogramPulseShape.cc
        \brief               DQM class for Calibration example -> use it as a templare
        \author              Fabio Ravera
        \date                17/1/20
        Support :            mail to : fabio.ravera@cern.ch
*/
#include "DQMUtils/CBCHistogramPulseShape.h"
#include "RootUtils/HistContainer.h"
#include "RootUtils/RootContainerFactory.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TH2F.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/Occupancy.h"
#include "Utils/ThresholdAndNoise.h"

//========================================================================================================================
CBCHistogramPulseShape::CBCHistogramPulseShape() {}

//========================================================================================================================
CBCHistogramPulseShape::~CBCHistogramPulseShape() {}

//========================================================================================================================
void CBCHistogramPulseShape::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    fInitialVcth           = findValueInSettings<double>(pSettingsMap, "PulseShapeInitialVcth", 250);
    fInitialLatency        = findValueInSettings<double>(pSettingsMap, "PulseShapeInitialLatency", 200);
    fFinalVcth             = findValueInSettings<double>(pSettingsMap, "PulseShapeFinalVcth", 600);
    fVcthStep              = findValueInSettings<double>(pSettingsMap, "PulseShapeVCthStep", 10);
    fInitialDelay          = findValueInSettings<double>(pSettingsMap, "PulseShape_InitialDelay", 0);
    fFinalDelay            = findValueInSettings<double>(pSettingsMap, "PulseShape_FinalDelay", 25);
    fDelayStep             = findValueInSettings<double>(pSettingsMap, "PulseShape_DelayStep", 1);
    fPlotPulseShapeSCurves = findValueInSettings<double>(pSettingsMap, "PlotPulseShapeSCurves", 0);

    uint32_t numberOfChannels = theDetectorStructure.getFirstObject()->getFirstObject()->getFirstObject()->getFirstObject()->size();
    int      delayNbins       = (fFinalDelay - fInitialDelay) / fDelayStep + 1;
    fEffectiveFinalDelay      = (delayNbins - 1) * fDelayStep + fInitialDelay;

    float delayHistogramMin = fInitialDelay - fDelayStep / 2.;
    float delayHistogramMax = fEffectiveFinalDelay + fDelayStep / 2.;

    HistContainer<TH1F> theTH1FPulseShapeContainer("PulseShapePerChannel", "PulseShape Per Channel", delayNbins, delayHistogramMin, delayHistogramMax);
    theTH1FPulseShapeContainer.fTheHistogram->GetXaxis()->SetTitle("time [ns]");
    theTH1FPulseShapeContainer.fTheHistogram->GetYaxis()->SetTitle("Vcth");
    theTH1FPulseShapeContainer.fTheHistogram->SetStats(false);
    RootContainerFactory::bookChannelHistograms(theOutputFile, theDetectorStructure, fDetectorChannelPulseShapeHistograms, theTH1FPulseShapeContainer);

    theTH1FPulseShapeContainer.fTheHistogram->SetNameTitle("PulseShapePerChip", "PulseShape Per Chip");
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fDetectorChipPulseShapeHistograms, theTH1FPulseShapeContainer);

    if(fPlotPulseShapeSCurves)
    {
        for(uint16_t delay = fInitialDelay; delay <= fFinalDelay; delay += fDelayStep)
        {
            uint16_t delayDAC   = 25 - (delay % 25);
            uint16_t latencyDAC = fInitialLatency - (delay / 25);
            if(delayDAC == 25)
            {
                delayDAC   = 0;
                latencyDAC = latencyDAC + 1;
            }

            uint16_t nYbins = 1024;
            float    minY   = -0.5;
            float    maxY   = 1023.5;

            std::string histogramName = "SCurve_latency_" + std::to_string(latencyDAC) + "_delay_" + std::to_string(delayDAC);

            HistContainer<TH2F> theTH2FSCurve(histogramName.c_str(), histogramName.c_str(), numberOfChannels, -0.5, numberOfChannels - 0.5, nYbins, minY, maxY);
            RootContainerFactory::bookChipHistograms<HistContainer<TH2F>>(theOutputFile, theDetectorStructure, fDetectorSCurveHistogramMap[std::make_tuple(latencyDAC, delayDAC)], theTH2FSCurve);
        }
    }
}

//========================================================================================================================
void CBCHistogramPulseShape::fillCBCPulseShapePlots(uint16_t delay, DetectorDataContainer& theThresholdAndNoiseContainer)
{
    // float latencyStep = -int(delay/25);
    // float binCenterValue = (delay % 25) + (ceil((fFinalDelay-delay) / 25.) -1 ) * 25;
    // float binCenterValue = ceil(fFinalDelay / 25.) * 25 - (delay / 25) * 25. - (25. - delay % 25);
    float binCenterValue = delay;
    std::cout << binCenterValue << std::endl;
    // std::cout<<delay << " - " <<  binCenterValue <<  std::endl;

    for(auto board: theThresholdAndNoiseContainer) // for on boards - begin
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
                    if(chip->getSummaryContainer<ThresholdAndNoise, ThresholdAndNoise>() == nullptr) continue;
                    TH1F* chipPulseShapeHistogram =
                        fDetectorChipPulseShapeHistograms.getObject(boardId)->getObject(opticalGroupId)->getObject(hybridId)->getObject(chipId)->getSummary<HistContainer<TH1F>>().fTheHistogram;
                    int currentBin = chipPulseShapeHistogram->FindBin(binCenterValue);
                    chipPulseShapeHistogram->SetBinContent(currentBin, chip->getSummary<ThresholdAndNoise, ThresholdAndNoise>().fThreshold);
                    chipPulseShapeHistogram->SetBinError(currentBin, chip->getSummary<ThresholdAndNoise, ThresholdAndNoise>().fThresholdError);
                    // Check if the chip data are there (it is needed in the case of the SoC when data may be sent chip
                    // by chip and not in one shot) Get channel data and fill the histogram

                    for(uint16_t row = 0; row < chip->getNumberOfRows(); ++row)
                    {
                        for(uint16_t col = 0; col < chip->getNumberOfCols(); ++col)
                        {
                            TH1F* channelPulseShapeHistogram = fDetectorChannelPulseShapeHistograms.getObject(boardId)
                                                                   ->getObject(opticalGroupId)
                                                                   ->getObject(hybridId)
                                                                   ->getObject(chipId)
                                                                   ->getChannel<HistContainer<TH1F>>(row, col)
                                                                   .fTheHistogram;
                            int currentBin = channelPulseShapeHistogram->FindBin(binCenterValue);
                            channelPulseShapeHistogram->SetBinContent(currentBin, chip->getChannel<ThresholdAndNoise>(row, col).fThreshold);
                            channelPulseShapeHistogram->SetBinError(currentBin, chip->getChannel<ThresholdAndNoise>(row, col).fNoise);
                        }
                    } // for on channel - end
                } // for on chip - end
            } // for on hybrid - end
        } // for on opticalGroup - end
    } // for on boards - end
}

//========================================================================================================================
void CBCHistogramPulseShape::fillSCurvePlots(uint16_t vcthr, uint16_t latency, uint16_t delay, DetectorDataContainer& fSCurveOccupancy)
{
    for(auto board: fSCurveOccupancy)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    TH2F* chipSCurve = fDetectorSCurveHistogramMap.at(std::make_tuple(latency, delay))
                                           .getObject(board->getId())
                                           ->getObject(opticalGroup->getId())
                                           ->getObject(hybrid->getId())
                                           ->getObject(chip->getId())
                                           ->getSummary<HistContainer<TH2F>>()
                                           .fTheHistogram;

                    if(chip->hasChannelContainer() == false) continue;
                    for(uint16_t row = 0; row < chip->getNumberOfRows(); ++row)
                    {
                        for(uint16_t col = 0; col < chip->getNumberOfCols(); ++col)
                        {
                            float tmpOccupancy      = chip->getChannel<Occupancy>(row, col).fOccupancy;
                            float tmpOccupancyError = chip->getChannel<Occupancy>(row, col).fOccupancyError;
                            chipSCurve->SetBinContent(linearizeRowAndCols(row, col, chip->getNumberOfCols()) + 1, vcthr + 1, tmpOccupancy);
                            chipSCurve->SetBinError(linearizeRowAndCols(row, col, chip->getNumberOfCols()) + 1, vcthr + 1, tmpOccupancyError);
                        }
                    }
                }
            }
        }
    }
}

//========================================================================================================================
void CBCHistogramPulseShape::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
    for(auto board: fDetectorChipPulseShapeHistograms) // for on boards - begin
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup) // for on hybrid - begin
            {
                // Create a canvas do draw the plots
                std::string cCanvasName     = "PulseShape_B_" + std::to_string(board->getId()) + "_O_" + std::to_string(opticalGroup->getId()) + "_H_" + std::to_string(hybrid->getId());
                TCanvas*    cChipPulseShape = new TCanvas(cCanvasName.c_str(), cCanvasName.c_str(), 0, 0, 650, 650);
                cChipPulseShape->Divide(0, hybrid->size());

                for(auto chip: *hybrid) // for on chip - begin
                {
                    size_t       chipId        = chip->getId();
                    TVirtualPad* currentCanvas = cChipPulseShape->cd(chipId + 1);
                    TPad*        myPad         = static_cast<TPad*>(cChipPulseShape->GetPad(chipId + 1));
                    // Retreive the corresponging chip histogram:
                    TH1F*   chipPulseShapeHistogram = chip->getSummary<HistContainer<TH1F>>().fTheHistogram;
                    TGaxis* theAxis                 = new TGaxis(myPad->GetUxmin(), myPad->GetUymax(), myPad->GetUxmax() / 3, myPad->GetUymax(), 0., 25., 510, "-");

                    // Format the histogram (here you are outside from the SoC so you can use all the ROOT functions you
                    // need)
                    chipPulseShapeHistogram->DrawCopy();
                    theAxis->SetLabelColor(kRed);
                    theAxis->SetLineColor(kRed);
                    theAxis->Draw();
                    currentCanvas->Modified();
                    currentCanvas->Update();
                } // for on chip - end
            } // for on hybrid - end
        } // for on opticalGroup - end
    } // for on boards - end
}

//========================================================================================================================
void CBCHistogramPulseShape::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool CBCHistogramPulseShape::fill(std::string& inputStream)
{
    ContainerSerialization theThresholdAndNoiseSerialization("CBCPulseShapeThresholdAndNoise");
    ContainerSerialization theSCurveSerialization("CBCPulseShapeSCurve");

    if(theThresholdAndNoiseSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched CBCPulseShape ThresholdAndNoise!!!!!\n";
        uint16_t              delay;
        DetectorDataContainer fDetectorData =
            theThresholdAndNoiseSerialization.deserializeHybridContainer<ThresholdAndNoise, ThresholdAndNoise, ThresholdAndNoise, uint16_t>(fDetectorContainer, delay);
        fillCBCPulseShapePlots(delay, fDetectorData);
        return true;
    }
    else if(theSCurveSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched CBCPulseShape SCurve!!!!!\n";
        uint16_t              threshold, latencyDAC, delayDAC;
        DetectorDataContainer fDetectorData =
            theSCurveSerialization.deserializeHybridContainer<Occupancy, Occupancy, Occupancy, uint16_t, uint16_t, uint16_t>(fDetectorContainer, threshold, latencyDAC, delayDAC);
        fillSCurvePlots(threshold, latencyDAC, delayDAC, fDetectorData);
        return true;
    }
    // the stream does not match, the expected (DQM interface will try to check if other DQM istogrammers are looking
    // for this stream)
    return false;
    // SoC utilities only - END
}
