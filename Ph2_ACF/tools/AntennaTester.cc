#include "AntennaTester.h"

#ifdef __USE_ROOT__

// fill the Histograms, count the hits and increment Vcth
struct HistogramFiller : public HwDescriptionVisitor
{
    TH1F*  fBotHist;
    TH1F*  fTopHist;
    Event* fEvent;

    HistogramFiller(TH1F* pBotHist, TH1F* pTopHist, Event* pEvent) : fBotHist(pBotHist), fTopHist(pTopHist), fEvent(pEvent) {}

    void visit(Chip& pCbc)
    {
        std::vector<bool> cDataBitVector = fEvent->DataBitVector(pCbc.getHybridId(), pCbc.getId());

        for(uint32_t cId = 0; cId < NCHANNELS; cId++)
        {
            if(cDataBitVector.at(cId))
            {
                uint32_t globalChannel = (pCbc.getId() * 254) + cId;

                // find out why histograms are not filling!
                if(globalChannel % 2 == 0)
                    fBotHist->Fill(globalChannel / 2);
                else
                    fTopHist->Fill((globalChannel - 1) / 2);
            }
        }
    }
};

AntennaTester::AntennaTester() : Tool() {}

// D'tor
AntennaTester::~AntennaTester() {}

void AntennaTester::Initialize()
{
    gStyle->SetOptStat(000000);
    fDataCanvas = new TCanvas("fAntennaDataCanvas", "SingleStripEfficiency (w/ Antenna)", 1200, 800);
    fDataCanvas->Divide(2);

    InitialiseSettings();
    InitializeHists();
}

void AntennaTester::EnableAntenna(bool pAntennaEnable, uint8_t pDigiPotentiometer)
{
#ifdef __ANTENNA__
    LOG(INFO) << BOLDBLUE << "Configuring antenna..." << RESET;
    Antenna cAntenna;
    uint8_t cADCChipSlave = 4;
    cAntenna.initializeAntenna();         // initialize USB communication
    cAntenna.ConfigureADC(cADCChipSlave); // initialize SPI communication for ADC
    cAntenna.SelectTriggerSource(trigSource);
    if(trigSource == 5)
    {
        cAntenna.ConfigureClockGenerator(3, 8); // initialize SPI communication for ADC
    }
    else if(trigSource == 7) {}
    else { LOG(INFO) << "ERROR, wrong trig source set " << int(trigSource); }
    cAntenna.ConfigureDigitalPotentiometer(2, pDigiPotentiometer); // configure bias for antenna pull-up
    // configure analogue switch
    uint8_t analog_switch_cs = 0;
    cAntenna.ConfigureAnalogueSwitch(analog_switch_cs); // configure communication with analogue switch
    LOG(INFO) << "Chip Select ID " << +analog_switch_cs;

    if(!pAntennaEnable)
    {
        LOG(INFO) << BOLDBLUE << "Disabling all channels on the antenna." << RESET;
        cAntenna.TurnOnAnalogSwitchChannel(9); // configure communication with analogue switch
    }
    else
        for(int i = 1; i < 5; i++) cAntenna.TurnOnAnalogSwitchChannel(i);
#endif
    // sleep ( 0.1 );
}

void AntennaTester::InitialiseSettings()
{
    fDecisionThreshold = 10.0;
    // figure out whether the hybrid was configured to run in hole/electron mode
    auto cSetting = fSettingsMap.find("HoleMode");
    fHoleMode     = (cSetting != std::end(fSettingsMap)) ? boost::any_cast<double>(cSetting->second) : true;

    // figure out how many CBCs you're working with
    Counter cCbcCounter;
    accept(cCbcCounter);
    fNCbc = cCbcCounter.getNChip();

    // figure out what the number of events to take is
    cSetting     = fSettingsMap.find("Nevents");
    fTotalEvents = (cSetting != std::end(fSettingsMap)) ? boost::any_cast<double>(cSetting->second) : 999;

    cSetting = fSettingsMap.find("TriggerSource");

    for(auto cBoard: *fDetectorContainer)
    {
        BeBoard* theBoard = static_cast<BeBoard*>(cBoard);
        trigSource        = fBeBoardInterface->ReadBoardReg(theBoard, "fc7_daq_cnfg.fast_command_block.trigger_source");
        LOG(INFO) << int(trigSource);
    }
}
void AntennaTester::InitializeHists()
{
    TString cFrontName("fAntennaHistTop");
    fHistTop = (TH1F*)(gROOT->FindObject(cFrontName));

    if(fHistTop) delete fHistTop;

    fHistTop = new TH1F(cFrontName, "Front Pad Channels; Pad Number; Occupancy measured w/ Antenna [%]", (fNCbc / 2 * 254), -0.5, (fNCbc / 2 * 254) - 0.5);
    fHistTop->SetFillColor(4);
    fHistTop->SetFillStyle(3001);

    TString cBackName("fAntennaHistBottom");
    fHistBottom = (TH1F*)(gROOT->FindObject(cBackName));

    if(fHistBottom) delete fHistBottom;

    fHistBottom = new TH1F(cBackName, "Back Pad Channels; Pad Number; Occupancy measured w/ Antenna [%]", (fNCbc / 2 * 254), -0.5, (fNCbc / 2 * 254) - 0.5);
    fHistBottom->SetFillColor(4);
    fHistBottom->SetFillStyle(3001);

    TString cFrontNameMerged("fAntennaHistTopMerged");
    fHistTopMerged = (TH1F*)(gROOT->FindObject(cFrontNameMerged));

    if(fHistTopMerged) delete fHistTopMerged;

    fHistTopMerged = new TH1F(cFrontNameMerged, "Front Pad Channels; Pad Number; Occupancy measured w/ Antenna [%]", (fNCbc / 2 * 254), -0.5, (fNCbc / 2 * 254) - 0.5);
    fHistTopMerged->SetFillColor(4);
    fHistTopMerged->SetFillStyle(3001);

    TString cBackNameMerged("fAntennaHistBottomMerged");
    fHistBottomMerged = (TH1F*)(gROOT->FindObject(cBackNameMerged));

    if(fHistBottomMerged) delete fHistBottomMerged;

    fHistBottomMerged = new TH1F(cBackNameMerged, "Back Pad Channels; Pad Number; Occupancy measured w/ Antenna [%]", (fNCbc / 2 * 254), -0.5, (fNCbc / 2 * 254) - 0.5);
    fHistBottomMerged->SetFillColor(4);
    fHistBottomMerged->SetFillStyle(3001);
    UpdateHists();
}

void AntennaTester::UpdateHists()
{
    fDataCanvas->cd(1);
    fHistTop->Draw();
    fDataCanvas->cd(2);
    fHistBottom->Draw();
    fDataCanvas->Update();
    this->HttpServerProcess();
}
void AntennaTester::UpdateHistsMerged()
{
    fDataCanvas->cd(1);
    fHistTopMerged->Draw();
    fDataCanvas->cd(2);
    fHistBottomMerged->Draw();
    fDataCanvas->Update();
    this->HttpServerProcess();
}
void AntennaTester::ReconfigureCBCRegisters(std::string pDirectoryName)
{
    for(auto cBoard: *fDetectorContainer)
    {
        BeBoard* theBoard = static_cast<BeBoard*>(cBoard);
        fBeBoardInterface->ChipReset(theBoard);

        trigSource = fBeBoardInterface->ReadBoardReg(theBoard, "fc7_daq_cnfg.fast_command_block.trigger_source");
        LOG(INFO) << int(trigSource);

        trigSource = fBeBoardInterface->ReadBoardReg(theBoard, "fc7_daq_cnfg.fast_command_block.trigger_source");
        LOG(INFO) << int(trigSource);

        for(auto cOpticalGroup: *cBoard)

            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cCbc: *cHybrid)
                {
                    ReadoutChip* theCbc = static_cast<ReadoutChip*>(cCbc);
                    std::string  pRegFile;
                    char         buffer[120];

                    if(pDirectoryName.empty())
                        sprintf(buffer, "%s/FE%dCBC%d.txt", fDirectoryName.c_str(), cHybrid->getId(), cCbc->getId());
                    else
                        sprintf(buffer, "%s/FE%dCBC%d.txt", pDirectoryName.c_str(), cHybrid->getId(), cCbc->getId());

                    pRegFile = buffer;
                    theCbc->loadfRegMap(pRegFile);
                    fReadoutChipInterface->ConfigureChip(theCbc);
                    LOG(INFO) << GREEN << "\t\t Successfully reconfigured CBC" << int(cCbc->getId()) << "'s regsiters from " << pRegFile << " ." << RESET;
                }
            }

        fBeBoardInterface->ChipReSync(theBoard);
    }
}

void AntennaTester::writeObjects()
{
    // writeGraphs();
    fResultFile->cd();
    UpdateHistsMerged();

    fHistTop->Write(fHistTop->GetName(), TObject::kOverwrite);
    fHistBottom->Write(fHistBottom->GetName(), TObject::kOverwrite);
    fDataCanvas->Write(fDataCanvas->GetName(), TObject::kOverwrite);
    LOG(INFO) << BOLDBLUE << "Results of Antenna scan written to " << fDirectoryName + "/Summary.root" << RESET;

    this->SaveResults();
    fResultFile->Flush();
}

void AntennaTester::Measure(uint8_t pDigiPotentiometer)
{
    LOG(INFO) << "Measuring Efficiency per Strip with the Antenna ... ";
    LOG(INFO) << "Taking data with " << fTotalEvents << " Events!";

    // in read mode like this!
    ThresholdVisitor cReader(fReadoutChipInterface);
    accept(cReader);

    InitializeHists();
#ifdef __ANTENNA__

    EnableAntenna(0, pDigiPotentiometer);
    Antenna cAntenna;
    sleep(0.1);

    fHistTop->GetYaxis()->SetRangeUser(0, fTotalEvents);
    fHistBottom->GetYaxis()->SetRangeUser(0, fTotalEvents);

    uint8_t analog_switch_cs = 0;
    cAntenna.ConfigureAnalogueSwitch(analog_switch_cs); // configure communication with analogue switch
    LOG(INFO) << "Chip Select ID " << +analog_switch_cs;

    for(uint8_t channel_position = 1; channel_position < 5; channel_position++)
    {
        cAntenna.TurnOnAnalogSwitchChannel(channel_position);
        if(channel_position == 9) break;

        for(auto pBoard: *fDetectorContainer)
        {
            uint32_t cN      = 1;
            uint32_t cNthAcq = 0;

            this->StartBoard(pBoard);

            while(cN <= fTotalEvents)
            {
                // Run( pBoard, cNthAcq );
                ReadData(pBoard);
                const std::vector<Event*>& events = GetEvents();

                // Loop over Events from this Acquisition
                for(auto& cEvent: events)
                {
                    HistogramFiller cFiller(fHistBottom, fHistTop, cEvent);
                    pBoard->accept(cFiller);

                    if(cN % 100 == 0) UpdateHists();

                    cN++;
                }

                cNthAcq++;
            }

            this->StopBoard(pBoard);

            /*Here the reconstruction of histograms happens*/
            for(uint16_t channel_id = 1; channel_id < fNCbc * 127 + 1; channel_id++)
            {
                if(fHistTopMerged->GetBinContent(channel_id) < fHistTop->GetBinContent(channel_id)) fHistTopMerged->SetBinContent(channel_id, fHistTop->GetBinContent(channel_id));

                if(fHistBottomMerged->GetBinContent(channel_id) < fHistBottom->GetBinContent(channel_id)) fHistBottomMerged->SetBinContent(channel_id, fHistBottom->GetBinContent(channel_id));
            }

            /*Here clearing histograms after each event*/
            fHistBottom->Reset();
            fHistTop->Reset();
        }
    }
    // EnableAntenna( 0, pDigiPotentiometer);

    // cAntenna.close();
#endif
    fHistTopMerged->Scale(100 / double_t(fTotalEvents));
    fHistTopMerged->GetYaxis()->SetRangeUser(0, 100);
    fHistBottomMerged->Scale(100 / double_t(fTotalEvents));
    fHistBottomMerged->GetYaxis()->SetRangeUser(0, 100);

    UpdateHistsMerged();
    LOG(INFO) << BOLDGREEN << "Mean occupancy (measured w/ antenna) for the Top side: " << GetMeanOccupancyTop() << "%." << RESET;
    LOG(INFO) << BOLDGREEN << "Mean occupancy (measured w/ antenna) for the Botton side: " << GetMeanOccupancyBottom() << "%." << RESET;
}
#endif
