/*!

        \file                                            CICInterface.h
        \brief                                           User Interface to the Cics
        \version                                         1.0

 */

#ifndef __CICINTERFACE_H__
#define __CICINTERFACE_H__
#include "HWInterface/ChipInterface.h"
#include "HWInterface/D19clpGBTInterface.h"

template <typename T, size_t N, size_t... S>
class GenericDataArray;

/*!
 * \namespace Ph2_HwInterface
 * \brief Namespace regrouping all the interfaces to the hardware
 */
namespace Ph2_HwInterface
{
/*!
 * \class CicInterface
 * \brief Class representing the User Interface to the Cic on different boards
 */
class CicInterface : public ChipInterface
{
  public:
    /*!
     * \brief Constructor of the CICInterface Class
     * \param pBoardMap
     */
    CicInterface(const BeBoardFWMap& pBoardMap);
    /*!
     * \brief Destructor of the CICInterface Class
     */
    ~CicInterface();

    /*!
     * \brief Configure the Cic with the Cic Config File
     * \param pCic: pointer to CIC object
     * \param pVerify: perform a readback check
     * \param pBlockSize: the number of registers to be written at once, default is 310
     */
    bool ConfigureChip(Ph2_HwDescription::Chip* pCic, bool pVerify = true, uint32_t pBlockSize = 310) override;
    void CheckConfig(Ph2_HwDescription::Chip* pChip);

    /*!
     * \brief Write the designated register in both Chip and Chip Config File
     * \param pChip
     * \param pRegNode : Node of the register to write
     * \param pValue : Value to write
     */
    bool WriteChipReg(Ph2_HwDescription::Chip* pChip, const std::string& pRegNode, uint16_t pValue, bool pVerify = true) override;

    /*!
     * \brief Write several registers in both Chip and Chip Config File
     * \param pChip
     * \param pVecReq : Vector of pair: Node of the register to write versus value to write
     */
    bool WriteChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::pair<std::string, uint16_t>>& pVecReq, bool pVerify = true) override;

    /*!
     * \brief Read the designated register in the Chip
     * \param pChip
     * \param pRegNode : Node of the register to read
     */
    int32_t                                       ReadChipReg(Ph2_HwDescription::Chip* pChip, const std::string& pRegNode) override;
    std::vector<std::pair<std::string, uint16_t>> ReadChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& theRegisterList) override;

    uint8_t ReadFCMDEdge(Ph2_HwDescription::Chip* pChip);

    uint32_t                    ReadChipFuseID(Ph2_HwDescription::Chip* pCic, uint8_t version = 1) override;
    bool                        SetFePhaseTap(Ph2_HwDescription::Chip* pChip, uint8_t pFeId, uint8_t pLineId, int pPhaseTap);
    bool                        SetPhaseTap(Ph2_HwDescription::Chip* pChip, uint8_t pPhyPort, uint8_t pPhyPortChannel, int pPhaseTap);
    std::pair<uint8_t, uint8_t> fromChipL1ToPhyPortAndChannel(Ph2_HwDescription::Chip* pChip, std::vector<uint8_t> chipToCICMapping, uint8_t frontEndId);
    std::pair<uint8_t, uint8_t> fromChipStubToPhyPortAndChannel(Ph2_HwDescription::Chip* pChip, std::vector<uint8_t> chipToCICMapping, uint8_t frontEndId, uint8_t stubLine);
    std::pair<uint8_t, uint8_t> fromPhyPortAndChanneltoChipIdAndLine(Ph2_HwDescription::Chip* pChip, uint8_t phyPort, uint8_t channel);
    uint8_t                     fromChipIdToCICFEid(Ph2_HwDescription::Chip* pChip, uint8_t chipId);
    uint8_t                     fromCICFEidToChipId(Ph2_HwDescription::Chip* pChip, uint8_t cicFEid);

    GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS> getAllOptimalTaps(Ph2_HwDescription::Chip* pChip);
    GenericDataArray<bool, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>    getLineLocked(Ph2_HwDescription::Chip* pChip);
    bool                        writeAllTaps(Ph2_HwDescription::Chip* pChip, GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS> cicInputTaps);
    bool                        SetSparsification(Ph2_HwDescription::Chip* pChip, uint8_t pState = 0);
    bool                        PhaseAlignerPorts(Ph2_HwDescription::Chip* pChip, uint8_t pState);
    bool                        SetStaticPhaseAlignment(Ph2_HwDescription::Chip* pChip);
    bool                        SetAutomaticPhaseAlignment(Ph2_HwDescription::Chip* pChip, bool pAuto = true);
    bool                        SetStaticWordAlignment(Ph2_HwDescription::Chip* pChip);
    bool                        CheckPhaseAlignerLock(Ph2_HwDescription::Chip* pChip, uint8_t pCheckValue = 0xFF);
    bool                        ResetPhaseAligner(Ph2_HwDescription::Chip* pChip, uint16_t pWait_ms = 1);
    bool                        ResetDLL(Ph2_HwDescription::Chip* pChip, uint16_t pWait_ms = 100);
    bool                        CheckDLL(Ph2_HwDescription::Chip* pChip);
    bool                        CheckFastCommandLock(Ph2_HwDescription::Chip* pChip);
    bool                        ConfigureAlignmentPatterns(Ph2_HwDescription::Chip* pChip, std::vector<uint8_t> pAlignmentPatterns);
    bool                        AutomatedWordAlignment(Ph2_HwDescription::Chip* pChip, std::vector<uint8_t> pAlignmentPatterns);
    bool                        PrepareForAutomatedBX0Alignment(Ph2_HwDescription::Chip* pChip, std::vector<uint8_t> pAlignmentPatterns, uint8_t pLine, uint8_t pFEChip);
    bool                        AutomatedBX0Alignment(Ph2_HwDescription::Chip* pChip, std::vector<uint8_t> pAlignmentPatterns);
    bool                        CheckAutomatedBX0Alignment(Ph2_HwDescription::Chip* pChip);
    bool                        ConfigureBx0Alignment(Ph2_HwDescription::Chip* pChip, std::vector<uint8_t> pPatterns, uint8_t pFEId = 0, uint8_t pLineId = 0);
    std::pair<bool, uint8_t>    CheckBx0Alignment(Ph2_HwDescription::Chip* pChip); // success , delay
    bool                        CheckReSync(Ph2_HwDescription::Chip* pChip);
    bool                        SoftReset(Ph2_HwDescription::Chip* pChip, uint32_t cWait_ms = 100);
    bool                        CheckSoftReset(Ph2_HwDescription::Chip* pChip);
    bool                        StartUp(Ph2_HwDescription::Chip* pChip);
    bool                        ManualBx0Alignment(Ph2_HwDescription::Chip* pChip, uint8_t pBx0delay = 8);
    bool                        SelectMode(Ph2_HwDescription::Chip* pChip, uint8_t pMode = 0);
    bool                        SelectOutput(Ph2_HwDescription::Chip* pChip, bool pFixedPattern = true);
    bool                        EnableFEs(Ph2_HwDescription::Chip* pChip, std::vector<uint8_t> pFEs = {0, 1, 2, 3, 4, 5, 6, 7}, bool pEnable = true);
    bool                        configureEnabledFEs(Ph2_HwDescription::Chip* pChip, std::vector<uint8_t> pFeIds);
    bool                        AutoBx0Alignment(Ph2_HwDescription::Chip* pChip, uint8_t pStatus);
    bool                        SelectMux(Ph2_HwDescription::Chip* pChip, uint8_t pPhyPort);
    bool                        ControlMux(Ph2_HwDescription::Chip* pChip, uint8_t pEnable);
    bool                        ConfigureStubOutput(Ph2_HwDescription::Chip* pChip, uint8_t pLineSel = 0);
    bool                        ConfigureTermination(Ph2_HwDescription::Chip* pChip, uint8_t pClkTerm = 1, uint8_t pRxTerm = 1);
    bool                        ConfigureDriveStrength(Ph2_HwDescription::Chip* pChip, uint8_t pDriveStrength = 3);
    bool                        ConfigureFCMDEdge(Ph2_HwDescription::Chip* pChip, uint8_t pUseNegEdge = 1);
    bool                        GetResyncRequest(Ph2_HwDescription::Chip* pChip);
    std::pair<uint8_t, uint8_t> GetPhyPortConfig(Ph2_HwDescription::Chip* pChip, uint8_t pFeId, uint8_t pLineId);
    // TO-DO.. clean-up
    bool                          runVerification(Ph2_HwDescription::Chip* pChip, uint8_t pValue, std::string pRegName);
    std::pair<uint16_t, uint16_t> getRetrySummary() { return std::make_pair(fReW, fReWR); }
    std::pair<int, float>         getWRattempts();
    std::pair<float, float>       getMinMaxWRattempts();
    std::pair<uint16_t, uint16_t> getReadBackErrorSummary() { return std::make_pair(fReadBackErrors, fRegisterWrites); }
    std::pair<uint16_t, uint16_t> getWriteErrorSummary() { return std::make_pair(fWriteErrors, fRegisterWrites); }
    std::pair<uint16_t, uint16_t> getConfigSumary() { return std::make_pair(fSuccRegisterWrites, fSuccRegisterRbs); }
    void                          resetStatusLog() { fI2CStatus.clear(); }
    void                          resetRetrySummary()
    {
        fReWMap.clear();
        fReWrMap.clear();
        fReW  = 0;
        fReWR = 0;
    }
    void resetErrorSummary()
    {
        fWriteErrorMap.clear();
        fReadBackErrorMap.clear();
        fRegisterWrites = 0;
        fReadBackErrors = 0;
        fWriteErrors    = 0;
        resetRetrySummary();
    }
    void printErrorSummary();
    void setRetryI2C(bool pRetry) { fRetryI2C = pRetry; }
    void setMaxI2CAttempts(uint8_t pMaxAttempts) { fMaxI2CAttempts = pMaxAttempts; }
    // return information on phase aligners
    std::vector<uint8_t>                                                              getI2CStatus() { return fI2CStatus; }
    void                                                                              setWithlpGBT(uint8_t pIsWithLpGBT) {}
    GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS - 1> retrieveExternalWordAlignmentValues(Ph2_HwDescription::Chip* pChip);
    bool     ConfigureExternalWordAlignment(Ph2_HwDescription::Chip* pChip, const GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS - 1>& theWordAlignmentValues);
    uint16_t retrieveExternalBX0AlignmentValue(Ph2_HwDescription::Chip* pChip);
    bool     ConfigureExternalBX0Delay(Ph2_HwDescription::Chip* pChip, const uint16_t theBX0AlignmentValues);

  private:
    bool    fRetryI2C       = true;
    uint8_t fMaxI2CAttempts = 20;

    std::map<uint8_t, std::string> fMap;
    std::map<uint8_t, uint16_t>    fReWMap;
    std::map<uint8_t, uint16_t>    fReWrMap;
    std::map<uint8_t, uint16_t>    fWriteErrorMap;
    std::map<uint8_t, uint16_t>    fReadBackErrorMap;
    std::vector<uint8_t>           fI2CStatus;

    uint16_t fAttemptedWrites    = 0;
    uint16_t fSuccRegisterWrites = 0;
    uint16_t fSuccRegisterRbs    = 0;
    uint16_t fRegisterWrites     = 0;
    uint16_t fReadBackErrors     = 0;
    uint16_t fWriteErrors        = 0;
    uint16_t fReW                = 0;
    uint16_t fReWR               = 0;

  protected:
    std::map<uint8_t, uint8_t> fTxDriveStrength  = {{0, 0}, {1, 2}, {2, 6}, {3, 1}, {4, 3}, {5, 7}};
    uint8_t                    fMaxDriveStrength = 5;
    // 4 channels per phyPort ... 12 phyPorts per CIC

    // register map
};
} // namespace Ph2_HwInterface

#endif
