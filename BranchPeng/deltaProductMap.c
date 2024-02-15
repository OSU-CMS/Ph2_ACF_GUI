void deltaProductMap()
{
    const int nBinsX = 434;
    const int nBinsY = 337;

    TCanvas *canvas = new TCanvas("canvas", "D_B(0)_O(0)_H(0)_DeltaThreshold * DeltaNoise Map_Chip(12)", 0, 67, 700, 500);

    TH2F *hist = new TH2F("D_B(0)_O(0)_H(0)_#DeltaThreshold * #DeltaNoise Map_Chip(12)", "D_B(0)_O(0)_H(0)_#DeltaThreshold * #DeltaNoise Map_Chip(12)", nBinsX, 1, nBinsX + 1, nBinsY, 1, nBinsY + 1);

    ifstream dataFile("/Users/branchinpyjamas/Desktop/Pixels/workFiles/2DdeltaHist/processedDataPoints.txt");

    // Reading processed data structure to draw the 2D histogram
    double x, y, product;
    while (dataFile >> x >> y >> product) {
        // Check if bin indices are within the valid range
        if (x < 1 || x > nBinsX || y < 1 || y > nBinsY) {
            std::cerr << "Warning: Invalid bin indices. x: " << x << ", y: " << y << std::endl;
            continue;
        }

        hist->Fill(x, y, product);
    }
    dataFile.close();


    hist->Draw("COLZ");

    // Palette settings
    TPaletteAxis *palette = new TPaletteAxis(434.7,2.076121e-15,459,336,hist);
    palette->SetNdivisions(510);
    palette->SetAxisColor(1);
    palette->SetLabelColor(1);
    palette->SetLabelFont(42);
    palette->SetLabelOffset(0.005);
    palette->SetLabelSize(0.035);
    palette->SetMaxDigits(0);
    palette->SetTickLength(0.03);
    palette->SetTitleOffset(1);
    palette->SetTitleSize(0.035);
    palette->SetTitleColor(1);
    palette->SetTitleFont(42);
    palette->SetTitle("");
    hist->GetListOfFunctions()->Add(palette,"br");

    Int_t ci = TColor::GetColor("#000099");
    hist->SetLineColor(ci);

    hist->GetXaxis()->SetTitle("Column");
    hist->GetXaxis()->SetLabelFont(42);
    hist->GetXaxis()->SetTitleOffset(1);
    hist->GetXaxis()->SetTitleFont(42);
    hist->GetYaxis()->SetTitle("Row");
    hist->GetYaxis()->SetLabelFont(42);
    hist->GetYaxis()->SetTitleFont(42);
    hist->GetZaxis()->SetLabelFont(42);
    hist->GetZaxis()->SetTitleOffset(1);
    hist->GetZaxis()->SetTitleFont(42);

    // Statistics box
    TPaveStats *ptstats = new TPaveStats(0.78,0.695,0.98,0.935,"brNDC");
    ptstats->SetName("stats");
    ptstats->SetBorderSize(1);
    ptstats->SetFillColor(0);
    ptstats->SetTextAlign(12);
    ptstats->SetTextFont(42);
    ptstats->SetOptStat(1111);
    ptstats->SetOptFit(0);
    hist->GetListOfFunctions()->Add(ptstats);
    ptstats->Draw();
    ptstats->SetParent(hist);

    canvas->Update();
}

