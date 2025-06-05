/*!
        \file                                    Tool.h
        \brief                                   Controller of the System, overall wrapper of the framework
        \author                                  Georg AUZINGER
        \version                                 1.0
        \date                                    06/02/15
        Support :                                mail to : georg.auzinger@cern.ch
 */

#ifndef __TOOL_H__
#define __TOOL_H__

#include "System/SystemController.h"

#ifdef __USE_ROOT__
#include "TCanvas.h"
#include "TFile.h"
#include "TH1.h"
#include "TObject.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TTree.h"
#endif

class DetectorContainer;
class DetectorDataContainer;
class ChannelGroupHandler;
class ChannelGroupBase;
class ScanBase;
class ConfigureInfo;
class StartInfo;
class MetadataHandler;

#if defined __USE_ROOT__ && defined __HTTP__
#include "THttpServer.h"
#endif

typedef std::vector<uint16_t>                         MaskedChannels;
typedef std::map<std::string, MaskedChannels>         MaskedChannelsList;
typedef std::vector<std::pair<std::string, uint16_t>> RegisterVector;

/*!
 * \class Tool
 * \brief A base class for all kinds of applications that have to use ROOT that inherits from SystemController which
 * does not have any dependence on ROOT
 */
class Tool : public Ph2_System::SystemController
{
#ifdef __USE_ROOT__
    using ChipHistogramMap    = std::map<ChipContainer*, std::map<std::string, TObject*>>;
    using HybridHistogramMap  = std::map<HybridContainer*, std::map<std::string, TObject*>>;
    using BeBoardHistogramMap = std::map<BoardContainer*, std::map<std::string, TObject*>>;
    using CanvasMap           = std::map<BaseContainer*, TCanvas*>;
#endif

    using TestGroupChannelMap = std::map<int, std::vector<uint8_t>>;

  public:
    Tool();
#if defined __USE_ROOT__ && defined __HTTP__
    Tool(THttpServer* pServer);
#endif
    Tool(const Tool& pTool);
    ~Tool();

    void Inherit(const Tool* pTool);
    void Inherit(const Ph2_System::SystemController* pSystemController);
    void resetPointers();
    void Destroy();
    void SoftDestroy();
    void readBitslipRegs(); // TODO: once bitslip reload works, it can be removed
#ifdef __USE_ROOT__
    /*!
     * \brief Initialize a 'summary' TTree in the ROOT File, with branches 'parameter'(string) and 'value'(double)
     */
    void bookSummaryTree();

    /*!
     * \brief Insert data into the summary tree
     * \param cParameter : Name of the measurement to be stored
     * \param cValue: Value of the measurement to be stored
     */
    void fillSummaryTree(std::string cParameter, Double_t cValue);

    Double_t getSummaryParameter(std::string cParameter);

    void bookHistogram(ChipContainer* pChip, std::string pName, TObject* pObject);
    void bookHistogram(HybridContainer* pHybrid, std::string pName, TObject* pObject);
    void bookHistogram(BoardContainer* pBeBoard, std::string pName, TObject* pObject);

    TObject* getHist(ChipContainer* pChip, std::string pName);
    TObject* getHist(HybridContainer* pHybrid, std::string pName);
    TObject* getHist(BoardContainer* pBeBoard, std::string pName);
#endif

    void WriteRootFile();

    virtual void sendData() {};
    virtual void ConfigureCalibration() {};
    virtual void Running() {};
    virtual bool GetRunningStatus();

    void Configure(const ConfigureInfo& theConfigureInfo, bool pReInitialize = true) override;

    void Start(const StartInfo& theStartInfo) override;
    /* void InformImDone(); // @Mauro@ */
    void Stop() override;

    void waitForRunToBeCompleted();
    /* void privateRunning(std::promise<int>&& thePromise); // @Mauro@ */
    void SaveResults();
    void CloseResultFile();

    /*!
     * \brief Create a result directory at the specified path + ChargeMode + Timestamp
     * \param pDirectoryname : the name of the directory to create
     * \param pDate : apend the current date and time to the directoryname
     */
    void CreateResultDirectory(const std::string& pDirname, bool pMode = true, bool pDate = true);

    /*!
     * \brief Initialize the result Root file
     * \param pFilename : Root filename
     */
    void InitResultFile(const std::string& pFilename);
#ifdef __USE_ROOT__
    void AddMetadata();
    void StartHttpServer(const int pPort = 8080, bool pReadonly = true);
    void HttpServerProcess();
#endif

    void readAllReadOnlyRegisters();
    void dumpConfigFiles(bool checkReadOnlyRegisters = true);
    // General stuff that can be useful
    void setSystemTestPulse(uint8_t pTPAmplitude, uint8_t pTestGroup, bool pTPState = false, bool pHoleMode = false);
    // Enable test pulse
    void enableTestPulse(bool enableTP);
    // Enable commissioning loops and Test Pulse
    void setFWTestPulse(bool inject);
    // Make test groups for everything Test pulse or Calibration
    void setTestAllChannels(bool pAllChan) { fAllChan = pAllChan; }
    void SetTestPulse(bool pTestPulse) { fTestPulse = pTestPulse; }
    void SetHybridBroadcast(bool pDoBroadcast) { fDoHybridBroadcast = pDoBroadcast; }
    void SetBoardBroadcast(bool pDoBroadcast) { fDoBoardBroadcast = pDoBroadcast; }
    void SetSkipMaskedChannels(bool pSkipMaskedChannels) { fSkipMaskedChannels = pSkipMaskedChannels; }
    // For hybrid testing
    void CreateReport();
    void AmmendReport(std::string pString);

    // helper methods
    void ProcessRequests()
    {
#if defined __USE_ROOT__ && defined __HTTP__
        if(fHttpServer) fHttpServer->ProcessRequests();
#endif
    }

    std::string getResultFileName()
    {
        if(!fResultFileName.empty())
            return fResultFileName;
        else
            return "";
    }

    void setRegBit(uint16_t& pRegValue, uint8_t pPos, bool pValue) { pRegValue ^= (-pValue ^ pRegValue) & (1 << pPos); }
    void toggleRegBit(uint16_t& pRegValue, uint8_t pPos) { pRegValue ^= 1 << pPos; }
    bool getBit(uint16_t& pRegValue, uint8_t pPos) { return (pRegValue >> pPos) & 1; }

    /*!
     * \brief reverse the endianess before writing in to the register
     * \param pDelay: the actual delay
     * \param pGroup: the actual group number
     * \return the reversed endianness
     */
    inline uint8_t to_reg(uint8_t pDelay, uint8_t pGroup)
    {
        uint8_t cValue = ((reverse(pDelay)) & 0xF8) | ((reverse(pGroup)) >> 5);

        // LOG(DBUG) << std::bitset<8>( cValue ) << " cGroup " << +pGroup << " " << std::bitset<8>( pGroup ) << " pDelay
        // " << +pDelay << " " << std::bitset<8>( pDelay ) ;
        return cValue;
    }

    /*!
     * \brief reverse the byte
     * \param n:the number to be reversed
     * \return the reversed number
     */
    inline uint8_t reverse(uint8_t n)
    {
        // Reverse the top and bottom nibble then swap them.
        return (fLookup[n & 0xF] << 4) | fLookup[n >> 4];
    }

    unsigned char fLookup[16] = {
        0x0,
        0x8,
        0x4,
        0xc,
        0x2,
        0xa,
        0x6,
        0xe,
        0x1,
        0x9,
        0x5,
        0xd,
        0x3,
        0xb,
        0x7,
        0xf,
    }; /*!< Lookup table for reverce the endianness */

    std::map<uint8_t, std::string> fChannelMaskMapCBC3 = {
        {0, "MaskChannel-008-to-001"},  {1, "MaskChannel-016-to-009"},  {2, "MaskChannel-024-to-017"},  {3, "MaskChannel-032-to-025"},  {4, "MaskChannel-040-to-033"},  {5, "MaskChannel-048-to-041"},
        {6, "MaskChannel-056-to-049"},  {7, "MaskChannel-064-to-057"},  {8, "MaskChannel-072-to-065"},  {9, "MaskChannel-080-to-073"},  {10, "MaskChannel-088-to-081"}, {11, "MaskChannel-096-to-089"},
        {12, "MaskChannel-104-to-097"}, {13, "MaskChannel-112-to-105"}, {14, "MaskChannel-120-to-113"}, {15, "MaskChannel-128-to-121"}, {16, "MaskChannel-136-to-129"}, {17, "MaskChannel-144-to-137"},
        {18, "MaskChannel-152-to-145"}, {19, "MaskChannel-160-to-153"}, {20, "MaskChannel-168-to-161"}, {21, "MaskChannel-176-to-169"}, {22, "MaskChannel-184-to-177"}, {23, "MaskChannel-192-to-185"},
        {24, "MaskChannel-200-to-193"}, {25, "MaskChannel-208-to-201"}, {26, "MaskChannel-216-to-209"}, {27, "MaskChannel-224-to-217"}, {28, "MaskChannel-232-to-225"}, {29, "MaskChannel-240-to-233"},
        {30, "MaskChannel-248-to-241"}, {31, "MaskChannel-254-to-249"}};

    // Statistical methods
    std::pair<float, float>                           getStats(std::vector<float> pData);
    std::pair<float, float>                           evalNoise(std::vector<float> pData, std::vector<float> pWeights, bool pIgnoreNegative = true);
    std::pair<std::vector<float>, std::vector<float>> getDerivative(std::vector<float> pData, std::vector<float> pWeights, bool pIgnoreNegative = true);

    // Decode bend LUT for a given CBC
    std::map<uint8_t, double> decodeBendLUT(Ph2_HwDescription::Chip* pChip);

    // Un-mask pairs of channels on a given CBC
    void unmaskPair(Ph2_HwDescription::Chip* cChip, std::pair<uint8_t, uint8_t> pPair);

    // Select the group of channels for injecting the pulse
    void selectGroupTestPulse(Ph2_HwDescription::Chip* cChip, uint8_t pTestGroup);

    // Two dimensional dac scan
    void scanDacDac(const std::string&                               dac1Name,
                    const std::vector<uint16_t>&                     dac1List,
                    const std::string&                               dac2Name,
                    const std::vector<uint16_t>&                     dac2List,
                    uint32_t                                         numberOfEvents,
                    std::vector<std::vector<DetectorDataContainer*>> detectorContainerVectorOfVector,
                    int32_t                                          numberOfEventsPerBurst = -1);

    // Two dimensional dac scan per BeBoard
    void scanBeBoardDacDac(uint16_t                                         boardId,
                           const std::string&                               dac1Name,
                           const std::vector<uint16_t>&                     dac1List,
                           const std::string&                               dac2Name,
                           const std::vector<uint16_t>&                     dac2List,
                           uint32_t                                         numberOfEvents,
                           std::vector<std::vector<DetectorDataContainer*>> detectorContainerVector,
                           int32_t                                          numberOfEventsPerBurst = -1);

    // One dimensional dac scan
    void scanDac(const std::string&                  dacName,
                 const std::vector<uint16_t>&        dacList,
                 uint32_t                            numberOfEvents,
                 std::vector<DetectorDataContainer*> detectorContainerVector,
                 int32_t                             numberOfEventsPerBurst = -1);

    // One dimensional dac scan per BeBoard
    void scanBeBoardDac(uint16_t                             boardId,
                        const std::string&                   dacName,
                        const std::vector<uint16_t>&         dacList,
                        uint32_t                             numberOfEvents,
                        std::vector<DetectorDataContainer*>& detectorContainerVector,
                        int32_t                              numberOfEventsPerBurst = -1);
    // Bit wise scan
    void bitWiseScan(const std::string& dacName, uint32_t numberOfEvents, const float& targetOccupancy, int32_t numberOfEventsPerBurst = -1);
    // Bit wise scan per BeBoard
    void bitWiseScanBeBoard(uint16_t boardId, const std::string& dacName, uint32_t numberOfEvents, const float& targetOccupancy, int32_t numberOfEventsPerBurst = -1);
    // Full scan
    void fullScan(const std::string& dacName, uint32_t numberOfEvents, const float& targetOccupancy, int32_t numberOfEventsPerBurst = -1, int32_t startVal = 110, bool mask = false);
    // Full scan per BeBoard
    void fullScanBeBoard(uint16_t           boardId,
                         const std::string& dacName,
                         uint32_t           numberOfEvents,
                         const float&       targetOccupancy,
                         int32_t            numberOfEventsPerBurst = -1,
                         int32_t            startVal               = 110,
                         bool               mask                   = false);

    // Set dac and measure data
    void setDacAndMeasureData(const std::string& dacName, const uint16_t dacValue, uint32_t numberOfEvents, int32_t numberOfEventsPerBurst = -1);
    // Set dac and measure data per BeBoard
    void setDacAndMeasureBeBoardData(uint16_t boardId, const std::string& dacName, const uint16_t dacValue, uint32_t numberOfEvents, int32_t numberOfEventsPerBurst = -1);
    // Measure data
    void measureData(uint32_t numberOfEvents, int32_t numberOfEventsPerBurst = -1);
    // Measure data per BeBoard
    void measureBeBoardData(uint16_t boardId, uint32_t numberOfEvents, int32_t numberOfEventsPerBurst = -1);
    // Measure data per BeBoard and per group
    // void measureBeBoardDataPerGroup(uint16_t boardId, const uint16_t numberOfEvents, const ChannelGroupBase *cTestChannelGroup);
    // Set global DAC for all CBCs in the BeBoard
    void setAllGlobalDacBeBoard(uint16_t boardId, const std::string& dacName, DetectorDataContainer& globalDACContainer);
    // Set global DAC for all Chips in the BeBoard
    void setAllLocalDacBeBoard(uint16_t boardId, const std::string& dacName, DetectorDataContainer& globalDACContainer);
    // Set same global DAC for all Chips
    void setSameGlobalDac(const std::string& dacName, const uint16_t dacValue);
    // Set same global DAC for all Chips in the BeBoard
    void setSameGlobalDacBeBoard(Ph2_HwDescription::BeBoard* pBoard, const std::string& dacName, const uint16_t dacValue);
    // Set same local DAC list for all Chips
    void setSameLocalDac(const std::string& dacName, const uint16_t dacValue);
    // Set same local DAC for all Chips in the BeBoard
    void setSameLocalDacBeBoard(Ph2_HwDescription::BeBoard* pBoard, const std::string& dacName, const uint16_t dacValue);
    // Set same DAC for all Chips in the BeBoard (it is able to recognize if the dac is local or global)
    void setSameDacBeBoard(Ph2_HwDescription::BeBoard* pBoard, const std::string& dacName, const uint16_t dacValue);
    // Set same DAC list for all Chips (it is able to recognize if the dac is local or global)
    void setSameDac(const std::string& dacName, const uint16_t dacValue);

    std::string getDirectoryName() { return fDirectoryName; }

    struct StatsSum
    {
      public:
        float fMean;
        float fSum;
        float fSqSum;
        float fStdDev;
        float fNentries;
        float fMin;
        float fMax;
    };
    template <typename T>
    StatsSum SummarizeStats(std::vector<T> cData)
    {
        T cInitVal = (T)(0);
        // Remove NANs
        cData.erase(std::remove_if(cData.begin(), cData.end(), [](T x) { return std::isnan(x); }), cData.end());
        // Calculate stats
        StatsSum cStatsSum;
        cStatsSum.fSum      = std::accumulate(cData.begin(), cData.end(), cInitVal);
        cStatsSum.fMean     = cStatsSum.fSum / cData.size();
        cStatsSum.fMax      = *(std::max_element(cData.begin(), cData.end()));
        cStatsSum.fMin      = *(std::min_element(cData.begin(), cData.end()));
        cStatsSum.fSqSum    = std::inner_product(cData.begin(), cData.end(), cData.begin(), 0.0);
        cStatsSum.fStdDev   = std::sqrt(cStatsSum.fSqSum / cData.size() - cStatsSum.fMean * cStatsSum.fMean);
        cStatsSum.fNentries = cData.size();
        return cStatsSum;
    }

    bool    ifUseReadNEvents() { return fUseReadNEvents; }
    int     getWait() { return fWait_ms; }
    size_t  getNReadbackEvents() { return fNReadbackEvents; }
    void    setNReadbackEvents(size_t pNEvents) { fNReadbackEvents = pNEvents; }
    void    setNormalization(uint8_t pNorm) { fNormalize = pNorm; }
    uint8_t getNormalization() { return fNormalize; }
    void    resetOutputDirectoryName() { fDirectoryName = ""; }

    void           setOfStream(std::ofstream* pOfStream) { fOfStream = pOfStream; };
    std::ofstream* getOfStream() { return fOfStream; };
    std::string    getCalibrationName() const;

  private:
    void doScanOnAllGroupsBeBoard(uint16_t boardId, uint32_t numberOfEvents, int32_t numberOfEventsPerBurst, ScanBase* scanFunctor);

  protected:
    DetectorDataContainer* fDetectorDataContainer{nullptr};

    uint16_t getMaxNumberOfGroups();

#ifdef __USE_ROOT__
    CanvasMap           fCanvasMap;
    ChipHistogramMap    fChipHistMap;
    HybridHistogramMap  fHybridHistMap;
    BeBoardHistogramMap fBeBoardHistMap;
    TTree*              fSummaryTree; /*< TTree for summary of results*/
    static std::string  fSummaryTreeParameter;
    static double       fSummaryTreeValue;
#endif

    FrontEndType        fType;
    TestGroupChannelMap fTestGroupChannelMap;

    std::map<int, std::vector<uint8_t>> fMaskForTestGroupChannelMap;

    std::string fDirectoryName{""}; /*< the Directoryname for the Root file with results */
#ifdef __USE_ROOT__
    TFile* fResultFile; /*< the Name for the Root file with results */
#endif
    std::string fResultFileName;
#if defined __USE_ROOT__ && defined __HTTP__
    THttpServer* fHttpServer;
#endif

    // ################################
    // # Hanldlers for Running thread #
    // ################################
    static std::atomic<bool> fKeepRunning;
    static std::atomic<int>  fRunNumber;
    std::future<void>        fRunningFuture; // @Fabio@
    // @Mauro@
    /* bool                        doExit; */
    /* std::thread                 fRunningThread; */
    /* std::future<int>            fRunningFuture; */
    /* std::condition_variable_any wakeUp; */
    /* std::recursive_mutex        theMtx; */

    bool    fSkipMaskedChannels;
    bool    fAllChan;
    bool    fMaskChannelsFromOtherGroups;
    bool    fTestPulse;
    bool    fDoBoardBroadcast;
    bool    fDoHybridBroadcast;
    bool    fUseReadNEvents{1};
    int     fWait_ms{100};
    size_t  fNReadbackEvents{0};
    uint8_t fNormalize{1};

    std::ofstream* fOfStream;

    MetadataHandler* fMetadataHandler{nullptr};
};

#endif
