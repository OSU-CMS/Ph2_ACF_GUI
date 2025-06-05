#include "SignalScanFit.h"
#ifdef __USE_ROOT__

void SignalScanFit::Initialize()
{
#ifdef __USE_ROOT__
    fDQMHistogramSignalScanFit.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif

    // To read the SignalScanFit-specific stuff
    parseSettings();

    // Initialize all the plots
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                fVCthNbins = int((1024 / double(fSignalScanStep)) + 1);
                fVCthMax   = double((fVCthNbins * fSignalScanStep) - (double(fSignalScanStep) / 2.)); //"center" de bins
                fVCthMin   = 0. - (double(fSignalScanStep) / 2.);

                // Make a canvas for the live plot
                uint32_t cHybridId = cHybrid->getId();
                fNCbc              = 0; // cHybrid->getNChip();
                for(auto cCbc: *cHybrid) { fNCbc = (cCbc->getId() >= fNCbc) ? (cCbc->getId() + 1) : fNCbc; }
                TCanvas* ctmpCanvas = new TCanvas(Form("c_online_canvas_fe%d", cHybridId), Form("FE%d  Online Canvas", cHybridId));
                fCanvasMap[cHybrid] = ctmpCanvas;

                // Histograms
                TString  cName = Form("h_hybrid_thresholdScan_Fe%d", cHybridId);
                TObject* cObj  = gROOT->FindObject(cName);

                if(cObj) delete cObj;

                // 2D-plot with all the channels on the x-axis, Vcth on the y-axis and #clusters on the z-axis.
                cName             = Form("h_hybrid_thresholdScan_SingleStripClusters_S0_Fe%d", cHybridId);
                TH2D* cSignalEven = new TH2D(cName,
                                             "Signal threshold vs channel [half strips] ; Even Sensor Strip [half "
                                             "strips] ; Threshold; # of Hits",
                                             fNCbc * NCHANNELS,
                                             -0.5,
                                             fNCbc * NCHANNELS - 0.5,
                                             fVCthNbins,
                                             fVCthMin,
                                             fVCthMax);
                bookHistogram(cHybrid, "SingleStripClusters_S0", cSignalEven);

                cName            = Form("h_hybrid_thresholdScan_SingleStripClusters_S1_Fe%d", cHybridId);
                TH2D* cSignalOdd = new TH2D(cName,
                                            "Signal threshold vs channel [half strips] ; Odd Sensor Strip [half strips] ; Threshold; # of Hits",
                                            fNCbc * NCHANNELS,
                                            -0.5,
                                            fNCbc * NCHANNELS - 0.5,
                                            fVCthNbins,
                                            fVCthMin,
                                            fVCthMax);
                bookHistogram(cHybrid, "SingleStripClusters_S1", cSignalOdd);

                // 2D-plot with all the channels on the x-axis, Vcth on the y-axis and #clusters on the z-axis.
                cName             = Form("h_hybrid_thresholdScan_Fe%d", cHybridId);
                TH2D* cSignalHist = new TH2D(cName, "Signal threshold vs channel ; Channel # ; Threshold; # of Hits", fNCbc * NCHANNELS, -0.5, fNCbc * NCHANNELS - 0.5, fVCthNbins, fVCthMin, fVCthMax);
                bookHistogram(cHybrid, "hybrid_signal", cSignalHist);

                // 2D-plot with cluster width on the x-axis, Vcth on y-axis, counts of certain clustersize on z-axis.
                TH2D* cVCthClusterSizeHist = new TH2D(Form("h_hybrid_clusterSize_per_Vcth_Fe%d", cHybridId),
                                                      "Cluster size vs Vcth ; Cluster size [strips] ; Threshold [Vcth] ; # clusters",
                                                      15,
                                                      -0.5,
                                                      14.5,
                                                      fVCthNbins,
                                                      fVCthMin,
                                                      fVCthMax);
                bookHistogram(cHybrid, "vcth_ClusterSize", cVCthClusterSizeHist);

                // 1D-plot with the number of triggers per VCth
                TProfile* cNumberOfTriggers = new TProfile(
                    Form("h_hybrid_totalNumberOfTriggers_Fe%d", cHybridId), Form("Total number of triggers received ; Threshold [Vcth] ; Number of triggers"), fVCthNbins, fVCthMin, fVCthMax);
                bookHistogram(cHybrid, "number_of_triggers", cNumberOfTriggers);

                // 1D-plot with the timeout value per VCth
                TH1D* cNclocks = new TH1D(Form("h_hybrid_nClocks_Fe%d", cHybridId),
                                          Form("Number of clock cycles spent at each Vcth value ; Threshold [Vcth] ; "
                                               "Number of clocks to wait"),
                                          fVCthNbins,
                                          fVCthMin,
                                          fVCthMax);
                bookHistogram(cHybrid, "number_of_clocks", cNclocks);

                uint32_t cCbcCount = 0;
                uint32_t cCbcIdMax = 0;

                for(auto cCbc: *cHybrid)
                {
                    uint32_t cCbcId = cCbc->getId();
                    cCbcCount++;

                    if(cCbcId > cCbcIdMax) cCbcIdMax = cCbcId;

                    TString cHistname;
                    TH1D*   cHist;
                    TH2D*   cHist2D;

                    cHistname = Form("Fe%dCBC%d_Hits_even", cHybridId, cCbcId);
                    cHist     = new TH1D(cHistname, Form("%s ; Threshold [Vcth] ; Number of hits", cHistname.Data()), fVCthNbins, fVCthMin, fVCthMax);
                    bookHistogram(cCbc, "Cbc_Hits_even", cHist);

                    cHistname = Form("Fe%dCBC%d_Hits_odd", cHybridId, cCbcId);
                    cHist     = new TH1D(cHistname, Form("%s ; Threshold [Vcth] ; Number of hits", cHistname.Data()), fVCthNbins, fVCthMin, fVCthMax);
                    bookHistogram(cCbc, "Cbc_Hits_odd", cHist);

                    cHistname = Form("Fe%dCBC%d_Clusters2D_even", cHybridId, cCbcId);
                    cHist2D   = new TH2D(cHistname, Form("%s ; Threshold [Vcth] ; Cluster Size [strips];Number of clusters", cHistname.Data()), fVCthNbins, fVCthMin, fVCthMax, 20, 0 - 0.5, 20 - 0.5);
                    bookHistogram(cCbc, "Cbc_Clusters2D_even", cHist2D);

                    cHistname = Form("Fe%dCBC%d_Clusters_even", cHybridId, cCbcId);
                    cHist     = new TH1D(cHistname, Form("%s ; Threshold [Vcth] ; Number of clusters", cHistname.Data()), fVCthNbins, fVCthMin, fVCthMax);
                    bookHistogram(cCbc, "Cbc_Clusters_even", cHist);

                    cHistname = Form("Fe%dCBC%d_Clusters_odd", cHybridId, cCbcId);
                    cHist     = new TH1D(cHistname, Form("%s ; Threshold [Vcth] ; Number of clusters", cHistname.Data()), fVCthNbins, fVCthMin, fVCthMax);
                    bookHistogram(cCbc, "Cbc_Clusters_odd", cHist);

                    cHistname = Form("Fe%dCBC%d_Clusters2D_odd", cHybridId, cCbcId);
                    cHist2D   = new TH2D(cHistname, Form("%s ; Threshold [Vcth] ; Cluster Size [strips];Number of clusters", cHistname.Data()), fVCthNbins, fVCthMin, fVCthMax, 20, 0 - 0.5, 20 - 0.5);
                    bookHistogram(cCbc, "Cbc_Clusters2D_odd", cHist2D);

                    cHistname = Form("Fe%dCBC%d_ClusterSize_even", cHybridId, cCbcId);
                    bookHistogram(cCbc, "Cbc_ClusterSize_even", cHist);

                    cHistname = Form("Fe%dCBC%d_ClusterSize_odd", cHybridId, cCbcId);
                    bookHistogram(cCbc, "Cbc_ClusterSize_odd", cHist);
                }
            }
        }
    }

    LOG(INFO) << GREEN << "Histograms & Settings initialised." << RESET;
}

void SignalScanFit::ScanSignal(int pSignalScanLength)
{
    // The step scan is +1 for hole mode
    int cVcthDirection = (fHoleMode == 1) ? +1 : -1;

    // Reading the current threshold value
    ThresholdVisitor cVisitor(fReadoutChipInterface);
    this->accept(cVisitor);
    uint16_t cVCth = cVisitor.getThreshold();

    LOG(INFO) << "Programmed VCth value = " << +cVCth << ", the Initial VCth value for the scan = " << fInitialThreshold;

    // Setting the threshold to the start position of the scan, ffInitialThreshold which is TargetVCth in settings file
    cVCth = uint16_t(fInitialThreshold);
    cVisitor.setOption('w');
    cVisitor.setThreshold(cVCth);
    this->accept(cVisitor);
    for(int i = 0; i < pSignalScanLength; i += fSignalScanStep)
    {
        LOG(DEBUG) << BLUE << "Threshold: " << +cVCth << " - Iteration " << i << " - Taking data for x*25ns time (see triggers_to_accept in HWDesciption file.)" << RESET;
        // Take Data for all Boards
        for(auto pBoard: *fDetectorContainer)
        {
            BeBoard* theBoard       = static_cast<BeBoard*>(pBoard);
            uint32_t cTotalEvents   = 0;
            uint32_t cTriggerSource = fBeBoardInterface->ReadBoardReg(theBoard, "fc7_daq_cnfg.fast_command_block.trigger_source"); // check trigger source
            double   cTime          = 0;
            uint32_t cTimeout       = 0;
            if(cTriggerSource == 2)
            {
                // start the trigger FSM
                fBeBoardInterface->Start(theBoard);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                // if timeout is enabled ... then wait here until the trigger FMS is ready
                while(fBeBoardInterface->ReadBoardReg(theBoard, "fc7_daq_stat.fast_command_block.general.fsm_state") != 0) { std::this_thread::sleep_for(std::chrono::microseconds(100)); }
                ReadData(theBoard, false);
                fBeBoardInterface->Stop(theBoard);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                cTimeout = fBeBoardInterface->ReadBoardReg(theBoard, "fc7_daq_cnfg.fast_command_block.triggers_to_accept"); // in units of 25ns periods
                cTime    = cTimeout * 25.0;
            }
            else
            {
                LOG(INFO) << BOLDBLUE << "Generating periodic triggers : sending " << +fNevents << " triggers." << RESET;
                // start the trigger FSM
                fBeBoardInterface->Start(theBoard);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                // if timeout is enabled ... then wait here until the trigger FMS is ready
                uint8_t cCounter = 0;
                while(cCounter < fNevents)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    cCounter += 1;
                }
                fBeBoardInterface->Stop(theBoard);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                ReadData(theBoard, false);
                cTimeout = 10e-3;
                cTime    = fNevents * 1e-3;
            }
            const std::vector<Event*>& cEvents      = GetEvents(); // Get the events and play with them
            double                     cTriggerRate = (cEvents.size()) / cTime;
            cTotalEvents                            = cEvents.size();
            LOG(INFO) << BOLDBLUE << "Vcth: " << +cVCth << ". Recorded " << cTotalEvents << " Events [ average trigger rate " << cTriggerRate << " ]" << RESET;
            // cTotalEvents = std::min(cMaxEvents_toProcess, (uint32_t)cEvents.size());
            int cEventHits     = 0;
            int cEventClusters = 0;
            // Loop over Events from this Acquisition to fill the histos
            int cEventCounter = 0;
            for(auto& cEvent: cEvents)
            {
                // if( (uint32_t)cEventCounter > cMaxEvents_toProcess )
                //    continue;
                for(auto cOpticalGroup: *pBoard)
                {
                    for(auto cHybrid: *cOpticalGroup)
                    {
                        TH1D*     cClocksHist   = static_cast<TH1D*>(getHist(cHybrid, "number_of_clocks"));
                        TH2D*     cClustersS0   = static_cast<TH2D*>(getHist(cHybrid, "SingleStripClusters_S0"));
                        TH2D*     cClustersS1   = static_cast<TH2D*>(getHist(cHybrid, "SingleStripClusters_S1"));
                        TH2D*     cSignalHist   = static_cast<TH2D*>(getHist(cHybrid, "hybrid_signal"));
                        TH2D*     cVcthClusters = static_cast<TH2D*>(getHist(cHybrid, "vcth_ClusterSize"));
                        TProfile* cEventsHist   = static_cast<TProfile*>(getHist(cHybrid, "number_of_triggers"));
                        if(cEventCounter == 0)
                        {
                            cClocksHist->Fill(cVCth, cTimeout);
                            cEventsHist->Fill(cVCth, cTotalEvents);
                        }
                        for(auto cCbc: *cHybrid)
                        {
                            TH1D*     cHitsEvenHist       = dynamic_cast<TH1D*>(getHist(cCbc, "Cbc_Hits_even"));
                            TH1D*     cHitsOddHist        = dynamic_cast<TH1D*>(getHist(cCbc, "Cbc_Hits_odd"));
                            TH1D*     cClustersEvenHist   = dynamic_cast<TH1D*>(getHist(cCbc, "Cbc_Clusters_even"));
                            TH1D*     cClustersOddHist    = dynamic_cast<TH1D*>(getHist(cCbc, "Cbc_Clusters_odd"));
                            TProfile* cClusterSizeEven    = static_cast<TProfile*>(getHist(cCbc, "Cbc_ClusterSize_even"));
                            TProfile* cClusterSizeOdd     = static_cast<TProfile*>(getHist(cCbc, "Cbc_ClusterSize_odd"));
                            TH2D*     cClusters2DEvenHist = dynamic_cast<TH2D*>(getHist(cCbc, "Cbc_Clusters2D_even"));
                            TH2D*     cClusters2DOddHist  = dynamic_cast<TH2D*>(getHist(cCbc, "Cbc_Clusters2D_odd"));
                            auto      cHits               = cEvent->GetHits(cHybrid->getId(), cCbc->getId());
                            LOG(DEBUG) << BOLDBLUE << "Found " << +cHits.size() << " hits in CBC" << +cCbc->getId() << RESET;
                            for(auto cId: cHits)
                            {
                                LOG(DEBUG) << BOLDBLUE << "\t.... Hit found in channel " << +cId.second << " i.e. sensor " << (int)(cId.second % 2) << RESET;
                                // Check which sensor we are on
                                if((int(cId.second) % 2) == 0)
                                    cHitsEvenHist->Fill(cVCth);
                                else
                                    cHitsOddHist->Fill(cVCth);

                                cSignalHist->Fill(cCbc->getId() * NCHANNELS + cId.second, cVCth);
                                cEventHits++;
                            } // end for cId
                            // Fill the cluster histos, use the middleware clustering
                            auto cClusters = static_cast<D19cCic2Event*>(cEvent)->getClusters(cHybrid->getId(), cCbc->getId());
                            cEventClusters += cClusters.size();
                            // Now fill the ClusterWidth per VCth plots:
                            for(auto& cCluster: cClusters)
                            {
                                double cClusterSize = cCluster.fClusterWidth;
                                cVcthClusters->Fill(cClusterSize, cCluster.fClusterWidth); // Cluster size counter
                                uint32_t cStrip = cCluster.getBaricentre() * 2 + cCbc->getId() * 127 * 2;
                                LOG(DEBUG) << BOLDBLUE << "\t " << cClusterSize << " strip cluster found with center in strip " << cStrip << " [half-strips] of sensor " << +cCluster.getSensor()
                                           << RESET;
                                if(cCluster.getSensor() == 0)
                                {
                                    if(cCluster.fClusterWidth == 1) cClustersS0->Fill(cStrip, cVCth);
                                    cClustersEvenHist->Fill(cVCth);
                                    cClusterSizeEven->Fill(cVCth, cClusterSize);
                                    cClusters2DEvenHist->Fill(cVCth, cClusterSize);
                                }
                                else if(cCluster.getSensor() == 1)
                                {
                                    if(cCluster.fClusterWidth == 1) cClustersS1->Fill(cStrip, cVCth);
                                    cClustersOddHist->Fill(cVCth);
                                    cClusterSizeOdd->Fill(cVCth, cClusterSize);
                                    cClusters2DOddHist->Fill(cVCth, cClusterSize);
                                }
                            }
                        }
                    }
                }
                cEventCounter += 1;
            }
        }
        // Now increment the threshold by cVCth+fVcthDirecton*fSignalScanStep
        cVCth += cVcthDirection * fSignalScanStep;
        cVisitor.setOption('w');
        cVisitor.setThreshold(cVCth);
        this->accept(cVisitor);
    }
    // Last but not least, save the results. This also happens in the commissioning.cc but when we only use that some
    // plots do not get saved properly! To be checked!
    SaveResults();
    // Now do the fit if requested, WARNING this fitting procedure is still under construction (N. Deelen / G. Zevi
    // Della Porta)
    //    for ( BeBoard* pBoard : fBoardVector )
    //    {
    //        processCurves ( pBoard, "Cbc_ClusterOccupancy_even" );
    //        processCurves ( pBoard, "Cbc_ClusterOccupancy_odd" );
    //    }
}

//////////////////////////////////////          PRIVATE METHODS             //////////////////////////////////////

void SignalScanFit::updateHists(std::string pHistName, bool pFinal)
{
    for(auto& cCanvas: fCanvasMap)
    {
        // maybe need to declare temporary pointers outside the if condition?
        if(pHistName == "hybrid_signal")
        {
            cCanvas.second->cd();
            TH2D* cTmpHist = dynamic_cast<TH2D*>(getHist(static_cast<Ph2_HwDescription::Hybrid*>(cCanvas.first), pHistName));
            cTmpHist->DrawCopy("colz");
            cCanvas.second->Update();
        }
    }
}

void SignalScanFit::parseSettings()
{
    // now read the settings from the map
    auto cSetting = fSettingsMap.find("Nevents");

    if(cSetting != std::end(fSettingsMap))
        fNevents = boost::any_cast<double>(cSetting->second);
    else
        fNevents = 2000;

    cSetting = fSettingsMap.find("HoleMode");

    if(cSetting != std::end(fSettingsMap))
        fHoleMode = boost::any_cast<double>(cSetting->second);
    else
        fHoleMode = 0;

    cSetting = fSettingsMap.find("SignalScanStep");

    if(cSetting != std::end(fSettingsMap))
        fSignalScanStep = boost::any_cast<double>(cSetting->second);
    else
        fSignalScanStep = 1;

    cSetting = fSettingsMap.find("InitialVcth");

    if(cSetting != std::end(fSettingsMap))
        fInitialThreshold = boost::any_cast<double>(cSetting->second);
    else
        fInitialThreshold = 0x78;

    cSetting = fSettingsMap.find("FitSignal"); // In this case we fit the signal which is an s curve with charge sharing (???)

    if(cSetting != std::end(fSettingsMap))
        fFit = boost::any_cast<double>(cSetting->second);
    else
        fFit = 0;

    // Write a log
    LOG(INFO) << "Parsed the following settings:";
    LOG(INFO) << "	Nevents = " << fNevents;
    LOG(INFO) << "	InitialThreshold = " << fInitialThreshold;
    LOG(INFO) << "	HoleMode = " << int(fHoleMode);
    LOG(INFO) << "	SignalScanStep = " << fSignalScanStep;
    LOG(INFO) << "	Fit the scan = " << int(fFit);
}

// void SignalScanFit::processCurves ( BeBoard *pBoard, std::string pHistName )
//{
//    for ( auto cHybrid : pBoard->fHybridVector )
//    {
//        for ( auto cCbc : cHybrid->fReadoutChipVector )
//        {
//            // This one is not used yet?
//            TProfile* cProf = dynamic_cast<TProfile*> ( getHist ( cCbc, pHistName) );

//            TString clusters(pHistName);
//            clusters.ReplaceAll("Occupancy", "s");
//            TH1D* cClustersHist = dynamic_cast<TH1D*> ( getHist ( cCbc, clusters.Data()) );

//            TString hits(clusters);
//            hits.ReplaceAll("Clusters", "Hits");
//            TH1D* cHitsHist = dynamic_cast<TH1D*> ( getHist ( cCbc, hits.Data()) );

//            // Make the clusterSize histos
//            TString size(clusters);
//            size.ReplaceAll("Clusters", "ClusterSize");
//            TH1D* cClusterSizeHist = dynamic_cast<TH1D*> ( getHist ( cCbc, size.Data()) );
//            for (int i = 1; i <= cClustersHist->GetNbinsX(); i++)
//            {
//                if (cClustersHist->GetBinContent(i)>0)
//                cClusterSizeHist->SetBinContent(i, cHitsHist->GetBinContent(i) / cClustersHist->GetBinContent(i));
//                else
//                cClusterSizeHist->SetBinContent(i, 0);
//            }

//            // Make the differential histo
//            // Do this with the histogram, not the profile
//            this->differentiateHist (cCbc, clusters.Data());
//            // Only do this if requested? Yes, see SignalScan and fFit setting!
//            if ( fFit ) this->fitHist (cCbc, pHistName);
//        }
//    }
//}

// void SignalScanFit::differentiateHist ( Chip* pCbc, std::string pHistName )
//{
//    TH1D* cHist = dynamic_cast<TH1D*> ( getHist ( pCbc, pHistName) );
//    TString cHistname(cHist->GetName());
//    cHistname.ReplaceAll("Clusters", "ClustersDiff");
//    TH1D* cDerivative = (TH1D*) cHist->Clone(cHistname);
//    cDerivative->Sumw2();
//    cDerivative->Reset();
//    bookHistogram ( pCbc, pHistName + "_Diff", cDerivative );

//    double_t cDiff;
//    double_t cCurrent;
//    double_t cPrev;
//    bool cActive; // indicates existence of data points
//    int cStep = 1;
//    int cDiffCounter = 0;

//    double cBin = 0;

//    cPrev = cHist->GetBinContent ( cHist->FindBin ( 0 ) );
//    cActive = false;

//    for ( cBin = cHist->FindBin (fVCthMax); cBin >= cHist->FindBin (fVCthMin); cBin-- )
//    //for ( cBin = cHist->FindBin (129); cBin >= cHist->FindBin (0); cBin-- ) // Hardcoded Max Vcth, careful...
//    {

//	      cCurrent = cHist->GetBinContent (cBin);
//	      cDiff = cPrev - cCurrent;
//
//	      if ( cPrev > 0.75 ) cActive = true; // sampling begins
//
//	      int iBinDerivative = cDerivative->FindBin ( (cHist->GetBinCenter (cBin - 1) + cHist->GetBinCenter (cBin) )
/// 2.0);
//
//	      if ( cActive ) cDerivative->SetBinContent ( iBinDerivative, cDiff  );
//
//	      if ( cActive && cDiff == 0 && cCurrent == 0 ) cDiffCounter++;
//
//	      if ( cDiffCounter == 8 ) break;
//
//	      cPrev = cCurrent;
//    }
//}

// void SignalScanFit::fitHist ( Chip* pCbc, std::string pHistName )
//{

//    std::cout << BOLDRED << "WARNING: The fitting precedure is WORK IN PROGRESS and it might not work out of the box
//    therefore the deault is set to disable the automatic fit!" << RESET << std::endl;

//    // This fitting procedure is very long-winded, there are better fititng procedures in the making!!!
//    // Fitting depends on variable / single mode for CBC2. We use a fitfunction here that can be used for both in most
//    cases.

//    TProfile* cHist = dynamic_cast<TProfile*> ( getHist ( pCbc, pHistName) );
//    double cStart  = 0;
//    double cStop = 110; // Fit fails if it reaches the noise, this is for low energies!

//    // Draw fit parameters
//    TStyle * cStyle = new TStyle();
//    cStyle->SetOptFit(1011);

//    std::string cFitname = "CurveFit";
//    TF1* cFit = dynamic_cast<TF1*> (gROOT->FindObject (cFitname.c_str() ) );
//    if (cFit) delete cFit;

//    double cMin = 0., cMax = 0., variable = 0., cPlateau = 0., cWidth = 0., cVsignal = 0., cNoise = 0.;

//    cPlateau = 0.01;
//    cWidth = 15.;
//    cVsignal = 490.;
//    cNoise = 6.;
//    cMin = 0;
//    cMax = 530;
//    cFit = new TF1 ("MyGammaSignal", MyGammaSignal, cMin, cMax, 4);
//    cFit->SetParLimits(0, 0., 50.);
//    cFit->SetParLimits(1, 450., 530.);
//    cFit->SetParLimits(2, 0., 20.);
//    cFit->SetParLimits(3, 0., 20.);
//
//    cFit->SetParameter ( 0, cPlateau );
//    cFit->SetParameter ( 1, cWidth );
//    cFit->SetParameter ( 2, cVsignal );
//    cFit->SetParameter ( 3, cNoise );

//    cFit->SetParName(0, "plateau");
//    cFit->SetParName(1, "width");
//    cFit->SetParName(2, "signal");
//    cFit->SetParName(3, "noise???");

//    cHist->Fit(cFit, "R+", "", cMin, cMax);
//    cFit->Draw("same");

//    // Would be nice to catch failed fits
//    cHist->Write (cHist->GetName(), TObject::kOverwrite);

//}

/*double cFirst = -999;             // First data point
double cCurrent = 0;              // Bin content
double cPrevious = 0;             // Content of previous bin
double cNext = 0;                 // Content of next bin
double cRatioFirst = -999;        // First Ratio
double cRatio = 0;                // Ratio between first data point and current data point
uint32_t cFirstOrderCounter = 0;  // Count for how long we are in first zone
uint32_t cSecondOrderCounter = 0; // Count for how long we are in second zone
uint32_t cThirdOrderCounter = 0;  // Count for how long we are in third zone
double cRunningAverage = 0;       // Running average for during the plateau

// Default values for the fit, these will be customized in the next for-loop.
double cPlateau = 0., cWidth = 0., cVsignal = 0., cNoise = 1;
if ( fType == ChipType::CBC2 ) cPlateau = 0.05, cWidth = 10, cVsignal = 74;
else if ( fType == ChipType::CBC3 ) cPlateau = 0.05, cWidth = 20, cVsignal = 120;

// Not Hole Mode
if ( !fHoleMode )
{

    for ( Int_t cBin = 2; cBin < cHist->GetNbinsX() - 1; cBin++ )
      {
        cCurrent = cHist->GetBinContent ( cBin );
        cPrevious = cHist->GetBinContent ( cBin - 1 );
        cNext = cHist->GetBinContent ( cBin + 1 );
        cRunningAverage = (cCurrent + cPrevious + cNext) / 3;

        if ( cRunningAverage != 0 && cFirst == -999 ) {
            cFirst = cRunningAverage;
            cStart = cBin;
            std::cout << "This is cFirst " << cFirst << std::endl;
        }

        if ((cFirst / cRunningAverage) == 1) continue;

        if ( cFirst > 0 ) {
            cRatio = cFirst / cRunningAverage; // This is for the three zones
        }

        if ( cRatioFirst == -999 && cRatio != 0 ) cRatioFirst = cRatio;

        // Now we have all the Ratios, let's play
        // We are in the beginning when the ratio is still bigger than 0.1, in the beginning there is a quick rise
        if ( cRatio > cRatioFirst * 0.1 )
        {
            cFirstOrderCounter++;
        }
        // At more or less half of the second order we can set the Vsignal
        // Second order (times 2?) can be the width
        else if ( (cRatio > cRatioFirst * 0.007) && (cRatio < cRatioFirst * 0.1) )
        {
            cSecondOrderCounter++;
        }
        // This is the plateau, the points here should be close in value (take average)
        //else if ( (cRatio < 0.007) && (cRatioCurrent > 0.8 && cRatioCurrent < 1.2) )
        else if ( (cRatio < cRatioFirst * 0.007) )
        {
            if ( cVsignal == 0 && cWidth == 0 )
            {
                //cWidth is roughly the cSecondOrder width, cVsignal is roughly half of that
                if ( cSecondOrderCounter < 10 )
                {
                    cWidth = cSecondOrderCounter*2;
                    cVsignal = cHist->GetBinCenter( cBin - (cSecondOrderCounter/2) );
                } else {
                    cWidth = cSecondOrderCounter;
                    cVsignal = cHist->GetBinCenter( cBin - (cSecondOrderCounter) );
                }
            }
            if ( cCurrent == 0 || cThirdOrderCounter == 10) break; // Either there is no more data (and of scan) or to
much (we don't want to hit the noise regime) cThirdOrderCounter++; cPlateau = cHist->GetBinContent( cBin -
(cThirdOrderCounter/2) ); cStop = cBin - (cThirdOrderCounter/2);
        }
      }
}


// Hole mode not implemented!
else
{
    LOG (INFO) << BOLDRED << "Hole mode is not implemented, the fitting procedure is terminated!" << RESET;
    return;
}

// Now we can make the fit function
double cXmin = cStart-5; // we take the first data_point-5 to
double cXmax = 0;
if ( cHist->GetBinCenter( cStop + 10 ) < 90 ) cXmax = cHist->GetBinCenter( cStop + 10 );
else cXmax = 90;

std::cout << "Fit parameters: width = " << cWidth << " , 'noise' = " << cNoise << " , plateau = " << cPlateau << " ,
VcthSignal = " << cVsignal << std::endl; std::cout << "Start point of fit: " << cXmin << ", end point of fit: " << cXmax
<< std::endl;

cFit = new TF1 ("CurveFit", MyGammaSignal, cXmin, cXmax, 4); // MyGammaSignal is in Utils

//cFit = new TF1 ( "CurveFit", MyGammaSignal, cStart, cStop, 4 ); // MyGammaSignal is in Utils

cFit->SetParameter ( 0, cPlateau );
cFit->SetParameter ( 1, cWidth );
cFit->SetParameter ( 2, cVsignal );
cFit->SetParameter ( 3, cNoise );

cFit->SetParName ( 0, "Plateau" );
cFit->SetParName ( 1, "Width" );
cFit->SetParName ( 2, "Vsignal" );
cFit->SetParName ( 3, "Noise" );

// Constraining the width to be positive
cFit->SetParLimits ( 1, 0, 100 );

// Fit
//cHist->Fit ( cFit, "RQB+" );
cHist->Fit ( cFit, "RB+" );

// Would be nice to catch failed fits
cHist->Write (cHist->GetName(), TObject::kOverwrite);
}*/

// State machine control functions

void SignalScanFit::ConfigureCalibration() {}

void SignalScanFit::Running() {}

void SignalScanFit::Stop() {}

void SignalScanFit::Pause() {}

void SignalScanFit::Resume() {}
#endif
