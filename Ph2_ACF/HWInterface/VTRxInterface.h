/*!

        \file                                            VTRxInterface.h
        \brief                                           User Interface to the VTRx
        \version                                         1.0

 */

#ifndef __VTRXINTERFACE_H__
#define __VTRXINTERFACE_H__
#include "HWInterface/ChipInterface.h"

namespace Ph2_HwInterface
{
/*!
 * \class CicInterface
 * \brief Class representing the User Interface to the Cic on different boards
 */

class lpGBTInterface;

class VTRxInterface : public ChipInterface
{
  public:
    /*!
     * \brief Constructor of the VTRxInterface Class
     * \param pBoardMap
     */
    VTRxInterface(const BeBoardFWMap& pBoardMap, lpGBTInterface* theLpGBTInterface);
    /*!
     * \brief Destructor of the VTRxInterface Class
     */
    ~VTRxInterface();

    /*!
     * \brief Configure the Cic with the Cic Config File
     * \param pChip: pointer to VTRx object
     * \param pVerify: perform a readback check
     * \param pBlockSize: the number of registers to be written at once, default is 310
     */
    bool ConfigureChip(Ph2_HwDescription::Chip* pChip, bool pVerify = true, uint32_t pBlockSize = 310) override;

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

    uint32_t ReadChipFuseID(Ph2_HwDescription::Chip* pChip, uint8_t version = 1) override;

  private:
    lpGBTInterface* fTheLpGBTinterface;
};
} // namespace Ph2_HwInterface

#endif
