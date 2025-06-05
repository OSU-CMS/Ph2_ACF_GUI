/*!
  \file                  ChipInterface.h
  \brief                 User Interface to the Chip, base class for, CBC, MPA, SSA, RD53
  \author                Fabio RAVERA
  \version               1.0
  \date                  25/02/19
  Support:               email to fabio.ravera@cern.ch
*/

#ifndef __CHIPINTERFACE_H__
#define __CHIPINTERFACE_H__

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace Ph2_HwDescription
{
class BeBoard;
class Chip;
class Hybrid;
} // namespace Ph2_HwDescription

template <typename T>
class ChannelContainer;

/*!
 * \namespace Ph2_HwInterface
 * \brief Namespace regrouping all the interfaces to the hardware
 */
namespace Ph2_HwInterface
{
class BeBoardFWInterface;
using BeBoardFWMap = std::map<uint16_t, BeBoardFWInterface*>; /*!< Map of Board connected */

/*!
 * \class ChipInterface
 * \brief Class representing the User Interface to the Chip on different boards
 */
class ChipInterface
{
  protected:
    BeBoardFWMap        fBoardMap;            /*!< Map of Board connected */
    BeBoardFWInterface* fBoardFW;             /*!< Board loaded */
    uint16_t            fPrevBoardIdentifier; /*!< Id of the previous board */
    bool                fWithlpGBT = false;   /*!< lpGBT is used for configuration */

    /*!
     * \brief Set the board to talk with
     * \param pBoardId
     */
    void setBoard(uint16_t pBoardIdentifier);

  public:
    /*!
     * \brief Constructor of the ChipInterface Class
     * \param pBoardMap
     */
    ChipInterface(const BeBoardFWMap& pBoardMap);

    /*!
     * \brief Destructor of the ChipInterface Class
     */
    virtual ~ChipInterface() {}

    /*!
     * \brief Configure the Chip with the Chip Config File
     * \param pChip: pointer to Chip object
     * \param pVerify: perform a readback check
     * \param pBlockSize: the number of registers to be written at once, default is 310
     */
    virtual bool ConfigureChip(Ph2_HwDescription::Chip* pChip, bool pVerify = true, uint32_t pBlockSize = 310) = 0;

    /*!
     * \brief Write the designated register in both Chip and Chip Config File
     * \param pChip
     * \param pRegNode : Node of the register to write
     * \param pValue : Value to write
     */
    virtual bool WriteChipReg(Ph2_HwDescription::Chip* pChip, const std::string& pRegNode, uint16_t pValue, bool pVerify = true) = 0;
    virtual void WriteHybridBroadcastChipReg(const Ph2_HwDescription::Hybrid* pHybrid, const std::string& pRegNode, uint16_t data);
    virtual void WriteBoardBroadcastChipReg(const Ph2_HwDescription::BeBoard* pBoard, const std::string& pRegNode, uint16_t data);

    /*!
     * \brief Write several registers in both Chip and Chip Config File
     * \param pChip
     * \param pVecReq : Vector of pair: Node of the register to write versus value to write
     */
    virtual bool     WriteChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::pair<std::string, uint16_t>>& pVecReq, bool pVerify = true);
    virtual uint32_t ReadChipFuseID(Ph2_HwDescription::Chip* pChip, uint8_t version = 1);

    /*!
     * \brief Read the designated register in the Chip
     * \param pChip
     * \param pRegNode : Node of the register to read
     */
    virtual int32_t ReadChipReg(Ph2_HwDescription::Chip* pChip, const std::string& pRegNode) = 0;

    virtual std::vector<std::pair<std::string, uint16_t>> ReadChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& theRegisterList);

    // this does not need to be virtual as its the same for all types of readout chips
    bool lpGBTCheck(const Ph2_HwDescription::BeBoard* pBoard);
    bool lpGBTFound() { return fWithlpGBT; }
    void setWithLpGBT(bool pValue) { fWithlpGBT = pValue; }

    void output();
};

} // namespace Ph2_HwInterface

#endif
