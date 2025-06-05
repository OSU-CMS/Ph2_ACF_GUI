/*!
  \file                   Chip.h
  \brief                  Chip Description class, config of the Chips
  \author                 Lorenzo BIDEGAIN
  \version                1.0
  \date                   25/06/14
  Support :               mail to : lorenzo.bidegain@gmail.com
*/

#ifndef Chip_H
#define Chip_H

#include "ChipRegItem.h"
#include "FrontEndDescription.h"
#include "Utils/ConsoleColor.h"
#include "Utils/Container.h"
#include "Utils/Exception.h"
#include "Utils/Visitor.h"
#include "Utils/easylogging++.h"

#include <iostream>
#include <regex>
#include <set>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <utility>

class ChannelGroupBase;

/*!
 * \namespace Ph2_HwDescription
 * \brief Namespace regrouping all the hardware description
 */

namespace Ph2_HwDescription
{
struct ChipRegMask
{
    uint8_t fBitShift;
    uint8_t fNbits;
};
class ChipFuseID // I think this makes sennse in chip?
{
    uint32_t fVal;

  public:
    uint8_t Pos() { return (fVal & 0xFF); }
    uint8_t Wafer() { return (fVal >> 8) & 0x1F; }
    uint8_t Lot() { return (fVal >> 13) & 0x7F; }
    uint8_t Status() { return (fVal >> 20) & 0x3; }
    uint8_t Process() { return (fVal >> 22) & 0x1F; }
    uint8_t ADCRef() { return (fVal >> 27) & 0x1F; }
    void    SetId(uint32_t fID) { fVal = fID; };
};

using ChipRegMap  = std::unordered_map<std::string, ChipRegItem>;
using ChipRegPair = std::pair<std::string, ChipRegItem>;
using CommentMap  = std::map<int, std::string>;

/*!
 * \class Chip
 * \brief Read/Write Chip's registers on a file, contains a register map
 */
class Chip : public FrontEndDescription
{
  public:
    // C'tors which take Board ID, Frontend ID/Hybrid ID, FMC ID, Chip ID
    Chip(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, uint8_t pChipId, uint16_t pMaxRegValue = 255);

    // C'tors with object FE Description
    Chip(const FrontEndDescription& pFeDesc, uint8_t pChipId, uint16_t pMaxRegValue = 255);

    // Default C'tor
    Chip();

    // Copy C'tor
    Chip(const Chip&) = delete;

    Ph2_HwDescription::ChipFuseID pChipFuseID;

    // D'Tor
    virtual ~Chip();

    /*!
     * \brief acceptor method for HwDescriptionVisitor
     * \param pVisitor
     */
    virtual void accept(HwDescriptionVisitor& pVisitor) { pVisitor.visitChip(*this); }

    /*!
     * \brief Load RegMap from a file
     * \param filename
     */
    virtual void loadfRegMap(const std::string& filename) = 0;

    /*!
     * \brief Get any register from the Map
     * \param pReg
     * \return The value of the register
     */
    uint16_t getReg(const std::string& pReg) const;

    /*!
     * \brief Set any register of the Map
     * \param pReg
     * \param psetValue
     */
    virtual void setReg(const std::string& pReg, uint16_t psetValue, uint8_t pStatusReg = 0);

    /*!
     * \brief Get any registeritem of the Map
     * \param pReg
     * \return RegItem
     */
    const ChipRegItem& getRegItem(const std::string& pReg) const;
    ChipRegItem&       getRegItem(const std::string& pReg);

    /*!
     * \brief Write the registers of the Map in a file
     * \param fileName
     */
    void saveRegMap(const std::string& fileName);

    /*!
     * \brief Prepare a stream with the registers of the Map
     * \return std::stringstream
     */
    virtual std::stringstream getRegMapStream() = 0;

    /*!
     * \brief Get the Map of the registers
     * \return The map of register
     */
    ChipRegMap& getRegMap() { return fRegMap; }

    const ChipRegMap& getRegMap() const { return fRegMap; }

    void appendToRegMap(std::string pRegName, ChipRegItem pItem) { fRegMap[pRegName] = pItem; }

    /*!
     * \brief Get the Chip Id
     * \return The Chip ID
     */
    virtual uint16_t getId() const { return fChipId; }

    /*!
     * \brief Get the Chip address
     * \return The Chip address
     */
    uint8_t getChipAddress() const { return fChipAddress; }

    /*!
     * \brief Set the Chip address
     */
    void setChipAddress(uint16_t pChipAddress) { fChipAddress = pChipAddress; }

    /*!
     * \brief Get the Chip code
     * \return The Chip code
     */
    uint8_t getChipCode() const { return fChipCode; }

    /*!
     * \brief Set the I2C Master Id corresponding to the Chip
     * \param The I2C Master Id
     */
    void setMasterId(uint8_t pMasterId) { fMasterId = pMasterId; };

    /*!
     * \brief Get the I2C Master Id corresponding to the Chip
     * \return The I2C Master Id
     */
    uint8_t getMasterId() const { return fMasterId; };

    virtual uint8_t getNumberOfBits(const std::string& dacName) = 0;
    void            printChipType(std::ostream& os) const { os << "FrontEndType\t--> " << FrontEndDescription::getFrontEndName(fType); }

    // Set some of the bits in register , leave others untouched
    void setRegBits(const std::string& pReg, ChipRegMask pMask, uint16_t pValue)
    {
        uint16_t cMask = 0x00;
        for(uint8_t cIndx = 0; cIndx < pMask.fNbits; cIndx++) cMask = cMask | (1 << cIndx);
        uint16_t cRegMask = (cMask << pMask.fBitShift);
        cRegMask          = ~(cRegMask);
        setReg(pReg, (getReg(pReg) & cRegMask) | (pValue << pMask.fBitShift));
    }
    // retrieve some bits of register
    uint16_t getRegBits(const std::string& pReg, ChipRegMask pMask)
    {
        uint16_t cMask = 0x0000;
        for(uint8_t cIndx = 0; cIndx < pMask.fNbits; cIndx++) cMask = cMask | (1 << cIndx);
        uint16_t cRegMask = (cMask << pMask.fBitShift);
        uint16_t cValue   = (getReg(pReg) & cRegMask) >> pMask.fBitShift;
        // std::cout << "\t\t\t Value is 0x" << std::hex << getReg(pReg) <<  std::dec << " Mask is 0x" << std::hex << cRegMask << std::dec << " value is " << +cValue << "\n";
        return cValue;
    }

    // Update write count
    void     updateWriteCount(uint32_t fIncrement = 1) { fI2CWrites += fIncrement; }
    void     updateReadCount(uint32_t fIncrement = 1) { fI2Reads += fIncrement; }
    void     updateRBMismatchCount(uint32_t fIncrement = 1) { fI2CReadMismatches += fIncrement; }
    void     updateRegWriteCount(uint32_t fIncrement = 1) { fRegWrites += fIncrement; }
    void     updateRegReadCount(uint32_t fIncrement = 1) { fRegReads += fIncrement; }
    uint32_t getWriteCount() { return fI2CWrites; }
    uint32_t getReadCount() { return fI2Reads; }
    uint32_t getRBMismatchCount() { return fI2CReadMismatches; }
    uint32_t getRegWriteCount() { return fRegWrites; }
    uint32_t getRegReadCount() { return fRegReads; }

    std::string getFileName(const std::string& fName2Add = "") const
    {
        std::string output = this->fConfigFileName;
        output.insert(output.find_last_of("/\\") + 1, fName2Add);
        return output;
    }

    enum class RegisterType
    {
        ReadOnly,
        Utility,
        User
    };

    void                                             takeSnapshot();
    void                                             clearSnapshot();
    std::vector<std::pair<std::string, uint16_t>>    getSnapshot() const;
    void                                             reinitializeFreeRegisters();
    void                                             addFreeRegister(const std::regex& theRegisterName);
    std::vector<std::pair<std::regex, RegisterType>> getFreeRegisters() const { return fListOfFreeRegisters; }
    std::vector<std::string>                         getReadOnlyRegisterList() const;

  protected:
    virtual void                                     initializeFreeRegisters() {};
    std::string                                      fConfigFileName;
    uint8_t                                          fChipCode;
    uint8_t                                          fChipId;
    uint8_t                                          fChipAddress; // I2C addess of chip
    uint16_t                                         fMaxRegValue;
    uint8_t                                          fMasterId;
    ChipRegMap                                       fRegMap;
    CommentMap                                       fCommentMap;
    std::vector<std::pair<std::regex, RegisterType>> fListOfFreeRegisters{};

  private:
    uint32_t                                  fI2CWrites         = 0;
    uint32_t                                  fI2Reads           = 0;
    uint32_t                                  fI2CReadMismatches = 0;
    uint32_t                                  fRegWrites         = 0;
    uint32_t                                  fRegReads          = 0;
    bool                                      fTrackModifiedRegistersEnabled{false};
    std::unordered_map<std::string, uint16_t> fModifiedRegisters{};
};

/*!
 * \struct ChipComparer
 * \brief Compare two Chip by their ID
 */
struct ChipComparer
{
    bool operator()(const Chip& cbc1, const Chip& cbc2) const;
};

/*!
 * \struct RegItemComparer
 * \brief Compare two pair of Register Name Versus ChipRegItem by the Page and Adress of the ChipRegItem
 */
struct RegItemComparer
{
    bool operator()(const ChipRegPair& pRegItem1, const ChipRegPair& pRegItem2) const;
};

} // namespace Ph2_HwDescription

#endif
