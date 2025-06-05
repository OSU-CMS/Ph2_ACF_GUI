#include "PulseShape.h"

#ifdef __USE_ROOT__
#include "HWInterface/CbcInterface.h"

PulseShape::PulseShape() : Tool() {}

PulseShape::~PulseShape() {}

void PulseShape::Initialize()
{
    LOG(INFO) << "starting the Initialize()";
    fNCbc = 0;

    std::cerr << "void PulseShape::Initialize()";

    for(auto cBoard: *fDetectorContainer)
    {
        LOG(INFO) << "Loop for board()";
        uint32_t cBoardId = cBoard->getId();
        std::cerr << "cBoardId = " << cBoardId;
        LOG(INFO) << "Before reading board parameter()";
        // fDelayAfterPulse = fBeBoardInterface->ReadBoardReg (cBoard, getDelAfterTPString ( cBoard->getBoardType() ) );
        fDelayAfterPulse = 196;
        LOG(INFO) << "After reading board parameter()";

        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                LOG(INFO) << "Certain board()";
                uint32_t cHybridId = cHybrid->getId();
                std::cerr << "cHybridId = " << cHybridId;
                fType = static_cast<OuterTrackerHybrid*>(cHybrid)->getFrontEndType();

                for(auto cCbc: *cHybrid)
                {
                    uint16_t cMaxValue = 1023;
                    uint32_t cCbcId    = cCbc->getId();
                    std::cerr << "cCbcId = " << cCbcId;
                    fNCbc++;
                    // Create the Canvas to draw
                    TCanvas* ctmpCanvas = new TCanvas(Form("c_online_canvas_fe%dcbc%d", cHybridId, cCbcId), Form("FE%dCBC%d  Online Canvas", cHybridId, cCbcId));
                    ctmpCanvas->Divide(2, 1);
                    fCanvasMap[cCbc] = ctmpCanvas;

                    // should set the canvas frames sane!
                    int cLow  = (fDelayAfterPulse + 3) * 25;
                    int cHigh = (fDelayAfterPulse + 8) * 25;
                    // TH2I* cFrame = new TH2I ( "cFrame", "PulseShape; Delay [ns]; Amplitude [VCth]", 350, cLow, cHigh,
                    // cMaxValue, 0, cMaxValue );
                    TH2I* cFrame = new TH2I("cFrame", "PulseShape; Delay [ns]; Amplitude [VCth]", 350, 0, cHigh - cLow, cMaxValue, 0,
                                            cMaxValue); // Jarne
                    cFrame->SetStats(false);
                    ctmpCanvas->cd(2);
                    cFrame->Draw();
                    bookHistogram(cCbc, "frame", cFrame);
                    // Create Multigraph Object for each CBC
                    TString  cName = Form("g_cbc_pulseshape_MultiGraph_Fe%dCbc%d", cHybridId, cCbcId);
                    TObject* cObj  = gROOT->FindObject(cName);

                    if(cObj) delete cObj;

                    TMultiGraph* cMultiGraph = new TMultiGraph();
                    cMultiGraph->SetName(cName);
                    bookHistogram(cCbc, "cbc_pulseshape", cMultiGraph);
                    cName = Form("f_cbc_pulse_Fe%dCbc%d", cHybridId, cCbcId);
                    cObj  = gROOT->FindObject(cName);

                    if(cObj) delete cObj;
                }
            }
        }
    }

    parseSettings();
    LOG(INFO) << "Histograms and Settings initialised.";
}

void PulseShape::ScanTestPulseDelay(uint8_t fStepSize)
{
    // setSystemTestPulse(fTPAmplitude, fChannelId);
    // enableChannel(fChannelId);
    LOG(INFO) << "setSystemTestPulse";
    setSystemTestPulse(fTPAmplitude /*, 0 */);
    LOG(INFO) << "toggleTestGroup (true)";
    toggleTestGroup(true);
    LOG(INFO) << " end of toggleTestGroup (true)";
    // initialize the historgram for the channel map
    // should set the histogram boardes frames sane (from config file)!
    uint32_t cCoarseDefault = fDelayAfterPulse;
    uint32_t cLow           = (cCoarseDefault + 3) * 25;
    uint32_t cHigh          = (cCoarseDefault + 8) * 25;

    LOG(INFO) << "Starting loop over delays";
    for(uint32_t cTestPulseDelay = cLow; cTestPulseDelay < cHigh; cTestPulseDelay += fStepSize)
    {
        setDelayAndTesGroup(cTestPulseDelay);
        ScanVcth(cTestPulseDelay, cLow);
    }

    this->fitGraph(cLow);
    updateHists("cbc_pulseshape", true);
    toggleTestGroup(false);
}

void PulseShape::ScanVcth(uint32_t pDelay, int cLow)
{
    for(auto& cChannelVector: fChannelMap)
        for(auto& cChannel: cChannelVector.second) cChannel->initializeHist(pDelay, "Delay");

    uint16_t cMaxValue      = 0x003FF;
    uint16_t cVcth          = (fHoleMode) ? cMaxValue : 0x00;
    int      cStep          = (fHoleMode) ? -10 : +10;
    uint32_t cAllOneCounter = 0;
    bool     cAllOne        = false;
    bool     cNonZero       = false;
    bool     cSaturate      = false;
    uint16_t cDoubleVcth    = 0;

    ThresholdVisitor cVisitor(fReadoutChipInterface, 0);

    // Adaptive VCth loop
    while(0x00 == cVcth && cVcth <= cMaxValue)
    {
        // LOG (INFO) << "   "<< cVcth ;
        if(cAllOne) break;

        if(cVcth == cDoubleVcth)
        {
            cVcth += cStep;
            continue;
        }

        // then we take fNEvents
        uint32_t cNthAcq = 0;
        int      cNHits  = 0;

        // Take Data for all Hybrids
        for(auto pBoard: *fDetectorContainer)
        {
            BeBoard* theBoard = static_cast<BeBoard*>(pBoard);
            for(auto cOpticalGroup: *pBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    cVisitor.setThreshold(cVcth);
                    static_cast<OuterTrackerHybrid*>(cHybrid)->accept(cVisitor);
                }
            }

            // LOG(INFO) << "Reading N Events";
            ReadNEvents(theBoard, fNevents);
            // LOG(INFO) << "End Reading N Events";
            const std::vector<Event*>& events = GetEvents();
            if(events.empty()) LOG(INFO) << " EMPTY EVENT VECTOR !!!";
            // LOG (INFO) <<"events size, VCTH value " << events.size()<< "  "<< (uint16_t) cVcth;
            // int iii=0;
            for(auto& cEvent: events)
            {
                // LOG (INFO) <<"EVENT ID "<< iii;
                // iii++;
                cNHits += fillVcthHist(theBoard, cEvent, cVcth);
            }
            cNthAcq++;

            if(!cNonZero && cNHits != 0)
            {
                cNonZero      = true;
                cDoubleVcth   = cVcth;
                int cBackStep = 2 * cStep;

                if(int(cVcth) - cBackStep > cMaxValue)
                    cVcth = cMaxValue;
                else if(int(cVcth) - cBackStep < 0)
                    cVcth = 0;
                else
                    cVcth -= cBackStep;

                cStep /= 10;
                continue;
            }

            if(cNHits > 0.95 * fNCbc * fNevents * findChannelsInTestGroup(fTestGroup).size()) cAllOneCounter++;
            // if ( cAllOneCounter > 6 ) cAllOne = true;
            if(cAllOneCounter > 30) cAllOne = true; // increase fine scan steps

            if(cAllOne) break; // by BB, suspect this is the bug for zero entries

            cVcth += cStep;
            updateHists("", false);

            if(fHoleMode && cVcth >= cMaxValue - 1 && cNHits != 0)
            {
                cSaturate = true;
                break;
            }

            if(!fHoleMode && cVcth <= 0x01 && cNHits != 0)
            {
                cSaturate = true;
                break;
            }
        }
    }

    for(auto& cChannelVector: fChannelMap)
    {
        for(auto& cChannel: cChannelVector.second)
        {
            if(fFitHist)
                cChannel->fitHist(fNevents, fHoleMode, pDelay, "Delay", fResultFile);
            else
                cChannel->differentiateHist(fNevents, fHoleMode, pDelay, "Delay", fResultFile);

            // if ( !cSaturate ) cChannel->setPulsePoint ( pDelay, cChannel->getPedestal() );
            // else cChannel->setPulsePoint ( pDelay, 255 );

            if(!cSaturate)
            {
                cChannel->setPulsePoint(pDelay - cLow, cChannel->getPedestal()); // Jarne
                if(cChannel->getPedestal() == 0)
                    LOG(INFO) << " I AM HERE WITH ZERO!!  "
                              << "  " << (double)cChannel->fCbcId << "  " << (double)cChannel->fChannelId << "  " << cChannel->getPedestal();
            }
            else
                cChannel->setPulsePoint(pDelay - cLow, 255); // Jarne
            // CbcRegReader cReader( fCbcInterface, "SelTestPulseDel&ChanGroup" );
            // LOG (INFO) <<std::hex << +cReader.visit(cCbc).fRegValue;
        }
    }
    LOG(INFO) << "   " << cVcth;

    updateHists("", true);
    updateHists("cbc_pulseshape", false);
}

//////////////////////////////////////      PRIVATE METHODS     /////////////////////////////////////////////

void PulseShape::fitGraph(int pLow)
{
    for(auto& cCbc: fChannelMap)
    {
        for(auto& cChannel: cCbc.second)
        {
            TString  cName = Form("f_cbc_pulse_Fe%dCbc%d_Channel%d", static_cast<ReadoutChip*>(cCbc.first)->getHybridId(), static_cast<ReadoutChip*>(cCbc.first)->getId(), cChannel->fChannelId);
            TObject* cObj  = gROOT->FindObject(cName);

            if(cObj) delete cObj;

            bool UsePulseShape2 = true;
            // TF1* cPulseFit = new TF1( cName, pulseshape, ( fDelayAfterPulse - 1 ) * 25, ( fDelayAfterPulse + 6 ) *
            // 25, 6 );
            TF1* cPulseFit = new TF1(cName, pulseshape, 5000, 5300, 6);

            cPulseFit->SetParNames("Amplitude", "t_0", "tau", "Amplitude offset", "Rel. negative pulse amplitude", "Delta t");
            //"scale_par"
            cPulseFit->SetParLimits(0, 160, 2000);
            cPulseFit->SetParameter(0, 250);
            //"offset"
            cPulseFit->SetParLimits(1, 5000, 5300);
            cPulseFit->SetParameter(1, 5025);
            //"time_constant"
            cPulseFit->SetParLimits(2, 25, 75);
            cPulseFit->SetParameter(2, 50);
            //"y_offset"
            cPulseFit->SetParLimits(3, 0, 200);
            cPulseFit->SetParameter(3, 50);
            //"Relative amplitude of negative pulse"
            cPulseFit->SetParameter(4, 0.5);
            cPulseFit->SetParLimits(4, 0, 1);
            // delta t
            cPulseFit->FixParameter(5, 50);

            if(UsePulseShape2 == true)
            {
                cPulseFit = new TF1(cName, pulseshape2, 0, 140, 5);
                cPulseFit->SetParNames("theta", "r", "tau", "amplitude_offset", "amplitude_stretch");
                //"theta"
                cPulseFit->SetParLimits(0, -1, 1);
                cPulseFit->SetParameter(0, 0.5);
                //"r"
                cPulseFit->SetParLimits(1, -5, -30);
                cPulseFit->SetParameter(1, -12);
                //"tau"
                cPulseFit->SetParLimits(2, 5, 30);
                cPulseFit->SetParameter(2, 12);
                //"amplitude offset"
                cPulseFit->SetParLimits(3, 0, 255);
                cPulseFit->SetParameter(3, 135);
                //"amplitude stretch"
                cPulseFit->SetParLimits(4, 0, 255);
                cPulseFit->SetParameter(4, 35);
            }

            cChannel->fPulse->Fit(cPulseFit, "R+S");
            TString     cDirName = "PulseshapeFits";
            TDirectory* cDir     = dynamic_cast<TDirectory*>(gROOT->FindObject(cDirName));

            if(!cDir) cDir = fResultFile->mkdir(cDirName);

            fResultFile->cd(cDirName);
            cChannel->fPulse->Write(cChannel->fPulse->GetName(), TObject::kOverwrite);
            cPulseFit->Write(cPulseFit->GetName(), TObject::kOverwrite);
            fResultFile->cd();
            fResultFile->Flush();
        }
    }
}

std::vector<uint32_t> PulseShape::findChannelsInTestGroup(uint32_t pTestGroup)
{
    std::vector<uint32_t> cChannelVector;
    // Jarne: changing this to all channels, so the script thinks it has to make a pulse shape for everything
    for(int idx = 0; idx < 254; idx++)
    {
        int ctemp1 = idx + 1;
        int ctemp2 = ctemp1 + 1;

        if(ctemp1 < 254) cChannelVector.push_back(ctemp1);

        if(ctemp2 < 254) cChannelVector.push_back(ctemp2);
        idx++;
    }

    /* for ( int idx = 0; idx < 16; idx++ )
     {
         int ctemp1 = idx * 16  + pTestGroup * 2 + 1 ;
         int ctemp2 = ctemp1 + 1;

         if ( ctemp1 < 254 ) cChannelVector.push_back ( ctemp1 );

         if ( ctemp2 < 254 )  cChannelVector.push_back ( ctemp2 );
     }*/
    return cChannelVector;
}

void PulseShape::toggleTestGroup(bool pEnable)
{
    std::vector<std::pair<std::string, uint8_t>> cRegVec;
    uint8_t                                      cDisableValue = fHoleMode ? 0x00 : 0xFF;
    uint8_t                                      cValue        = pEnable ? fOffset : cDisableValue;

    for(auto& cChannel: fChannelVector)
    {
        TString cRegName = Form("Channel%03d", cChannel);
        cRegVec.push_back(std::make_pair(cRegName.Data(), cValue));
    }

    // CbcMultiRegWriter cWriter ( fReadoutChipInterface, cRegVec );
    // this->accept ( cWriter );

    /*LOG(INFO) << "Going to do a broadcast()";
    for (BeBoard* cBoard : fBoardVector)
    {
        for (Module* cHybrid : cBoard->fModuleVector)
            dynamic_cast<CbcInterface*>(fReadoutChipInterface)->WriteBroadcastCbcMultiReg (cHybrid, cRegVec);
    }
    */
}

void PulseShape::setDelayAndTesGroup(uint32_t pDelay)
{
    uint8_t cCoarseDelay = floor(pDelay / 25);
    uint8_t cFineDelay   = (cCoarseDelay * 25) + 24 - pDelay;

    LOG(INFO) << "cFineDelay: " << +cFineDelay;
    LOG(INFO) << "cCoarseDelay: " << +cCoarseDelay;
    LOG(INFO) << "Current Time: " << +pDelay;

    for(auto pBoard: *fDetectorContainer)
    {
        // potentially have to reset the IC FW commissioning cycle state machine?

        LOG(INFO) << "Writing the delay values to the board";
        // for (auto cReg : getDelAfterTPString (pBoard->getBoardType() ) )
        //        fBeBoardInterface->WriteBoardReg (pBoard, cReg, cCoarseDelay);
        fBeBoardInterface->WriteBoardReg(static_cast<BeBoard*>(pBoard), "fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse", cCoarseDelay);
    }

    LOG(INFO) << "Writing fine delay and test group, i2c...";
    CbcRegWriter cWriter(fReadoutChipInterface, "TestPulseDel&ChanGroup", to_reg(cFineDelay, fTestGroup));
    this->accept(cWriter);
    LOG(INFO) << "End of Writing fine delay and test group, i2c...";
}

uint32_t PulseShape::fillVcthHist(BeBoard* pBoard, Event* pEvent, uint32_t pVcth)
{
    uint32_t cHits = 0;

    // Loop over Events from this Acquisition
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cCbc: *cHybrid)
            {
                //  get histogram to fill
                auto cChannelVector = fChannelMap.find(cCbc);

                if(cChannelVector == std::end(fChannelMap))
                    LOG(INFO) << "Error, no channel vector mapped to this CBC ( " << +cCbc->getId() << " )";
                else
                {
                    for(auto& cChannel: cChannelVector->second)
                    {
                        if(pEvent->DataBit(cHybrid->getId(), cCbc->getId(), 0, cChannel->fChannelId - 1))
                        {
                            cChannel->fillHist(pVcth);
                            cHits++;
                        }
                    }
                }
            }
        }
    }
    return cHits;
}

void PulseShape::parseSettings()
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
        fHoleMode = 1;

    cSetting = fSettingsMap.find("Vplus");

    if(cSetting != std::end(fSettingsMap))
        fVplus = boost::any_cast<double>(cSetting->second);
    else
        fVplus = 0x6F;

    cSetting = fSettingsMap.find("TestPulsePotentiometer");

    if(cSetting != std::end(fSettingsMap))
        fTPAmplitude = boost::any_cast<double>(cSetting->second);
    else
        fTPAmplitude = 0x20;

    cSetting = fSettingsMap.find("ChannelOffset");

    //  if ( cSetting != std::end ( fSettingsMap ) ) fOffset = boost::any_cast<double>(cSetting->second);
    //  else fOffset = 0x5;

    cSetting = fSettingsMap.find("TestGroup");

    if(cSetting != std::end(fSettingsMap))
        fTestGroup = boost::any_cast<double>(cSetting->second);
    else
        fTestGroup = 0;

    cSetting = fSettingsMap.find("StepSize");

    if(cSetting != std::end(fSettingsMap))
        fStepSize = boost::any_cast<double>(cSetting->second);
    else
        fStepSize = 5;

    cSetting = fSettingsMap.find("FitSCurves");
    fFitHist = (cSetting != std::end(fSettingsMap)) ? boost::any_cast<double>(cSetting->second) : 0;

    LOG(INFO) << "Parsed the following settings:";
    LOG(INFO) << "	Nevents = " << fNevents;
    LOG(INFO) << "	HoleMode = " << int(fHoleMode);
    LOG(INFO) << "	Vplus = " << int(fVplus);
    LOG(INFO) << "	TPAmplitude = " << int(fTPAmplitude);
    LOG(INFO) << "	ChOffset = " << int(fOffset);
    LOG(INFO) << "	StepSize = " << int(fStepSize);
    LOG(INFO) << "	FitSCurves = " << int(fFitHist);
    LOG(INFO) << "	TestGroup = " << int(fTestGroup);
}

void PulseShape::setSystemTestPulse(uint8_t pTPAmplitude)
{
    std::vector<std::pair<std::string, uint16_t>> cRegVec;
    fChannelVector = findChannelsInTestGroup(fTestGroup);
    // set the value of test pulsepot registrer and MiscTestPulseCtrl&AnalogMux register
    if(fHoleMode)
        cRegVec.push_back(std::make_pair("MiscTestPulseCtrl&AnalogMux", 0xC1));
    else
        cRegVec.push_back(std::make_pair("MiscTestPulseCtrl&AnalogMux", 0x61));

    // cRegVec.push_back ( std::make_pair ( "TestPulsePot", pTPAmplitude ) );
    cRegVec.push_back(std::make_pair("TestPulsePot", 0x40));
    cRegVec.push_back(std::make_pair("Vplus", fVplus));
    cRegVec.push_back(std::make_pair("HitDetectSLVS", 0xE7));

    cRegVec.push_back(std::make_pair("FrontEndControl", 0x3C));
    cRegVec.push_back(std::make_pair("Ipre1", 0xF0));
    cRegVec.push_back(std::make_pair("Ipre2", 0x14));
    cRegVec.push_back(std::make_pair("Ipsf", 0x2D));
    cRegVec.push_back(std::make_pair("Ipa", 0x1E));
    cRegVec.push_back(std::make_pair("Ipaos", 0x2d));
    cRegVec.push_back(std::make_pair("Vpafb", 0x00));
    cRegVec.push_back(std::make_pair("Icomp", 0x1E));
    cRegVec.push_back(std::make_pair("Vpc", 0x4B));
    cRegVec.push_back(std::make_pair("TestPulseChargeMirrCascodeVolt", 0x41));

    /*cRegVec.push_back ( std::make_pair ( "MaskChannelFrom008downto001",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom016downto009",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom024downto017",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom032downto025",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom040downto033",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom048downto041",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom056downto049",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom064downto057",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom072downto065",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom080downto073",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom088downto081",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom096downto089",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom104downto097",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom112downto105",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom120downto113",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom128downto121",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom136downto129",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom144downto137",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom152downto145",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom160downto153",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom168downto161",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom176downto169",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom184downto177",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom192downto185",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom200downto193",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom208downto201",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom216downto209",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom224downto217",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom232downto225",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom240downto233",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom248downto241",  0xff ) );
    cRegVec.push_back ( std::make_pair ( "MaskChannelFrom254downto249",  0xff ) );
*/

    cRegVec.push_back(std::make_pair("TestPulsePotNodeSel", pTPAmplitude));
    cRegVec.push_back(std::make_pair("Vplus1&2", fVplus));
    cRegVec.push_back(std::make_pair("Pipe&StubInpSel&Ptwidth",
                                     0x03)); // select the hit detect pipeline logic, def is sampled (variable) mode
                                             // (0x03), 0xc3 for fixed with mode, 0x43 for OR mode, 0x83 for HIP supp

    CbcMultiRegWriter cWriter(fReadoutChipInterface, cRegVec);

    this->accept(cWriter);

    for(auto cBoard: *fDetectorContainer)
    {
        uint32_t cBoardId = cBoard->getId();
        for(auto cOpticalGroup: *cBoard)
        {
            uint32_t cOpticalGroupId = cOpticalGroup->getId();
            for(auto cHybrid: *cOpticalGroup)
            {
                uint32_t cHybridId = cHybrid->getId();

                for(auto cCbc: *cHybrid)
                {
                    std::vector<Channel*> cChannelVector;
                    uint32_t              cCbcId      = cCbc->getId();
                    int                   cMakerColor = 1;

                    for(auto& cChannelId: fChannelVector)
                    {
                        Channel* cChannel = new Channel(cBoardId, cOpticalGroupId, cHybridId, cCbcId, cChannelId);
                        TString  cName    = Form("g_cbc_pulseshape_Fe%dCbc%d_Channel%d", cHybridId, cCbcId, cChannelId);
                        cChannel->initializePulse(cName);
                        cChannelVector.push_back(cChannel);
                        cChannel->fPulse->SetMarkerColor(cMakerColor);
                        cMakerColor++;
                        TMultiGraph* cTmpGraph = static_cast<TMultiGraph*>(getHist(cCbc, "cbc_pulseshape"));
                        cTmpGraph->Add(cChannel->fPulse, "lp");
                    }

                    fChannelMap[cCbc] = cChannelVector;
                }
            }
        }
    }
}

void PulseShape::updateHists(std::string pHistName, bool pFinal)
{
    for(auto& cCanvas: fCanvasMap)
    {
        cCanvas.second->cd();

        if(pHistName == "")
        {
            // now iterate over the channels in the channel map and draw
            auto cChannelVector = fChannelMap.find(static_cast<ChipContainer*>(cCanvas.first));

            if(cChannelVector == std::end(fChannelMap))
                LOG(INFO) << "Error, no channel mapped to this CBC ( " << +cCanvas.first << " )";
            else
            {
                TString cOption = "P";

                for(auto& cChannel: cChannelVector->second)
                {
                    cCanvas.second->cd(1);
                    cChannel->fScurve->Draw(cOption);
                    cOption = "p same";
                }
            }
        }

        if(pHistName == "" && pFinal)
        {
            // now iterate over the channels in the channel map and draw
            auto cChannelVector = fChannelMap.find(static_cast<ChipContainer*>(cCanvas.first));

            if(cChannelVector == std::end(fChannelMap))
                LOG(INFO) << "Error, no channel mapped to this CBC ( " << +cCanvas.first << " )";
            else
            {
                TString cOption = "P";

                for(auto& cChannel: cChannelVector->second)
                {
                    cCanvas.second->cd(1);
                    cChannel->fScurve->Draw(cOption);
                    cOption = "p same";

                    if(fFitHist)
                        cChannel->fFit->Draw("same");
                    else
                        cChannel->fDerivative->Draw("same");
                }
            }
        }
        else if(pHistName == "cbc_pulseshape")
        {
            auto cChannelVector = fChannelMap.find(static_cast<ChipContainer*>(cCanvas.first));

            if(cChannelVector == std::end(fChannelMap))
                LOG(INFO) << "Error, no channel mapped to this CBC ( " << +cCanvas.first << " )";
            else
            {
                TH2I* cTmpFrame = static_cast<TH2I*>(getHist(static_cast<ChipContainer*>(cCanvas.first), "frame"));
                cCanvas.second->cd(2);
                cTmpFrame->Draw();
                TString cOption = "P same";

                for(auto& cChannel: cChannelVector->second)
                {
                    cCanvas.second->cd(2);
                    cChannel->fPulse->Draw(cOption);
                    cOption = "P same";
                }
            }
        }
        else if(pHistName == "cbc_pulseshape" && pFinal)
        {
            auto cChannelVector = fChannelMap.find(static_cast<ChipContainer*>(cCanvas.first));

            if(cChannelVector == std::end(fChannelMap))
                LOG(INFO) << "Error, no channel mapped to this CBC ( " << +cCanvas.first << " )";
            else
            {
                cCanvas.second->cd(2);
                TMultiGraph* cMultiGraph = static_cast<TMultiGraph*>(getHist(static_cast<ChipContainer*>(cCanvas.first), "cbc_pulseshape"));
                cMultiGraph->Draw("A");
                cCanvas.second->Modified();
            }
        }

        cCanvas.second->Update();
    }
}

double pulseshape(double* x, double* par)
{
    double xx  = x[0];
    double val = par[3];

    if(xx > par[1]) val += par[0] * (xx - par[1]) / par[2] * exp(-((xx - par[1]) / par[2]));

    if(xx > par[1] + par[5]) val -= par[0] * par[4] * (xx - par[1] - par[5]) / par[2] * exp(-((xx - par[1] - par[5]) / par[2]));

    return val;
}

double pulseshape2(double* x, double* par)
{
    double xx = x[0]; // time

    double theta             = par[0];
    double r                 = par[1];
    double tau               = par[2];
    double Amplitude_offset  = par[3];
    double Amplitude_stretch = par[4];

    double f_0 = 0.5;
    double f_1 = r / (6 * tau * tau) * (TMath::Cos(theta) + TMath::Sin(theta)) * (xx - 3 * tau);
    double f_2 = r * r / (24 * pow(tau, 4)) * (1 + TMath::Cos(theta) * TMath::Sin(theta)) * (xx * xx - 8 * tau * xx + 12 * tau * tau);

    return Amplitude_offset + Amplitude_stretch * TMath::Exp(-xx / tau) * pow(xx / tau, 2) * (f_0 + f_1 + f_2);
}

#endif
