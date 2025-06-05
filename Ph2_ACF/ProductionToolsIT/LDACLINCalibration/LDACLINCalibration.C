// ROOT macro to find the best LDAC_LIN value for threshold tuning after running the shell script LDACLINMeasurement.sh
// Created by Marijus Ambrozas (marijus.ambrozas@cern.ch)
// Aug. 30 2021

#include <TCanvas.h>
#include <TFile.h>
#include <TGraph.h>
#include <TH1D.h>
#include <TROOT.h>
#include <TString.h>
#include <TSystemDirectory.h>
#include <TSystemFile.h>
#include <fstream>
#include <iostream>
#include <vector>

// Useful function to compare strings as numbers
bool cmp(TString a, TString b) { return a.Atoi() < b.Atoi(); }

void LDACLINCalibration(TString measurementSet = "ChipX_1", TString option = "")
{
    TString Option = option;
    Option.ToUpper();
    if(Option != "" && Option != "S" && Option != "SHORT" && Option != "L" && Option != "LONG")
    {
        std::cerr << "Error: Wrong option! Received \"" << Option << "\" while it should be empty, \"L\", \"LONG\", \"S\", or \"SHORT\"." << std::endl;
        return;
    }
    if(measurementSet == "")
    {
        std::cerr << "Error: No measurementSet specified!" << std::endl;
        return;
    }

    // Getting the measured LDAC_LIN values by checking the list of root files
    std::vector<TString> filenames_threqu;
    std::vector<TString> filenames_scurve;
    std::vector<int>     values_threqu;
    std::vector<int>     values_scurve;
    TString              dirname = "./Results/" + measurementSet + "/";
    TSystemDirectory     dir(dirname, dirname);
    TList*               files = dir.GetListOfFiles();
    if(files)
    {
        TSystemFile* file;
        TString      fname;
        TIter        next(files);
        while((file = (TSystemFile*)next()))
        {
            fname = file->GetName();
            if(!file->IsDirectory() && fname.EndsWith(".root"))
            {
                if(fname.BeginsWith("LDACLIN"))
                {
                    fname.ReplaceAll("LDACLIN_", "");
                    fname.ReplaceAll(".root", "");
                    filenames_threqu.push_back((TString)fname);
                    values_threqu.push_back(fname.Atoi());
                }
                else if(fname.BeginsWith("scurve_LDACLIN"))
                {
                    fname.ReplaceAll("scurve_LDACLIN_", "");
                    fname.ReplaceAll(".root", "");
                    filenames_scurve.push_back(fname);
                    values_scurve.push_back(fname.Atoi());
                }
            }
        }
    }
    if(filenames_threqu.size() < 7)
    {
        std::cerr << "Error: Less than 7 threqu root files found! Please produce more measurements before running this calibration." << std::endl;
        return;
    }
    if(option != "S" && option != "SHORT" && filenames_scurve.size() < 7)
    {
        std::cerr << "Error: Less than 7 scurve root files found! Please produce more measurements before running this calibration." << std::endl;
        return;
    }

    // Sorting the vectors in case GetListOfFiles does not provide them in correct order
    sort(values_threqu.begin(), values_threqu.end(), less<int>());
    sort(filenames_threqu.begin(), filenames_threqu.end(), cmp);
    sort(values_scurve.begin(), values_scurve.end(), less<int>());
    sort(filenames_scurve.begin(), filenames_scurve.end(), cmp);

    TH1D*   h_efficiency[values_threqu.size()];
    TH1D*   h_ThrSpread[values_scurve.size()];
    TGraph* g_effSigma_vs_LDACLIN = new TGraph(values_threqu.size());
    TGraph* g_effTails_vs_LDACLIN = new TGraph(values_threqu.size());
    TGraph* g_thrSigma_vs_LDACLIN = new TGraph(values_scurve.size());

    // If doing the regular version (s-curve only for 7 points)
    std::vector<double> values_best;
    if(Option == "")
    {
        ifstream t_in;
        t_in.open("./Results/" + measurementSet + "/LDACLINCandidates.txt");
        if(t_in.is_open())
        {
            double read;
            while(t_in >> read) values_best.push_back(read);
            t_in.close();
        }
        else
        {
            std::cerr << "Error: could not open the file Results/" + measurementSet + "/LDACLINCandidates.txt!" << std::endl;
            std::cout << "Maybe you need to create it first? If so, run the macro with the \"S\" argument" << std::endl;
            return;
        }
    }

    // Efficiency spread vs LDAC_LIN
    for(int i = 0; i < (int)(values_threqu.size()); i++)
    {
        // Efficiency measurement from threshold equalization
        TFile* f_eff = new TFile("./Results/" + measurementSet + "/LDACLIN_" + filenames_threqu[i] + ".root", "READ");
        if(!f_eff->IsOpen())
        {
            std::cerr << "No file ./Results/" + measurementSet + "/LDACLIN_" + filenames_threqu[i] + ".root found! Quitting..." << std::endl;
            return;
        }

        TCanvas* c_read_eff = ((TCanvas*)(f_eff->Get("Detector/Board_0/OpticalGroup_0/Hybrid_0/Chip_0/D_B(0)_O(0)_H(0)_ThrEqualization_Chip(0)")));
        TH1D*    h_read_eff = ((TH1D*)(c_read_eff->GetPrimitive("D_B(0)_O(0)_H(0)_ThrEqualization_Chip(0)")));
        Double_t effSigma = 0, int_effTails = 0;

        // Efficiency Sigma
        effSigma = h_read_eff->GetStdDev();
        g_effSigma_vs_LDACLIN->SetPoint(i, values_threqu[i], effSigma);

        // First and last bin
        int_effTails = (h_read_eff->GetBinContent(1) + h_read_eff->GetBinContent(101)) / h_read_eff->Integral();
        g_effTails_vs_LDACLIN->SetPoint(i, values_threqu[i], int_effTails);

        h_efficiency[i] = ((TH1D*)(h_read_eff->Clone("Efficiency_LDAC_LIN_" + filenames_threqu[i])));
        h_efficiency[i]->SetDirectory(0);

        f_eff->Close();
    }

    // Threshold spread vs LDAC_LIN
    if(Option != "S" && Option != "SHORT")
    {
        for(int i = 0; i < (int)(values_scurve.size()); i++)
        {
            // Only 7 points if not running "LONG" mode
            if(Option == "" && std::find(values_best.begin(), values_best.end(), values_scurve[i]) == values_best.end()) continue;

            // Threshold spread measurement
            TFile* f_Scurve = new TFile("./Results/" + measurementSet + "/scurve_LDACLIN_" + filenames_scurve[i] + ".root", "READ");
            if(!f_Scurve->IsOpen())
            {
                std::cerr << "No file ./Results/" + measurementSet + "/scurve_LDACLIN_" + filenames_scurve[i] + ".root found! Quitting..." << std::endl;
                return;
            }

            TCanvas* c_read_thr = ((TCanvas*)(f_Scurve->Get("Detector/Board_0/OpticalGroup_0/Hybrid_0/Chip_0/D_B(0)_O(0)_H(0)_Threshold1D_Chip(0)")));
            TH1D*    h_read_thr = ((TH1D*)(c_read_thr->GetPrimitive("D_B(0)_O(0)_H(0)_Threshold1D_Chip(0)")));
            Double_t thrSigma   = 0;

            // Threshold sigma
            thrSigma = h_read_thr->GetStdDev();
            g_thrSigma_vs_LDACLIN->SetPoint(i, values_scurve[i], thrSigma);
            h_ThrSpread[i] = ((TH1D*)(h_read_thr->Clone("ThrSpread_LDAC_LIN_" + filenames_scurve[i])));
            h_ThrSpread[i]->SetDirectory(0);

            f_Scurve->Close();
        }
    }

    // For saving all the plots
    TFile* f_out = new TFile("./Results/" + measurementSet + "/ThrTuning_vs_LDAC_LIN.root", "RECREATE");
    if(!f_out->IsOpen())
    {
        std::cerr << "Error: failed to open the output file \"Results/" + measurementSet + "/ThrTuning_vs_LDAC_LIN.root\"!" << std::endl;
        return;
    }
    f_out->cd();
    // Saving relevant histograms and finding best LDAC_LIN
    Double_t best_estimator = 9999999, best_LDAC_LIN = -1, best_i = -1;
    for(Int_t i = 0; i < (int)(values_threqu.size()); i++)
    {
        // Saving efficiency
        h_efficiency[i]->Write();

        // Searching for best LDAC_LIN
        if(Option == "S" || Option == "SHORT")
        {
            Double_t estimator, LDAC_LIN_val;
            g_effSigma_vs_LDACLIN->GetPoint(i, LDAC_LIN_val, estimator);
            if(estimator < best_estimator)
            {
                best_estimator = estimator;
                best_LDAC_LIN  = LDAC_LIN_val;
                best_i         = i;
            }
        }
    }

    for(Int_t i = 0; i < (int)(values_scurve.size()); i++)
    {
        // Only 7 points if not running "LONG" mode
        if(Option == "" && std::find(values_best.begin(), values_best.end(), values_scurve[i]) == values_best.end()) continue;

        if(Option != "S" && Option != "SHORT")
        {
            // Saving threshold spread (depends on mode)
            h_ThrSpread[i]->Write();

            // Searching for best LDAC_LIN
            Double_t estimator, LDAC_LIN_val;
            g_thrSigma_vs_LDACLIN->GetPoint(i, LDAC_LIN_val, estimator);
            if(estimator < best_estimator)
            {
                best_estimator = estimator;
                best_LDAC_LIN  = LDAC_LIN_val;
                best_i         = i;
            }
        }
    }

    std::cout << "Best LDAC_LIN value: " << best_LDAC_LIN << std::endl;

    // Making the output text file to hint where to measure s-curves
    if(Option == "S" || Option == "SHORT")
    {
        ofstream t_out;
        t_out.open("./Results/" + measurementSet + "/LDACLINCandidates.txt");
        if(t_out.is_open())
        {
            Int_t i_min = best_i - 2 >= 0 ? best_i - 2 : 0;
            Int_t i_max = best_i + 4 < (int)(values_threqu.size()) ? best_i + 4 : (int)(values_threqu.size() - 1);
            for(Int_t i = i_min; i <= i_max; i++) t_out << values_threqu[i] << std::endl;
            t_out.close();
        }
        else
            std::cerr << "Error: failed to open the output file \"Results/" + measurementSet + "/LDACLINCandidates.txt\"!" << std::endl;
    }

    // Removing empty points if only 7 s-curve points exist
    if(Option == "")
    {
        for(Int_t i = (int)(values_scurve.size() - 1); i >= 0; i--)
        {
            if(std::find(values_best.begin(), values_best.end(), values_scurve[i]) == values_best.end()) g_thrSigma_vs_LDACLIN->RemovePoint(i);
        }
    }

    // Drawing
    g_effSigma_vs_LDACLIN->SetName("g_effSigma_vs_LDACLIN");
    g_effSigma_vs_LDACLIN->SetTitle("Efficiency spread vs LDAC_LIN");
    g_effSigma_vs_LDACLIN->SetMarkerStyle(kFullDotLarge);
    g_effSigma_vs_LDACLIN->GetXaxis()->SetTitle("LDAC_LIN");
    g_effSigma_vs_LDACLIN->GetYaxis()->SetTitle("Efficiency spread sigma");
    g_effSigma_vs_LDACLIN->GetXaxis()->SetNoExponent(1);
    g_effSigma_vs_LDACLIN->GetXaxis()->SetMoreLogLabels(1);
    g_effSigma_vs_LDACLIN->GetXaxis()->SetTitleOffset(1.2);
    g_effSigma_vs_LDACLIN->GetYaxis()->SetTitleOffset(1.5);
    TCanvas* c_effSigma_vs_LDACLIN = new TCanvas("c_effSigma_vs_LDACLIN", "c_effSigma_vs_LDACLIN", 800, 800);
    c_effSigma_vs_LDACLIN->SetMargin(0.12, 0.08, 0.12, 0.08);
    c_effSigma_vs_LDACLIN->SetLogx();
    c_effSigma_vs_LDACLIN->SetGridx();
    c_effSigma_vs_LDACLIN->SetGridy();
    g_effSigma_vs_LDACLIN->Draw();
    c_effSigma_vs_LDACLIN->Update();

    g_effTails_vs_LDACLIN->SetName("g_effTails_vs_LDACLIN");
    g_effTails_vs_LDACLIN->SetTitle("% of pixels in efficiency tails vs LDAC_LIN");
    g_effTails_vs_LDACLIN->SetMarkerStyle(kFullDotLarge);
    g_effTails_vs_LDACLIN->GetXaxis()->SetTitle("LDAC_LIN");
    g_effTails_vs_LDACLIN->GetYaxis()->SetTitle("% of pixels in efficiency tails (first and last bin)");
    g_effTails_vs_LDACLIN->GetXaxis()->SetNoExponent(1);
    g_effTails_vs_LDACLIN->GetXaxis()->SetMoreLogLabels(1);
    g_effTails_vs_LDACLIN->GetXaxis()->SetTitleOffset(1.2);
    g_effTails_vs_LDACLIN->GetYaxis()->SetTitleOffset(1.5);
    TCanvas* c_effTails_vs_LDACLIN = new TCanvas("c_effTails_vs_LDACLIN", "c_effTails_vs_LDACLIN", 800, 800);
    c_effTails_vs_LDACLIN->SetMargin(0.12, 0.08, 0.12, 0.08);
    c_effTails_vs_LDACLIN->SetLogx();
    c_effTails_vs_LDACLIN->SetGridx();
    c_effTails_vs_LDACLIN->SetGridy();
    g_effTails_vs_LDACLIN->Draw();
    c_effTails_vs_LDACLIN->Update();
    if(Option != "S" && Option != "SHORT")
    {
        g_thrSigma_vs_LDACLIN->SetName("g_thrSigma_vs_LDACLIN");
        g_thrSigma_vs_LDACLIN->SetTitle("Threshold spread vs LDAC_LIN");
        g_thrSigma_vs_LDACLIN->SetMarkerStyle(kFullDotLarge);
        g_thrSigma_vs_LDACLIN->GetXaxis()->SetTitle("LDAC_LIN");
        g_thrSigma_vs_LDACLIN->GetYaxis()->SetTitle("Threshold spread sigma");
        g_thrSigma_vs_LDACLIN->GetXaxis()->SetNoExponent(1);
        g_thrSigma_vs_LDACLIN->GetXaxis()->SetMoreLogLabels(1);
        g_thrSigma_vs_LDACLIN->GetXaxis()->SetTitleOffset(1.2);
        g_thrSigma_vs_LDACLIN->GetYaxis()->SetTitleOffset(1.5);
        TCanvas* c_thrSigma_vs_LDACLIN = new TCanvas("c_thrSigma_vs_LDACLIN", "c_thrSigma_vs_LDACLIN", 800, 800);
        c_thrSigma_vs_LDACLIN->SetMargin(0.12, 0.08, 0.12, 0.08);
        c_thrSigma_vs_LDACLIN->SetLogx();
        c_thrSigma_vs_LDACLIN->SetGridx();
        c_thrSigma_vs_LDACLIN->SetGridy();
        g_thrSigma_vs_LDACLIN->Draw();
        c_thrSigma_vs_LDACLIN->Update();
    }

    // Saving the graphs
    g_effSigma_vs_LDACLIN->Write();
    g_effTails_vs_LDACLIN->Write();
    if(Option != "S" && Option != "SHORT") g_thrSigma_vs_LDACLIN->Write();
    f_out->Close();
}
