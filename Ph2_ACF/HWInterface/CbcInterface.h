/*!

        \file                                            CBCInterface.h
        \brief                                           User Interface to the Cbcs
        \author                                          Lorenzo BIDEGAIN, Nicolas PIERRE
        \version                                         1.0
        \date                        31/07/14
        Support :                    mail to : lorenzo.bidegain@gmail.com, nico.pierre@icloud.com

 */

#ifndef __CBCINTERFACE_H__
#define __CBCINTERFACE_H__

#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/ReadoutChipInterface.h"

#include <vector>

/*!
 * \namespace Ph2_HwInterface
 * \brief Namespace regrouping all the interfaces to the hardware
 */
namespace Ph2_HwInterface
{
/*!
 * \class CbcInterface
 * \brief Class representing the User Interface to the Chip on different boards
 */
class CbcInterface : public ReadoutChipInterface
{
  public:
    /*!
     * \brief Constructor of the CBCInterface Class
     * \param pBoardMap
     */
    CbcInterface(const BeBoardFWMap& pBoardMap);

    /*!
     * \brief Destructor of the CBCInterface Class
     */
    ~CbcInterface();

    /*!
     * \brief Configure the Chip with the Chip Config File
     * \param pCbc: pointer to CBC object
     * \param pVerify: perform a readback check
     * \param pBlockSize: the number of registers to be written at once, default is 310
     */
    bool ConfigurePage(Ph2_HwDescription::Chip* pCbc, uint8_t pPage, bool pVerify = true);
    bool ConfigureChip(Ph2_HwDescription::Chip* pCbc, bool pVerify = true, uint32_t pBlockSize = 310) override;

    void produceL1phaseAlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip);
    void produceStubLine0PhaseAlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip);
    void produceStubLines1To4PhaseAlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip);

    void producePhaseAlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip, uint8_t pWait_ms = 1) override;
    void produceWordAlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip) override;
    void produceBX0AlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip) override;

    bool setInjectionSchema(Ph2_HwDescription::ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool pVerify = true) override;

    bool enableInjection(Ph2_HwDescription::ReadoutChip* pChip, bool inject = true, bool pVerify = true) override;

    bool setInjectionAmplitude(Ph2_HwDescription::ReadoutChip* pChip, uint8_t injectionAmplitude, bool pVerify = true) override;

    bool maskChannelGroup(Ph2_HwDescription::ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool pVerify = true) override;

    bool maskChannelsAndSetInjectionSchema(Ph2_HwDescription::ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool mask, bool inject, bool pVerify = true) override;

    /*!
     * \brief Reapply the stored mask for the CBC, use it after group masking is applied
     * \param pCbc: pointer to CBC object
     */
    bool ConfigureChipOriginalMask(Ph2_HwDescription::ReadoutChip* pCbc, bool pVerify = true, uint32_t pBlockSize = 310) override;

    /*!
     * \brief Mask all channels of the chip
     * \param pCbc: pointer to Chip object
     * \param mask: if true mask, if false unmask
     * \param pVerify: perform a readback check
     * \param pBlockSize: the number of registers to be written at once, default is 310
     */
    bool MaskAllChannels(Ph2_HwDescription::ReadoutChip* pCbc, bool mask, bool pVerify = true) override;

    /*!
     * \brief Write the designated register in both Chip and Chip Config File
     * \param pCbc
     * \param pRegNode : Node of the register to write
     * \param pValue : Value to write
     */
    bool WriteChipReg(Ph2_HwDescription::Chip* pCbc, const std::string& pRegNode, uint16_t pValue, bool pVerify = true) override;

    /*!
     * \brief Write several registers in both Chip and Chip Config File
     * \param pCbc
     * \param pVecReq : Vector of pair: Node of the register to write versus value to write
     */
    bool WriteChipMultReg(Ph2_HwDescription::Chip* pCbc, const std::vector<std::pair<std::string, uint16_t>>& pVecReq, bool pVerify = true) override;

    /*!
     * \brief Write all Local registers on Cbc and Cbc Config File (able to recognize local parameter names such as
     * ChannelOffset) \param pCbc \param pRegNode : Node of the register to write \param pValue : Value to write
     */
    bool WriteChipAllLocalReg(Ph2_HwDescription::ReadoutChip* pCbc, const std::string& dacName, const ChipContainer& pValue, bool pVerify = true) override;

    /*!
     * \brief Read the designated register in the Chip
     * \param pCbc
     * \param pRegNode : Node of the register to read
     */
    int32_t ReadChipReg(Ph2_HwDescription::Chip* pCbc, const std::string& pRegNode) override;

    std::vector<std::pair<std::string, uint16_t>> ReadChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& theRegisterList) override;

    // cbc specific functions
    std::vector<uint8_t> createHitListFromStubs(uint8_t pSeed, bool pSeedLayer);
    std::vector<uint8_t> stubInjectionPattern(uint8_t pStubAddress, int pStubBend,
                                              bool pLayerSwap); // address + bend in units of half strips
    std::vector<uint8_t> stubInjectionPattern(Ph2_HwDescription::ReadoutChip* pChip, uint8_t pStubAddress, int pStubBend);
    bool                 selectLogicMode(Ph2_HwDescription::ReadoutChip* pCbc, std::string pModeSelect, bool pForHits, bool pForStubs, bool pVerify = true);
    bool                 enableHipSuppression(Ph2_HwDescription::ReadoutChip* pCbc, bool pForHits, bool pForStubs, uint8_t pClocks, bool pVerify = true);
    bool                 injectClusters(Ph2_HwDescription::ReadoutChip* pCbc, std::vector<std::pair<uint8_t, uint8_t>> theClusterAddressAndWidthVector);
    bool                 injectStubs(Ph2_HwDescription::ReadoutChip*      pCbc,
                                     std::vector<std::pair<uint8_t, int>> theStubAddressesAndBendVector,
                                     bool                                 pUseNoise   = true,
                                     bool                                 pUseOffsets = false,
                                     uint8_t                              pAllOff     = 0xFF); // address + bend in units of half strips
    uint16_t             readErrorRegister(Ph2_HwDescription::ReadoutChip* pCbc);
    std::vector<uint8_t> readLUT(Ph2_HwDescription::ReadoutChip* pCbc, uint8_t pMode = 0);
    uint8_t              GetLastPage(Ph2_HwDescription::Chip* pCbc);
    void                 resetPageMap() { fPageMap.clear(); }

    std::vector<uint8_t> getWordAlignmentPatterns() override { return fWordAlignmentPatterns; }
    std::vector<uint8_t> getBX0AlignmentPatterns() override { return fBX0AlignmentPatterns; }
    /*!
     * \brief Read CBC ID eFuse
     * \param pChip: pointer to Chip object
     */
    uint32_t ReadChipFuseID(Ph2_HwDescription::Chip* pChip, uint8_t version = 1) override;

  private:
    bool                 fSortPageInc           = true;
    std::vector<uint8_t> fWordAlignmentPatterns = {0x7A, 0xBC, 0xD4, 0x31, 0x81};
    // std::vector<uint8_t>        fBX0AlignmentPatterns  = {0x00, 0x00, 0x00, 0x00, 0x80};
    std::vector<uint8_t>        fBX0AlignmentPatterns = {0x0A, 0x0A, 0x0A, 0x99, 0x89};
    bool                        fRetry                = true;
    std::map<uint32_t, uint8_t> fPageMap;
    bool                        fWithlpGBT = false;
    std::bitset<NCHANNELS>      fActiveChannels;
    // void ReadAllCbc ( const Hybrid* pHybrid );
    // void CbcCalibrationTrigger(const Cbc* pCbc );
    void output();

    /*!
     * \brief Write the designated register in both Chip and Chip Config File
     * \param pCbc
     * \param pRegNode : Node of the register to write
     * \param pValue : Value to write
     */
    bool    WriteChipSingleReg(Ph2_HwDescription::Chip* pCbc, const std::string& pRegNode, uint16_t pValue, bool pVerify = true);
    uint8_t ReadChipSingleReg(Ph2_HwDescription::Chip* pCbc, const std::string& pRegNode);

    std::map<uint8_t, std::string> fChannelMaskMapCBC3 = {
        {0, "MaskChannel-008-to-001"},  {1, "MaskChannel-016-to-009"},  {2, "MaskChannel-024-to-017"},  {3, "MaskChannel-032-to-025"},  {4, "MaskChannel-040-to-033"},  {5, "MaskChannel-048-to-041"},
        {6, "MaskChannel-056-to-049"},  {7, "MaskChannel-064-to-057"},  {8, "MaskChannel-072-to-065"},  {9, "MaskChannel-080-to-073"},  {10, "MaskChannel-088-to-081"}, {11, "MaskChannel-096-to-089"},
        {12, "MaskChannel-104-to-097"}, {13, "MaskChannel-112-to-105"}, {14, "MaskChannel-120-to-113"}, {15, "MaskChannel-128-to-121"}, {16, "MaskChannel-136-to-129"}, {17, "MaskChannel-144-to-137"},
        {18, "MaskChannel-152-to-145"}, {19, "MaskChannel-160-to-153"}, {20, "MaskChannel-168-to-161"}, {21, "MaskChannel-176-to-169"}, {22, "MaskChannel-184-to-177"}, {23, "MaskChannel-192-to-185"},
        {24, "MaskChannel-200-to-193"}, {25, "MaskChannel-208-to-201"}, {26, "MaskChannel-216-to-209"}, {27, "MaskChannel-224-to-217"}, {28, "MaskChannel-232-to-225"}, {29, "MaskChannel-240-to-233"},
        {30, "MaskChannel-248-to-241"}, {31, "MaskChannel-254-to-249"}};

    struct
    {
        bool operator()(std::pair<std::string, Ph2_HwDescription::ChipRegItem> a, std::pair<std::string, Ph2_HwDescription::ChipRegItem> b) const { return a.second.fPage > b.second.fPage; }
    } customPageDec;

    struct
    {
        bool operator()(std::pair<std::string, Ph2_HwDescription::ChipRegItem> a, std::pair<std::string, Ph2_HwDescription::ChipRegItem> b) const { return a.second.fPage < b.second.fPage; }
    } customPageInc;
    struct
    {
        bool operator()(std::pair<std::string, Ph2_HwDescription::ChipRegItem> a, std::pair<std::string, Ph2_HwDescription::ChipRegItem> b) const { return a.second.fAddress < b.second.fAddress; }
    } customAddressInc;
};
} // namespace Ph2_HwInterface

#endif
