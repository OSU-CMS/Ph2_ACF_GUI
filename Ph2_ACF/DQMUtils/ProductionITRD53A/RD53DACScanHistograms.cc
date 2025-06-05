/*!
  \file                  RD53DACScanHistograms.cc
  \brief                 Implementation of DACScan histograms
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  09/04/21
  Support:               email to umberto.molinatti@cern.ch
*/

#include "RD53DACScanHistograms.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void DACScanHistograms::fillDAC(DetectorContainer& DataContainer, double* fitStart, double* fitEnd, double** VMUXvolt, double** DACcode, double** DNLcode, double** INLcode, std::string* writeVar)
{
    auto canvas = new TCanvas();
    remove("Results/DAC_linearity.root"); // Remove old file
    TFile* file = new TFile("Results/DAC_linearity.root", "new");
    canvas->Print("Results/DAC_plots.pdf[");
    for(const auto cBoard: DataContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    ChipRegMap& pRD53RegMap = static_cast<RD53*>(cChip)->getRegMap();
                    for(int variable = 0; variable < 1; variable++)
                    {
                        // LOG(INFO) << BOLDBLUE << "variable = " << BOLDYELLOW <<  variable <<" "<< BOLDYELLOW <<  fitStart[variable] <<" "<< BOLDYELLOW <<  fitEnd[variable] << " " <<
                        // pRD53RegMap[writeVar[variable]].fBitSize << RESET; LOG(INFO) << BOLDBLUE << "variable = " << BOLDYELLOW <<  variable <<" "<< BOLDYELLOW <<  fitStart[variable] <<" "<<
                        // BOLDYELLOW <<  fitEnd[variable] << cChip << RESET; LOG(INFO) << BOLDBLUE << "variable = " << BOLDYELLOW <<  variable <<" "<< BOLDYELLOW <<  DNLcode[variable][0] << " " <<
                        // BOLDYELLOW << " " << BOLDYELLOW <<  VMUXvolt[variable][0] << " " << cChip << RESET; LOG(INFO) << BOLDBLUE << "variable = " BOLDYELLOW <<  writeVar[variable] << " " << cChip
                        // << RESET;
                        auto spad1 = new TPad("spad1", "The first subpad", 0, 0.3, 1, 1);
                        auto spad2 = new TPad("spad2", "The second subpad", 0, 0.15, 1, 0.3);
                        auto spad3 = new TPad("spad3", "The third subpad", 0, 0, 1, 0.15);
                        spad1->Draw();
                        spad2->Draw();
                        spad3->Draw();
                        spad1->cd();
                        // TGraph *linear = new TGraph (pow(2.0,pRD53RegMap[writeVar[variable]].fBitSize), VMUXvolt[variable], DACcode[variable]);
                        TGraph* linear = new TGraph(pow(2.0, pRD53RegMap[writeVar[variable]].fBitSize), DACcode[variable], VMUXvolt[variable]);
                        // linear->SetTitle("Linear Graph;Voltage;DAC Code");
                        linear->SetTitle("CAL_HI;Injected DAC Code;Calibration DAC Output Voltage [V]");
                        linear->SetName(writeVar[variable].c_str());
                        // linear->Fit("pol1","","",fitStart[variable],fitEnd[variable]);
                        // gStyle->SetOptFit();
                        // gStyle->SetFitFormat(".2g");
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
                        canvas->Print("Results/DAC_plots.pdf");
                        linear->Write();
                        dnlGraph->Write();
                        inlGraph->Write();
                    }
                }
    file->Write();
    canvas->Print("Results/DAC_plots.pdf]");
}
