/*!
        \file                                            ReadoutChipInterface.h
        \brief                                           User Interface to the Chip, base class for, CBC, MPA, SSA, RD53
        \author                                          Fabio RAVERA
        \version                                         1.0
        \date                        25/02/19
        Support :                    mail to : fabio.ravera@cern.ch
 */

#ifndef __READOUTCHIPINTERFACE_H__
#define __READOUTCHIPINTERFACE_H__

#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/ChipInterface.h"
#include <vector>

template <typename T>
class ChannelContainer;
/*!
 * \namespace Ph2_HwInterface
 * \brief Namespace regrouping all the interfaces to the hardware
 */
namespace Ph2_HwInterface
{
using BeBoardFWMap = std::map<uint16_t, BeBoardFWInterface*>; /*!< Map of Board connected */

/*!
 * \class ReadoutChipInterface
 * \brief Class representing the User Interface to the Chip on different boards
 */
class ReadoutChipInterface : public ChipInterface
{
  public:
    /*!
     * \brief Constructor of the ReadoutChipInterface Class
     * \param pBoardMap
     */
    ReadoutChipInterface(const BeBoardFWMap& pBoardMap);

    /*!
     * \brief Destructor of the ReadoutChipInterface Class
     */
    ~ReadoutChipInterface();

    /*!
     * \brief setChannels fo be injected
     * \param pChip: pointer to Chip object
     * \param group: group of channels under test
     * \param pVerify: perform a readback check
     */
    virtual bool setInjectionSchema(Ph2_HwDescription::ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool pVerify = true)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return false;
    }

    virtual bool enableInjection(Ph2_HwDescription::ReadoutChip* pChip, bool inject = true, bool pVerify = true)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return false;
    }

    virtual bool setInjectionAmplitude(Ph2_HwDescription::ReadoutChip* pChip, uint8_t injectionAmplitude, bool pVerify = true)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return false;
    }

    /*!
     * \brief Mask the channels not belonging to the group under test
     * \param pChip: pointer to Chip object
     * \param group: group of channels under test
     * \param pVerify: perform a readback check
     */
    virtual bool maskChannelGroup(Ph2_HwDescription::ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool pVerify = true)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return false;
    }

    /*!
     * \brief Read the ADC values of the ADC MUX
     * \param pChip: pointer to Chip object
     */
    virtual uint32_t readADC(Ph2_HwDescription::ReadoutChip* pChip, std::string theADCName, uint16_t numberOfRead)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    /*!
     * \brief Maeasure the ground value of the ADC
     * \param pChip: pointer to Chip object
     */
    virtual uint32_t readADCGround(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    /*!
     * \brief Read the ADC band gap value specifically of the ADC MUX
     * \param pChip: pointer to Chip object
     */
    virtual uint32_t readADCBandGap(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    /*!
     * \brief Read the ADC Vref value specifically of the ADC MUX
     * \param pChip: pointer to Chip object
     */
    virtual uint32_t readADCVref(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    /*!
     * \brief Read the Vref register value
     * \param pChip: pointer to Chip object
     */
    virtual uint32_t readVrefRegister(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    /*!
     * \brief Set the ADC Vref value to a desired value
     * \param pChip: pointer to Chip object
     * \param theVrefRegisterValue: the value to be set to Vref register
     */
    virtual bool setVref(Ph2_HwDescription::ReadoutChip* pChip, uint16_t theVrefRegisterValue)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    /*!
     * \brief Set the ADC Vref value to the value cointened in the Fuse ID from wafer testing
     * \param pChip: pointer to Chip object
     */
    virtual bool setVrefFromFuseID(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    /*!
     * \brief Calculate in V the precision of the ADC LSB
     * \param pChip: pointer to Chip object
     * \param theVrefValue: the value of Vref, needed for SSA2
     */
    virtual float calculateADCLSB(Ph2_HwDescription::ReadoutChip* pChip, float theVrefValue)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    /*!
     * \brief Get the table cointaing < bias register name, < default register value, expected value in volts>>
     * \param pChip: pointer to Chip object
     */
    virtual const std::map<std::string, std::pair<uint8_t, float>> getBiasStructureDefaultTable(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        std::map<std::string, std::pair<uint8_t, float>> theBiasStucture;
        theBiasStucture["empty"] = std::make_pair(255, -1);
        return theBiasStucture;
    }

    /*!
     * \brief Get the ADC band gap expected value. At the moment a default value is stored in the definition file
     * \param pChip: pointer to Chip object
     */
    virtual float getBandGapExpectedValue(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    /*!
     * \brief Get the ADC Vref expected value.
     * \param pChip: pointer to Chip object
     */
    virtual float getVrefExpectedValue(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    /*!
     * \brief Get the ADC precision. Currently estimated from 1 skeleton testing
     * \param pChip: pointer to Chip object
     */
    virtual float getVrefPrecision(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    /*!
     * \brief Get the Vref allowed minimum value.
     * \param pChip: pointer to Chip object
     */
    virtual float getVrefMinValue(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    /*!
     * \brief Get the Vref allowed maximum value.
     * \param pChip: pointer to Chip object
     */
    virtual float getVrefMaxValue(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    /*!
     * \brief mask and inject with one function to increase speed
     * \param pChip: pointer to Chip object
     * \param group: group of channels under test
     * \param mask: mask channel not belonging to the group under test
     * \param inject: inject channels belonging to the group under test
     * \param pVerify: perform a readback check
     */
    virtual bool maskChannelsAndSetInjectionSchema(Ph2_HwDescription::ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool mask, bool inject, bool pVerify = true) = 0;

    /*!
     * \brief Reapply the stored mask for the Chip, use it after group masking is applied
     * \param pChip: pointer to Chip object
     * \param pVerify: perform a readback check
     * \param pBlockSize: the number of registers to be written at once, default is 310
     */
    virtual bool ConfigureChipOriginalMask(Ph2_HwDescription::ReadoutChip* pChip, bool pVerify = true, uint32_t pBlockSize = 310) = 0;

    /*!
     * \brief Disable the test pad output
     * \param pChip: pointer to Chip object
     */
    virtual bool disableTestPadsOutput(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return false;
    }

    /// @brief Tune the ADC registers. One should first tune Vref using the BandGap as reference to tune it and then tune the different bias registers.
    /// @param theChip: pointer to Chip object
    /// @param theSlope: ADC slope
    /// @param theExpectedValue: Register expected value
    /// @param theDACtoTuneName: Register name
    /// @param theDACValue: the initial register value
    /// @param isVref: tell if the register being tuned is the reference one
    /// @return
    virtual uint8_t TuneDAC(Ph2_HwDescription::ReadoutChip* theChip, float theSlope, float theExpectedValue, std::string theDACtoTuneName, uint8_t theDACValue, bool isVref)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    // /*!
    //  * \brief Select the register to be read on the test pad output
    //  * \param pChip: pointer to Chip object
    //  */
    // virtual bool selectTestPadsOutput(Ph2_HwDescription::ReadoutChip* pChip, std::string theRegisterName)
    // {
    //     LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
    //     return false;
    // }

    /*!
     * \brief Write all Local registers on Chip and Chip Config File (able to recognize local parameter names)
     * \param pCbc
     * \param pRegNode : Node of the register to write
     * \param pValue : Value to write
     */
    virtual bool WriteChipAllLocalReg(Ph2_HwDescription::ReadoutChip* pChp, const std::string& dacName, const ChipContainer& pValue, bool pVerify = true) = 0;
    /*!
     * \brief Read all Local registers on Chip and Chip Config File (able to recognize local parameter names)
     * \param pCbc
     * \param pRegNode : Node of the register to write
     * \param pValue : Readout value
     */

    uint16_t getMostFrequentLocalRegisterValue(const ChipContainer& theChipContainer);

    virtual void ReadChipAllLocalReg(Ph2_HwDescription::ReadoutChip* pChip, const std::string& dacName, ChipContainer& pValue) {}

    /*!
     * \brief Mask all channels of the chip
     * \param pChip: pointer to Chip object
     * \param mask: if true mask, if false unmask
     * \param pVerify: perform a readback check
     * \param pBlockSize: the number of registers to be written at once, default is 310
     */
    virtual bool MaskAllChannels(Ph2_HwDescription::ReadoutChip* pChip, bool mask, bool pVerify = true) = 0;

    virtual void producePhaseAlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip, uint8_t pWait_ms = 10)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
    }

    virtual void produceWordAlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
    }

    virtual void produceBX0AlignmentPattern(Ph2_HwDescription::ReadoutChip* pChip)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
    }

    virtual std::vector<uint8_t> getWordAlignmentPatterns()
    {
        std::vector<uint8_t> cAlignmentPattern{10, 0};
        return cAlignmentPattern;
    }

    virtual std::vector<uint8_t> getBX0AlignmentPatterns()
    {
        std::vector<uint8_t> cAlignmentPattern{10, 0};
        return cAlignmentPattern;
    }

    virtual void DumpChipRegisters(Ph2_HwDescription::ReadoutChip* pChip) { LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET; }

    /*!
     * \brief Monitoring memeber functions
     */
    virtual float ReadHybridTemperature(Ph2_HwDescription::ReadoutChip* pChip, bool silentRunning = false)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    virtual float ReadHybridVoltage(Ph2_HwDescription::ReadoutChip* pChip, bool silentRunning = false)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }

    virtual float ReadChipMonitor(Ph2_HwDescription::ReadoutChip* pChip, const std::string& observableName, bool silentRunning = false)
    {
        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
        return 0;
    }
};
} // namespace Ph2_HwInterface

#endif
