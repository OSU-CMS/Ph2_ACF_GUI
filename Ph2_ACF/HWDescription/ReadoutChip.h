/*!
  \file                   Chip.h
  \brief                  Chip Description class, config of the Chips
  \author                 Lorenzo BIDEGAIN
  \version                1.0
  \date                   25/06/14
  Support :               mail to : lorenzo.bidegain@gmail.com
*/

#ifndef ReadoutChip_H
#define ReadoutChip_H

#include "Chip.h"
#include "Definition.h"
#include "Utils/ChannelGroupHandler.h"
#include "Utils/Container.h"
#include "Utils/Exception.h"
#include "Utils/Visitor.h"
#include "Utils/easylogging++.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdint.h>
#include <string>
#include <utility>

/*!
 * \namespace Ph2_HwDescription
 * \brief Namespace regrouping all the hardware description
 */
namespace Ph2_HwDescription
{
/*!
 * \class ReadoutChip
 * \brief Read/Write Chip's registers on a file, contains a register map
 */
class ReadoutChip
    : public Chip
    , public ChipContainer
{
  public:
    // C'tors which take Board ID, Frontend ID/Hybrid ID, FMC ID, Chip ID
    ReadoutChip(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, uint8_t pChipId, uint16_t pMaxRegValue = 255);

    // C'tors with object FE Description
    ReadoutChip(const FrontEndDescription& pFeDesc, uint8_t pChipId, uint16_t pMaxRegValue = 255);

    // Copy C'tor
    ReadoutChip(const ReadoutChip&) = delete;

    ~ReadoutChip();

    uint16_t getId() const override { return ChipContainer::getId(); }

    /*!
     * \brief acceptor method for HwDescriptionVisitor
     * \param pVisitor
     */
    virtual void accept(HwDescriptionVisitor& pVisitor) { pVisitor.visitChip(*this); }

    virtual uint32_t getNumberOfChannels() const = 0;

    virtual bool isDACLocal(const std::string& dacName) = 0;

    const std::shared_ptr<ChannelGroupBase> getChipOriginalMask() const override { return fChipOriginalMask; }

    // Channels are encoded as columnNumber*Nrows + rowNumber
    void setChipOriginalMask(std::vector<uint16_t> pChannels)
    {
        fChipOriginalMask->enableAllChannels();
        for(auto cChnl: pChannels)
        {
            uint16_t cRow = cChnl;
            uint16_t cCol = 0;
            if(fChipOriginalMask->getNumberOfCols() > 0)
            {
                cRow = cChnl % fChipOriginalMask->getNumberOfRows();
                cCol = cChnl / fChipOriginalMask->getNumberOfRows();
                // std::cout << cRow << " , " << cCol << "\n";
            }
            fChipOriginalMask->disableChannel(cRow, cCol);
        }
        // std::cout << "Channel mask has " << fChipOriginalMask->getNumberOfEnabledChannels() << " enabled channels\n";
    }

    // Functions to convert from local to gloabal coordinates
    virtual bool isTopSensor(Ph2_HwDescription::ReadoutChip* pChip, uint16_t pLocalColumn = 0)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    virtual std::pair<uint16_t, uint16_t> getGlobalCoordinates(Ph2_HwDescription::ReadoutChip* pChip, uint16_t pLocalColumn, uint16_t pLocalRow)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return std::make_pair(65535, 65535);
    }

    /*!
     * \brief Set chip average noise
     * \param theNoise
     */
    void setAverageNoise(float theNoise) { fAverageNoise = theNoise; }
    /*!
     * \brief Get chip average noise
     * \return theNoise
     */
    float getAverageNoise() const { return fAverageNoise; }

    /*!
     * \brief Set chip average pedestal
     * \param thePedestal
     */
    void setAveragePedestal(float thePedestal) { fAveragePedestal = thePedestal; }
    /*!
     * \brief Get chip average pedestal
     * \return thePedestal
     */
    float getAveragePedestal() const { return fAveragePedestal; }

    virtual void setADCCalibrationValue(const std::string& theCalibrationName, float theCalibrationValue)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
    }

    virtual float getADCCalibrationValue(const std::string& theCalibrationName) const
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 999.;
    }

  protected:
    std::shared_ptr<ChannelGroupBase> fChipOriginalMask{nullptr};
    float                             fAverageNoise{-1.};
    float                             fAveragePedestal{-1.};
    std::map<std::string, float>      fADCcalibrationMap = {
        {"ADC_SLOPE", 0.0002},    // In volts, calculated as the target Vref (0.850 V) / ADC range (4095)
        {"ADC_OFFSET", 0.},       // In volts, assumed 0. It depends on the ground value
        {"TEMP_SLOPE", -0.00103}, // mV / C
        {"TEMP_OFFSET", 0.154}    // Voltage in mV at 25C
    };
};
} // namespace Ph2_HwDescription

#endif
