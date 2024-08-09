#include <string>
#include <iostream>
#include <vector>

using namespace std;

//insert string x into string y at index position
string insert(const string& x, const string& y, size_t position) {
    if (position > y.length()) {
        return y;
    }else{
        string result = y;
        result.insert(position, x);
        return result;
    }
}

/*
createROOTFile("/home/cmsTkUser/Ph2_ACF_GUI/data/FWDRVS-Bias.root")
createROOTFile("/home/cmsTkUser/Ph2_ACF_GUI/data/Crosstalk.root")
*/
void createROOTFile(const string& path) {
    TFile outputFile(path.c_str(), "RECREATE");
}

/*
bias("../../data/Run000073_SCurve.root", "../../data/Run000074_SCurve.root", "../../data/FWDRVS-Bias.root", const_cast<int*>(std::array<int, 4>{0, 0, 0, 12}.data()))

Example:
bias("path/to/scurve/reverse.root", "path/to/scurve/forward.root", "output/directory/file.root", const_cast<int*>(std::array<int, 4>{boardID, ogID, hybridID, chipID}.data()))

Preconditions:
- Assumes that the output file is already created. I don't know if it 
actually matters but since this is designed to be looped over, I thought it best to
create the root file outside the function and then update it.
*/
void bias(const string& scurve1, const string& scurve2, const string& outputDir, const int* path) {
    string boardID = to_string(path[0]);
    string opticalgroupID = to_string(path[1]);
    string hybridID = to_string(path[2]);
    string chipID = to_string(path[3]);

    gStyle->SetOptStat(0);
    gROOT->SetBatch(kTRUE);
    gROOT->ForceStyle();

    TFile f_neg_bias(scurve1.c_str());
    TFile f_pos_bias(scurve2.c_str());

    string baseDir = "Detector/Board_" + boardID + "/OpticalGroup_" + opticalgroupID + "/Hybrid_" + hybridID + "/Chip_" + chipID;

    // Function to create change in 2D plot
    auto createChangePlot = [&](const string& histName) {
        string shortBaseDir = "D_B(" + boardID + ")_O(" + opticalgroupID + ")_H(" + hybridID + ")_" + histName + "_Chip(" + chipID + ")";

        TCanvas* neg_scurve = static_cast<TCanvas*>(f_neg_bias.Get((baseDir + "/" + shortBaseDir).c_str()));
        TCanvas* pos_scurve = static_cast<TCanvas*>(f_pos_bias.Get((baseDir + "/" + shortBaseDir).c_str()));

        TH2F* h_neg = static_cast<TH2F*>(neg_scurve->GetPrimitive(shortBaseDir.c_str()));
        TH2F* h_pos = static_cast<TH2F*>(pos_scurve->GetPrimitive(shortBaseDir.c_str()));

        TH2F* cloned_hist = static_cast<TH2F*>(h_neg->Clone(insert("d", shortBaseDir, 17).c_str()));
        cloned_hist->SetTitle(("Change in " + histName + " Histograms").c_str());

        // Calculate the absolute value of the difference
        int nBinsX = cloned_hist->GetNbinsX();
        int nBinsY = cloned_hist->GetNbinsY();

        for (int binX = 1; binX <= nBinsX; ++binX) {
            for (int binY = 1; binY <= nBinsY; ++binY) {
                float negValue = h_neg->GetBinContent(binX, binY);
                float posValue = h_pos->GetBinContent(binX, binY);
                float absDiff = abs(negValue - posValue);
                cloned_hist->SetBinContent(binX, binY, absDiff);
            }
        }

        TCanvas* c_d2d = new TCanvas(shortBaseDir.c_str(), ("Change in " + histName + " Histograms").c_str());
        c_d2d->SetLogz();
        cloned_hist->Draw("colz");

        return c_d2d;
    };

    TCanvas* c_dnoise2d = createChangePlot("Noise2D");
    TCanvas* c_dthreshold2d = createChangePlot("Threshold2D");

    // Save histograms to ROOT file
    TFile file(outputDir.c_str(), "UPDATE");
    if (file.IsZombie()) {
        cerr << "Error: Could not open file " << outputDir << endl;
        return;
    }

    TDirectory* dir = &file;

    vector<string> items = {
        "Detector",
        "Board_" + boardID,
        "OpticalGroup_" + opticalgroupID,
        "Hybrid_" + hybridID,
        "Chip_" + chipID
    };

    for (const auto& item : items) {
        const char* c_str_item = item.c_str();
        TDirectory* subdir = dir->GetDirectory(c_str_item);
        if (!subdir) {
            subdir = dir->mkdir(c_str_item);
        }
        subdir->cd();
        dir = subdir;
    }

    dir->WriteTObject(c_dnoise2d, ("D_B(" + boardID + ")_O(" + opticalgroupID + ")_H(" + hybridID + ")_dNoise2D_Chip(" + chipID + ")").c_str());
    dir->WriteTObject(c_dthreshold2d, ("D_B(" + boardID + ")_O(" + opticalgroupID + ")_H(" + hybridID + ")_dThreshold2D_Chip(" + chipID + ")").c_str());

    file.Close();
}

/*
xtalk("../../Ph2_ACF/test/Results/Run000019_PixelAlive.root", "../../Ph2_ACF/test/Results/Run000020_PixelAlive.root", "../../Ph2_ACF/test/Results/Run000021_PixelAlive.root", "../../data/Crosstalk.root", const_cast<int*>(std::array<int, 4>{0, 0, 0, 12}.data()))

Example:
xtalk("path/to/pixelalive.root", "path/to/pixelalive/coupled.root", "path/to/pixelalive/uncoupled.root", "output/directory/file.root", const_cast<int*>(std::array<int, 4>{boardID, ogID, hybridID, chipID}.data()))

Preconditions:
- Assumes that the output file is already created. I don't know if it 
actually matters but since this is designed to be looped over, I thought it best to
create the root file outside the function and then update it.

Original function was written by Arash Jofrehei and is available at
https://github.com/Arash-Jofrehei/randomCodesForMaster/blob/master/xtalk.cpp
*/
void xtalk(const string& pixelalive1, const string& pixelalive2, const string& pixelalive3, const string& outputDir, const int* path) {
    string boardID = to_string(path[0]);
    string opticalgroupID = to_string(path[1]);
    string hybridID = to_string(path[2]);
    string chipID = to_string(path[3]);

    gStyle->SetOptStat(0);
    gROOT->SetBatch(kTRUE);
    gROOT->ForceStyle();

    TFile f_injtype1(pixelalive1.c_str());
    TFile f_injtype5(pixelalive2.c_str());
    TFile f_injtype6(pixelalive3.c_str());

    float alive_eff = 0.5;
    float coupled_eff = 0.3;
    float uncoupled_eff = 0.2;

    string baseDir = "Detector/Board_" + boardID + "/OpticalGroup_" + opticalgroupID + "/Hybrid_" + hybridID + "/Chip_" + chipID;
    string shortBaseDir = "D_B(" + boardID + ")_O(" + opticalgroupID + ")_H(" + hybridID + ")_PixelAlive_Chip(" + chipID + ")";

    TCanvas* c0_pixelalive1 = static_cast<TCanvas*>(f_injtype1.Get((baseDir + "/" + shortBaseDir).c_str()));
    TH2F* h_pixelalive1 = static_cast<TH2F*>(c0_pixelalive1->GetPrimitive(shortBaseDir.c_str()));
    h_pixelalive1->SetTitle("PixelAlive");
    h_pixelalive1->SetName("h_pixelalive2D");
    TCanvas* c0_pixelalive5 = static_cast<TCanvas*>(f_injtype5.Get((baseDir + "/" + shortBaseDir).c_str()));
    TH2F* h_pixelalive5 = static_cast<TH2F*>(c0_pixelalive5->GetPrimitive(shortBaseDir.c_str()));
    h_pixelalive5->SetTitle("PixelAlive - Coupled");
    h_pixelalive5->SetName("h_coupled2D");
    TCanvas* c0_pixelalive6 = static_cast<TCanvas*>(f_injtype6.Get((baseDir + "/" + shortBaseDir).c_str()));
    TH2F* h_pixelalive6 = static_cast<TH2F*>(c0_pixelalive6->GetPrimitive(shortBaseDir.c_str()));
    h_pixelalive6->SetTitle("PixelAlive - Uncoupled");
    h_pixelalive6->SetName("h_uncoupled2D");

    int nColumns = h_pixelalive5->GetXaxis()->GetNbins();
    int nRows = h_pixelalive5->GetYaxis()->GetNbins();

    vector<int> dead_row, dead_col, suspicious_row, suspicious_col, confirmed_row, confirmed_col;
    TH2F* h_suspicious2D = static_cast<TH2F*>(h_pixelalive5->Clone("h_suspicious2D"));
    h_suspicious2D->SetTitle("Suspicious Channels");
    TH2F* h_confirmed2D = static_cast<TH2F*>(h_pixelalive5->Clone("h_confirmed2D"));
    h_confirmed2D->SetTitle("Confirmed Disconnected Channels");

    for (int i = 0; i < nRows; ++i) {
        for (int j = 0; j < nColumns; ++j) {
            bool detectable = true;
            if (h_pixelalive1->GetBinContent(j + 1, i + 1) < alive_eff) {
                dead_row.push_back(i);
                dead_col.push_back(j);
                detectable = false;
            }
            if (detectable) {
                if ((i == 0 && j % 2 == 0) || (i == nRows - 1 && j % 2 == 1)) detectable = false;
            }
            if (detectable && h_pixelalive1->GetBinContent(j + 1, i + 1) > alive_eff && h_pixelalive5->GetBinContent(j + 1, i + 1) < coupled_eff) {
                suspicious_row.push_back(i);
                suspicious_col.push_back(j);
                h_suspicious2D->SetBinContent(j + 1, i + 1, 1);
            } else {
                h_suspicious2D->SetBinContent(j + 1, i + 1, 0.0001);
            }
            if (detectable && h_pixelalive1->GetBinContent(j + 1, i + 1) > alive_eff && h_pixelalive5->GetBinContent(j + 1, i + 1) < coupled_eff && h_pixelalive6->GetBinContent(j + 1, i + 1) < uncoupled_eff) {
                confirmed_row.push_back(i);
                confirmed_col.push_back(j);
                h_confirmed2D->SetBinContent(j + 1, i + 1, 1);
            } else {
                h_confirmed2D->SetBinContent(j + 1, i + 1, 0.0001);
            }
        }
    }

    TPaveText* textResults = new TPaveText(0.1, 0.1, 0.9, 0.9, "NDC");

    textResults->AddText(Form("dead: %d", static_cast<int>(dead_row.size())));
    textResults->AddText(Form("suspicious: %d", static_cast<int>(suspicious_row.size())));
    textResults->AddText(Form("confirmed: %d", static_cast<int>(confirmed_row.size())));

    textResults->SetTextAlign(22);
    textResults->SetTextSize(0.08);
    textResults->SetFillColor(0);
	textResults->SetBorderSize(0);

    auto saveCanvasHist = [](TH2F* hist, const string& title) {
        TCanvas* canvas = new TCanvas(title.c_str(), title.c_str());
        canvas->cd();

        gPad->SetLogz();
        hist->Draw("colz");

        canvas->Update();
        return canvas;
    };

    auto saveCanvasText = [](TPaveText* text, const string& title) {
        TCanvas* canvas = new TCanvas(title.c_str(), title.c_str());
        canvas->cd();

        text->Draw();

        canvas->Update();
        return canvas;
    };

    string nameTemplate = "D_B("+boardID+")_O("+opticalgroupID+")_H("+hybridID+")__Chip("+chipID+")";

    TCanvas* c_pixelalive = saveCanvasHist(h_pixelalive1, insert("PixelAlive", nameTemplate, 17));
    TCanvas* c_coupled = saveCanvasHist(h_pixelalive5, insert("Coupled", nameTemplate, 17));
    TCanvas* c_uncoupled = saveCanvasHist(h_pixelalive6, insert("Uncoupled", nameTemplate, 17));
    TCanvas* c_suspicious = saveCanvasHist(h_suspicious2D, insert("Suspicious", nameTemplate, 17));
    TCanvas* c_confirmed = saveCanvasHist(h_confirmed2D, insert("Confirmed", nameTemplate, 17));
    TCanvas* c_textResults = saveCanvasText(textResults, insert("Results", nameTemplate, 17));

    TFile file(outputDir.c_str(), "UPDATE");
    if (file.IsZombie()) {
        cerr << "Error: Could not open file " << outputDir << endl;
        return;
    }

    TDirectory* dir = &file;

    vector<string> items = {
        "Detector",
        "Board_" + boardID,
        "OpticalGroup_" + opticalgroupID,
        "Hybrid_" + hybridID,
        "Chip_" + chipID
    };

    for (const auto& item : items) {
        const char* c_str_item = item.c_str();
        TDirectory* subdir = dir->GetDirectory(c_str_item);
        if (!subdir) {
            subdir = dir->mkdir(c_str_item);
        }
        subdir->cd();
        dir = subdir;
    }

    for (const auto& canvas : { c_pixelalive, c_coupled, c_uncoupled, c_suspicious, c_confirmed, c_textResults }) {
        dir->WriteTObject(canvas, canvas->GetName());
    }

    file.Close();
}
