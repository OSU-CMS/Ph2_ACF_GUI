#include <iostream>

void extractPlots(TString plotFile, TString plotName, TString outplotFile)
{
    TFile * inputFile = new TFile(plotFile, "read");
    TCanvas * c = (TCanvas*)inputFile->Get(plotName);
    c->SaveAs(outplotFile);
}