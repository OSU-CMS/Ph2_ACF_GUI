/*!
  \file                  RD53CalibBase.h
  \brief                 Header of CalibBase
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53CalibBase_H
#define RD53CalibBase_H

#include "HWDescription/RD53.h"
#include "MetadataHandlerIT.h"
#include "MonitorUtils/DetectorMonitor.h"
#include "Tool.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/RD53ChannelGroupHandler.h"

#ifdef __USE_ROOT__
#include "TApplication.h"
#include "TKey.h"
#endif

// ##################################
// # Basic class for IT calibration #
// ##################################
class CalibBase : public Tool
{
  public:
    CalibBase() : showErrorReport(true) {}
    ~CalibBase() { CalibBase::Stop(); }
    void    chipErrorReport() const;
    void    copyMaskFromDefault(const std::string& which = "all") const;
    void    saveChipRegisters(bool doUpdateChip);
    void    downloadNewDACvalues(DetectorDataContainer&          DACcontainer,
                                 const std::vector<const char*>& regNames,
                                 bool                            silentDownload = false,
                                 bool                            checkAgainst   = false,
                                 int                             value          = 0,
                                 bool                            doNotDownload  = false);
    void    saveSCurveOrGaindValues(const std::vector<DetectorDataContainer*>& detectorContainerVector, const std::vector<uint16_t>& dacList, size_t offset, size_t nEvents, const std::string& name);
    uint8_t assignGroupType(RD53Shared::INJtype injType) const;
    void    prepareChipQueryForEnDis(const std::string& queryName);
    void    setChipEnDis(bool enable);
    bool    shiftEnable(size_t indx);
    void    setSinglePixel(Ph2_HwDescription::ReadoutChip* pChip, size_t row, size_t col, bool enable, bool inject);
    void    ResetBoardsReadBkFIFO();
    void    ResetBoards();
    void    WriteBroadcastChipReg(const std::string& regName, const uint16_t value);
    void    SilentRunning(bool doSilentRunning);

    void Stop() override;
    void ConfigureCalibration() override;

    virtual void   localConfigure(const std::string& histoFileName, int currentRun);
    virtual void   run()                      = 0;
    virtual void   draw(bool saveData = true) = 0;
    virtual size_t getNumberIterations() { return 0; };

    template <typename T>
    void bookHistoSaveMetadata(T* histos)
    {
#ifdef __USE_ROOT__
        if(histos->AreHistoBooked == false) histos->book(this->fResultFile, *fDetectorContainer, fSettingsMap);
#endif

        // ##################################
        // # Fill metadata final conditions #
        // ##################################
        if(this->fMetadataHandler != nullptr)
        {
            LOG(INFO) << BOLDBLUE << "\t--> Saving and/or shipping metadata..." << RESET;
            this->fMetadataHandler->fillFinalConditionsHardwareSpecific();
        }
    }

    template <typename T>
    void initializeFiles(const std::string& histoFileName, const std::string& calibName, T*& histos, int currentRun = -1, bool saveBinaryData = false)
    {
        if(saveBinaryData == true)
        {
            LOG(INFO) << BOLDBLUE << "\t--> Calibration initializing raw data file..." << RESET;
            this->fDirectoryName = dataOutputDir != "" ? dataOutputDir : RD53Shared::RESULTDIR;
            this->addFileHandler(std::string(this->fDirectoryName) + "/Run" + RD53Shared::fromInt2Str(currentRun) + "_" + calibName + ".raw", 'w');
            this->initializeWriteFileHandler();
        }

        delete histos;
        histos = new T;

#ifdef __USE_ROOT__
        if((this->fResultFile == nullptr) || (this->fResultFile->IsOpen() == false))
        {
            LOG(INFO) << BOLDBLUE << "\t--> Calibration initializing root file..." << RESET;
#else
        {
#endif
            this->InitResultFile(histoFileName);

            // #################
            // # Book metadata #
            // #################
            if(this->fMetadataHandler == nullptr)
            {
                this->fMetadataHandler = new MetadataHandlerIT(getTimeStampString());
                this->fMetadataHandler->Inherit(this);
                this->fMetadataHandler->initMetadata();
                this->fMetadataHandler->justBookDQMMetadata();
            }

            // ####################################
            // # Fill metadata initial conditions #
            // ####################################
            this->fMetadataHandler->fillInitialConditionsHardwareSpecific();
            this->WriteRootFile();
        }
    }

    template <typename T>
    void fillVectorContainer(DetectorDataContainer& theDataContainer, const size_t nElements, const T value, const int fromBoardId = -1)
    {
        for(const auto cBoard: theDataContainer)
        {
            const auto& theBoard = (fromBoardId < 0 ? cBoard : theDataContainer.getObject(fromBoardId));

            for(const auto cOpticalGroup: *theBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    for(const auto cChip: *cHybrid)
                    {
                        cChip->getSummary<std::vector<T>>().clear();
                        for(auto i = 0u; i < nElements; i++) cChip->getSummary<std::vector<T>>().push_back(value);
                    }

            if(fromBoardId >= 0) break;
        }
    }

    size_t nTRIGxEvent;
    int    theCurrentRun;

  protected:
    // ######################################
    // # Parameters from configuration file #
    // ######################################
    size_t      rowStart;
    size_t      rowStop;
    size_t      colStart;
    size_t      colStop;
    size_t      nEvents;
    size_t      nEvtsBurst;
    bool        splitFile;
    std::string dataOutputDir;

    bool showErrorReport;

  private:
    virtual void fillHisto() = 0;

    // #######################################
    // # Split output file by Board & Hybrid #
    // #######################################
#ifdef __USE_ROOT__
    bool splitHistoFile(TFile* theInputFile);
    void copyDirectories(TFile* theInputFile, TFile* theOutputFile, const std::string& boardName, const std::string& hybridName);
    void copyContent(TFile* theInputFile, TFile* theOutputFile, const std::string& dirName);
    bool openRootFileFolder(TFile* theInputFile, const std::string& folderName);
#endif
};

#endif
