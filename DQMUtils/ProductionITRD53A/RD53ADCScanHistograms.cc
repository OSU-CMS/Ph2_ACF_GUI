/*!
  \file                  RD53ADCScanHistograms.cc
  \brief                 Implementation of ADCScan histograms
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  16/04/21
  Support:               email to umberto.molinatti@cern.ch
*/

#include "RD53ADCScanHistograms.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void ADCScanHistograms::fillADC(DetectorContainer& DataContainer, double* fitStart, double* fitEnd, double** VMUXvolt, double** ADCcode, double** DNLcode, double** INLcode, std::string* writeVar)
{
    auto canvas = new TCanvas();
    remove("Results/linearity.root"); // Remove old file
    TFile* file = new TFile("Results/linearity.root", "new");
    canvas->Print("Results/ADCplots.pdf[");
    for(const auto cBoard: DataContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    ChipRegMap& pRD53RegMap = static_cast<RD53*>(cChip)->getRegMap();
                    for(int variable = 0; variable < 1; variable++)
                    {
                        LOG(INFO) << BOLDBLUE << "variable = " << BOLDYELLOW << variable << " " << BOLDYELLOW << fitStart[variable] << " " << BOLDYELLOW << fitEnd[variable] << " " << RESET;
                        auto spad1 = new TPad("spad1", "The first subpad", 0, 0.3, 1, 1);
                        auto spad2 = new TPad("spad2", "The second subpad", 0, 0.15, 1, 0.3);
                        auto spad3 = new TPad("spad3", "The third subpad", 0, 0, 1, 0.15);
                        spad1->Draw();
                        spad2->Draw();
                        spad3->Draw();
                        spad1->cd();
                        TGraph* linear = new TGraph(pow(2.0, pRD53RegMap[writeVar[variable]].fBitSize), VMUXvolt[variable], ADCcode[variable]);
                        linear->SetTitle("Linear Graph;Voltage;ADC Code");
                        linear->SetName(writeVar[variable].c_str());
                        linear->Fit("pol1", "", "", fitStart[variable], fitEnd[variable]);
                        // gStyle->SetOptFit();
                        gStyle->SetFitFormat(".2g");
                        linear->Draw("APL");
                        spad2->cd();
                        TGraph* dnlGraph = new TGraph(pow(2.0, pRD53RegMap[writeVar[variable]].fBitSize), VMUXvolt[variable], DNLcode[variable]);
                        dnlGraph->SetTitle("DNL Graph");
                        dnlGraph->SetName(writeVar[variable].c_str());
                        dnlGraph->Draw("AL");
                        spad3->cd();
                        TGraph* inlGraph = new TGraph(pow(2.0, pRD53RegMap[writeVar[variable]].fBitSize), VMUXvolt[variable], INLcode[variable]);
                        inlGraph->SetTitle("INL Graph");
                        inlGraph->SetName(writeVar[variable].c_str());
                        inlGraph->Draw("AL");
                        canvas->cd();
                        canvas->Print("Results/ADCplots.pdf");
                        linear->Write();
                        dnlGraph->Write();
                        inlGraph->Write();
                    }
                }
    file->Write();
    canvas->Print("Results/ADCplots.pdf]");
}
