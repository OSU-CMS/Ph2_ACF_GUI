void deltaMap()
{
    TCanvas *canvas = new TCanvas("D_B(0)_O(0)_H(0)_deltaThreshold & Noise2D_Chip(12)", "D_B(0)_O(0)_H(0)_DeltaThreshold & Noise Map_Chip(12);#DeltaThreshold;#DeltaNoise", 800, 600);
    canvas->SetGrid();

    ifstream dataFile("/Users/branchinpyjamas/Desktop/Pixels/workFiles/2DdeltaHist/dataPoints.txt");

    // Initialize min and max values for X and Y
    double minX = DBL_MAX, maxX = -DBL_MAX;
    double minY = DBL_MAX, maxY = -DBL_MAX;
    double x, y;

    // First pass to find min and max values
    while (dataFile >> x >> y) {
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }

    // Rewind the file to read again
    dataFile.clear();
    dataFile.seekg(0, ios::beg);

    // Define the histogram with dynamic ranges
    int nBinsX = 20;
    int nBinsY = 20;
    TH2F *hist = new TH2F("(#DeltaThreshold, #DeltaNoise)", "D_B(0)_O(0)_H(0)_#DeltaThreshold & Noise Map_Chip(12);#DeltaThreshold;#DeltaNoise", nBinsX, minX, maxX, nBinsY, minY, maxY);

    while (dataFile >> x >> y) {
        hist->Fill(x, y);
    }
    dataFile.close();

    gStyle->SetOptStat(1); // Display statistics box with histogram details
    gStyle->SetPalette(kRainBow); // Use a color palette for Z-axis
    hist->SetContour(100); // Fine contours for smoother color transition

    hist->Draw("COLZ"); // Draw with color palette and Z-axis as color

    canvas->Update();
}




