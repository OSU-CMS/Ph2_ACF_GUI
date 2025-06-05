#include "OTQuickNoise.h"
#ifdef __USE_ROOT__
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

// PUBLIC METHODS
OTQuickNoise::OTQuickNoise() : Tool() {}

OTQuickNoise::~OTQuickNoise() {}

void OTQuickNoise::Initialize()
{
    parseSettings();
    LOG(INFO) << "Settings initialised.";
}

void OTQuickNoise::SetThresholds()
{
    // Set Vcth to pedestal, or overload with manual setting
    ThresholdVisitor cVisitor(fReadoutChipInterface, 0);

    LOG(INFO) << "OT_MODULE_TEST:: Setting threshold on each chip" << RESET;
    for(auto pBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *pBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                LOG(INFO) << BOLDGREEN << "Setting Manual Vcth to " << fManualVcth << RESET;
                if(fManualVcth != 0)
                {
                    cVisitor.setThreshold(fManualVcth);
                    static_cast<OuterTrackerHybrid*>(cHybrid)->accept(cVisitor);
                }
                else { LOG(INFO) << BOLDCYAN << "Not resetting threshold! Running with values in config files." << RESET; }
            }
        }
    }
}

void OTQuickNoise::TakeData()
{
    ThresholdVisitor cVisitor(fReadoutChipInterface);
    this->accept(cVisitor);
    fVcth = cVisitor.getThreshold();
    LOG(INFO) << "Checking threshold on latest CBC that was touched...: " << fVcth;

    DetectorDataContainer theHitContainer;
    ContainerFactory::copyAndInitStructure<EmptyContainer,
                                           GenericDataArray<uint32_t, NCHANNELS + 1>,
                                           GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT + 1>,
                                           GenericDataArray<uint32_t, NCHANNELS * NCHIPS_OT * 2 + 1>,
                                           EmptyContainer,
                                           EmptyContainer>(*fDetectorContainer, theHitContainer);
#ifdef __USE_ROOT__
    for(auto cBoard: theHitContainer)
    {
        BeBoard* theBoard = static_cast<BeBoard*>(fDetectorContainer->getObject(cBoard->getId()));
        fBeBoardInterface->Start(theBoard);
        ReadNEvents(theBoard, fNevents);
        const std::vector<Event*>& events = GetEvents();

        for(auto cOpticalGroup: *cBoard)
        {
            json             j;
            std::vector<int> hits(NCHANNELS * NCHIPS_OT * 2, 0);
            for(auto& cEvent: events)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid)
                    {
                        uint32_t chipOffset_module = (cHybrid->getId() * NCHANNELS * NCHIPS_OT) + (cChip->getId() * NCHANNELS);
                        for(uint32_t iCh = 0; iCh < NCHANNELS + 1; iCh++)
                        {
                            if(cEvent->DataBit(cHybrid->getId(), cChip->getId(), 0, iCh)) { hits[chipOffset_module + iCh] += 1; }
                        }
                    }
                }
            } // end events loop
            if(fOfStream != nullptr)
            {
                j["type"]         = "data";
                j["data"]["hits"] = hits;
                *(fOfStream) << j << std::endl;
            }
        } // end module loop
    }
#endif
}

void OTQuickNoise::parseSettings()
{
    // now read the settings from the map
    fNevents    = findValueInSettings<double>("Nevents", 1000);
    fManualVcth = findValueInSettings<double>("CMNoise_manualVcth", 0);

    LOG(INFO) << "Parsed the following settings:";
    LOG(INFO) << "	Running " << fNevents;
    LOG(INFO) << "	Manual Vcth " << fManualVcth;
}

void OTQuickNoise::writeObjects() {}

void OTQuickNoise::ConfigureCalibration() {}

void OTQuickNoise::Running()
{
    LOG(INFO) << "Starting noise measurement";
    Initialize();
    SetThresholds();
    TakeData();
    LOG(INFO) << "Done with noise";
}

void OTQuickNoise::Stop()
{
    LOG(INFO) << "Stopping quick noise measurement";
    writeObjects();
    dumpConfigFiles();
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "Quick Noise measurement stopped.";
}

void OTQuickNoise::Pause() {}

void OTQuickNoise::Resume() {}
