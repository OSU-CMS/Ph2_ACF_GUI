/*!
  \file                  RD53RingOscillatorHistograms.cc
  \brief                 Implementation of RingOscillator histograms
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  19/04/21
  Support:               email to umberto.molinatti@cern.ch
*/

#include "RD53RingOscillatorHistograms.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void RingOscillatorHistograms::fillRO(double gloPulse[11], double oscCounts[8][11], double oscFrequency[8][11], double trimOscCounts[8][16], double trimOscFrequency[8][16], double trimVoltage[16])
{
    auto canvas = new TCanvas();
    canvas->Print("Results/oscillatorPlots.pdf[");
    remove("Results/oscillator.root"); // Remove old file
    TFile* file = new TFile("Results/oscillator.root", "new");
    // Oscillator graph with counts
    auto countsMG = new TMultiGraph();
    for(int ringOsc = 0; ringOsc < 8; ringOsc++)
    {
        TGraph* countPlot = new TGraph(11, gloPulse, oscCounts[ringOsc]);
        countPlot->SetTitle(oscNames[ringOsc]);
        countsMG->Add(countPlot, "APL");
        countPlot->Write();
    }
    countsMG->SetTitle("Oscillator Counts Graph;Global Pulse Duration[ns];Counts");
    canvas->SetLogx();
    canvas->SetLogy();
    countsMG->Draw("A pmc plc");
    gPad->BuildLegend();
    canvas->Print("Results/oscillatorPlots.pdf");
    canvas->Write();
    file->Write();

    auto canvas2 = new TCanvas();
    // Oscillator graph with frequency
    auto freqMG = new TMultiGraph();
    for(int ringOsc = 0; ringOsc < 8; ringOsc++)
    {
        TGraph* freqPlot = new TGraph(11, gloPulse, oscFrequency[ringOsc]);
        freqPlot->SetTitle(oscNames[ringOsc]);
        freqMG->Add(freqPlot, "APL");
        freqPlot->Write();
    }
    freqMG->SetTitle("Oscillator Frequency Graph;Global Pulse Duration[ns];Frequency[GHz]");
    canvas2->SetLogx();
    freqMG->Draw("A pmc plc");
    gPad->BuildLegend();
    canvas2->Print("Results/oscillatorPlots.pdf");
    canvas2->Write();
    file->Write();

    auto canvas3 = new TCanvas();
    // Oscillator graph with vddd
    auto vdddMG = new TMultiGraph();
    for(int ringOsc = 0; ringOsc < 8; ringOsc++)
    {
        TGraph* freqPlot = new TGraph(16, trimVoltage, trimOscFrequency[ringOsc]);
        freqPlot->SetTitle(oscNames[ringOsc]);
        vdddMG->Add(freqPlot, "APL");
        freqPlot->Write();
    }
    vdddMG->SetTitle("Oscillator Frequency Graph;VDDD[V];Frequency[GHz]");
    vdddMG->Draw("A pmc plc");
    gPad->BuildLegend();
    canvas3->Print("Results/oscillatorPlots.pdf");
    canvas3->Write();
    file->Write();
    canvas->Print("Results/oscillatorPlots.pdf]");
}
