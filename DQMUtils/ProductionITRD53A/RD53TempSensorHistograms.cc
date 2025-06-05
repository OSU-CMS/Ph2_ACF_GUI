/*!
  \file                  RD53TempSensorHistograms.cc
  \brief                 Implementation of TempSensor histograms
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  30/04/21
  Support:               email to umberto.molinatti@cern.ch
*/

#include "RD53TempSensorHistograms.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void TempSensorHistograms::fillTC(double time[100], double temperature[5][100], double idealityFactor[4], double calibNTCtemp[4][2], double calibSenstemp[4][2], double power[2])
{
    ////Stability plot
    auto canvas = new TCanvas();
    canvas->Print("Results/tempPlots.pdf[");
    remove("Results/temperature.root"); // Remove old file
    TFile*      file      = new TFile("Results/temperature.root", "new");
    auto        countsMG  = new TMultiGraph();
    const char* tNames[5] = {"TEMPSENS_1", "TEMPSENS_2", "TEMPSENS_3", "TEMPSENS_4", "NTC"};
    for(int sensor = 0; sensor < 5; sensor++)
    {
        TGraph* countPlot = new TGraph(5, time, temperature[sensor]);
        countPlot->SetTitle(tNames[sensor]);
        if(sensor < 4)
        {
            countPlot->SetLineColor(sensor + 1);
            countPlot->SetMarkerColor(sensor + 1);
        }
        else
        {
            countPlot->SetLineColor(42);
            countPlot->SetMarkerColor(42);
        }
        countsMG->Add(countPlot, "APL");
        countPlot->Write();
    }
    countsMG->SetTitle("Temperature Sensor Graph;Time;Temperature");
    countsMG->GetHistogram()->SetMaximum(50.);
    countsMG->GetHistogram()->SetMinimum(0.);
    countsMG->Draw("APL");
    gPad->BuildLegend();
    canvas->Print("Results/tempPlots.pdf");
    canvas->Print("Results/tempPlots.pdf]");
    canvas->Write();
    file->Write();

    // Calibration plot
    remove("Results/calibTemperature.root"); // Remove old file
    TFile* calibFile    = new TFile("Results/calibTemperature.root", "new");
    auto   calibMG      = new TMultiGraph();
    auto   calibMG_temp = new TMultiGraph();
    canvas              = new TCanvas();
    canvas->Print("Results/calibTempPlots.pdf[");
    for(int sensor = 0; sensor < 4; sensor++)
    {
        canvas                 = new TCanvas();
        calibMG                = new TMultiGraph();
        TGraph* calibPlotSens  = new TGraph(2, power, calibSenstemp[sensor]);
        TGraph* calibPlotNoFit = new TGraph(2, power, calibSenstemp[sensor]);
        calibPlotNoFit->SetTitle(tNames[sensor]);
        calibPlotNoFit->SetLineColor(sensor + 1);
        calibPlotNoFit->SetMarkerColor(sensor + 1);
        calibMG_temp->Add(calibPlotNoFit, "AP*");
        TGraph* calibPlotNTC = new TGraph(2, power, calibNTCtemp[sensor]);
        calibPlotSens->Fit("pol1", "RQC", 0, 0.8);
        calibPlotNTC->Fit("pol1", "RQC", 0, 0.8);
        calibPlotNTC->SetLineColor(42);
        calibPlotNTC->SetMarkerColor(42);
        calibPlotNTC->GetFunction("pol1")->SetLineColor(42);
        calibPlotNTC->SetMarkerSize(1.5);
        calibPlotSens->SetLineColor(sensor + 1);
        calibPlotSens->SetMarkerColor(sensor + 1);
        calibPlotSens->GetFunction("pol1")->SetLineColor(sensor + 1);
        calibPlotSens->SetMarkerSize(1.5);
        calibPlotSens->SetTitle(tNames[sensor]);
        calibPlotNTC->SetTitle("NTC");
        calibMG->Add(calibPlotSens, "APL*");
        calibMG->Add(calibPlotNTC, "APL*");
        calibPlotSens->Write();
        calibPlotNTC->Write();
        calibMG->SetTitle("Calibration Plot;Power Consumption (W);Temperature (C)");
        calibMG_temp->SetTitle("Calibration Plot;Power Consumption (W);Temperature (C)");
        calibMG->GetXaxis()->SetLimits(0., 0.8);
        calibMG_temp->GetXaxis()->SetLimits(0., 0.8);
        calibMG->GetHistogram()->SetMaximum(50.);
        calibMG_temp->GetHistogram()->SetMaximum(50.);
        calibMG->GetHistogram()->SetMinimum(0.);
        calibMG_temp->GetHistogram()->SetMinimum(0.);
        calibMG->Draw("APL");
        gPad->BuildLegend();
        canvas->Print("Results/calibTempPlots.pdf");
        canvas->Write();
        calibFile->Write();
    }
    calibMG_temp->Draw("APL");
    gPad->BuildLegend();
    canvas->Print("Results/calibTempPlots.pdf");
    canvas->Write();
    calibFile->Write();

    // Write calibration file
    remove("Results/tempCalibration.txt"); // Remove old file
    std::ofstream outfile;
    outfile.open("Results/tempCalibration.txt", std::ios_base::app);
    outfile << "TEMPSENS_1 Ratio: " << idealityFactor[0] << "\n";
    outfile << "TEMPSENS_2 Ratio: " << idealityFactor[1] << "\n";
    outfile << "TEMPSENS_3 Ratio: " << idealityFactor[2] << "\n";
    outfile << "TEMPSENS_4 Ratio: " << idealityFactor[3] << "\n";
    canvas->Print("Results/calibTempPlots.pdf]");
}
