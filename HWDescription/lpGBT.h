/*!
  \file                  lpGBT.h
  \brief                 lpGBT description class, config of the lpGBT
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef lpGBT_H
#define lpGBT_H

#include "Chip.h"
#include "Utils/ConsoleColor.h"
#include "Utils/Utilities.h"
#include "Utils/easylogging++.h"

#include <iomanip>

namespace Ph2_HwDescription
{
class lpGBT : public Chip
{
  public:
    struct eportProperty
    {
        uint8_t Group;
        uint8_t Channel;
        uint8_t Polarity;
        bool    isPhaseAligned;
    };

    lpGBT(uint8_t pBeBoardId, uint8_t FMCId, uint8_t pOpticalGroupId, uint8_t pChipId, const std::string& fileName, const std::string& configFilePath);

    lpGBT(const lpGBT&) = delete;

    void initializeFreeRegisters() override;

    void loadfRegMap(const std::string& fileName) override;

    std::stringstream getRegMapStream() override;
    uint8_t           getNumberOfBits(const std::string& dacName) override { return 0; }

    void    setVersion(uint8_t pVersion) { fVersion = pVersion; }
    uint8_t getVersion() const { return fVersion; }
    void    setInvertClock(uint8_t hybridId, bool pInvertClock);

    void setPhaseRxAligned(uint8_t RxGroup, bool value)
    {
        for(auto& RxProperty: fRxProperties)
            if(RxProperty.Group == RxGroup) RxProperty.isPhaseAligned = value;
    }
    bool getPhaseRxAligned(uint8_t RxGroup)
    {
        for(const auto& RxProperty: fRxProperties)
            if(RxProperty.Group == RxGroup) return RxProperty.isPhaseAligned;
        return false;
    };

    void setRxHSLPolarity(uint8_t pRxHSLPolarity) { fRxHSLPolarity = pRxHSLPolarity; }
    void setTxHSLPolarity(uint8_t pTxHSLPolarity) { fTxHSLPolarity = pTxHSLPolarity; }

    void addRxProperty(uint8_t pRxGroup, uint8_t pRxChannel, uint8_t pRxPolarity) { fRxProperties.push_back({pRxGroup, pRxChannel, pRxPolarity, false}); };
    void setRxDataRate(uint16_t pRxDataRate) { fRxDataRate = pRxDataRate; }

    void addTxProperty(uint8_t pTxGroup, uint8_t pTxChannel, uint8_t pTxPolarity) { fTxProperties.push_back({pTxGroup, pTxChannel, pTxPolarity, true}); };
    void setTxDataRate(uint16_t pTxDataRate) { fTxDataRate = pTxDataRate; }

    std::vector<uint8_t> getRxGroups()
    {
        std::vector<uint8_t> RxGroups;
        for(auto RxProperty: fRxProperties) RxGroups.push_back(RxProperty.Group);
        return RxGroups;
    }
    std::vector<eportProperty> getRxProperties() { return fRxProperties; }
    uint16_t                   getRxDataRate() { return fRxDataRate; }

    std::vector<eportProperty> getTxProperties() { return fTxProperties; }
    uint16_t                   getTxDataRate() { return fTxDataRate; }

    uint8_t getRxHSLPolarity() { return fRxHSLPolarity; }
    uint8_t getTxHSLPolarity() { return fTxHSLPolarity; }

    void     updateWriteCount(uint8_t pMasterId, uint32_t fIncrement = 1) { fI2CWrites[pMasterId] += fIncrement; }
    void     updateReadCount(uint8_t pMasterId, uint32_t fIncrement = 1) { fI2CReads[pMasterId] += fIncrement; }
    uint32_t getWriteCount(uint8_t pMasterId) { return fI2CWrites[pMasterId]; }
    uint32_t getReadCount(uint8_t pMasterId) { return fI2CReads[pMasterId]; }

    void                    setTuneVrefADC(std::string pTuneVrefADC) { fTuneVrefADC = pTuneVrefADC; }
    void                    setTuneVrefVoltage(uint16_t pTuneVrefVoltage) { fTuneVrefVoltage = pTuneVrefVoltage; }
    std::string             getTuneVrefADC() { return fTuneVrefADC; }
    uint16_t                getTuneVrefVoltage() { return fTuneVrefVoltage; }
    std::pair<float, float> getTemperatureCoefficients() { return fTemperatureCoefficients; }

    void setIsCalibrationDataLoaded(bool isLoaded) { fIsCalibrationDataLoaded = isLoaded; }
    bool getIsCalibrationDataLoaded() const { return fIsCalibrationDataLoaded; }

    std::map<std::string, float> getADCCalibrationData() const { return fADCcalibrationData; }
    void                         setADCCalibrationData(const std::map<std::string, float>& theInputMap);

    // ################################################################################
    // # Set the junction temperature. It should be updated by the user based on:     #
    // # - thermal simulations and measurements performed in the final system, or     #
    // # - data of its internal temperature sensor obtained during production testing #
    // #   (estimate_temperature_uncalib_vref)                                        #
    // ################################################################################
    void  setTemperature(float cTemperature) { fTemperature = cTemperature; }
    float getTemperature() { return fTemperature; }

    // ################################################################################
    // # Store the last value of the NTC measurement connected to the lpGBT           #
    // # - Value serves as estimation of next measurement, its 1000 if not set        #
    // ################################################################################
    float getNTCResistance() { return fNTCResistance; }
    void  setNTCResistance(float cNTCResistance);

    float getVtrxNTCResistance() { return fVtrxNTCResistance; }
    void  setVtrxNTCResistance(float cVtrxNTCResistance);

    std::string getConfigFilePath() const { return fConfigFilePath; }

  private:
    uint16_t                   fRxDataRate, fTxDataRate;
    uint8_t                    fVersion, fRxHSLPolarity, fTxHSLPolarity;
    std::vector<uint8_t>       fRxGroups;
    std::vector<eportProperty> fRxProperties, fTxProperties;
    std::string                fConfigFilePath;

    // #########################################################
    // # Number of write transactions - one element per master #
    // #########################################################
    std::vector<uint32_t> fI2CWrites{0, 0, 0};

    // ########################################################
    // # Number of read transactions - one element per master #
    // ########################################################
    std::vector<uint32_t> fI2CReads{0, 0, 0};

    // #######################################################
    // # ADC Channel and Voltage to manually tune Vref to 1V #
    // #######################################################
    std::string             fTuneVrefADC{"ADC2"};                                    // ADC2 = LV monitor line of 2S module, ADC7 for PS modules (2.55V line monitor)
    uint16_t                fTuneVrefVoltage{500};                                   // Voltage is given in mV!
    std::pair<float, float> fTemperatureCoefficients{std::make_pair(0.0021, 0.475)}; // In V per Celsius and Volt coming from the lpGBTv0 manual

    bool  fIsCalibrationDataLoaded{false};
    float fTemperature          = 0.0;
    float fNTCResistance        = 1000; // in Ohms, default value at 25°C
    float fMaxNTCResistance     = 40000;
    float fMinNTCResistance     = 50;
    float fVtrxNTCResistance    = 1000; // in Ohms, default value at 25°C
    float fMaxVtrxNTCResistance = 30000;
    float fMinVtrxNTCResistance = 80;

    // Default values, will be overwritten once the calibration is loaded
    std::map<std::string, float> fADCcalibrationData = {
        {"VREF_SLOPE", -3.3638e-01},
        {"VREF_OFFSET", 1.3426e+02},
        {"VDAC_SLOPE", 4.0906e+03},
        {"VDAC_OFFSET", 2.5420e-01},
        {"VDAC_SLOPE_TEMP", 1.0492e-01},
        {"VDAC_OFFSET_TEMP", -3.8617e-02},
        {"ADC_X2_SLOPE", 1.0429e-03},
        {"ADC_X2_OFFSET", -3.3531e-02},
        {"ADC_X2_SLOPE_TEMP", -2.9308e-09},
        {"ADC_X2_OFFSET_TEMP", 1.2004e-06},
        {"ADC_X8_SLOPE", 3.0928e-04},
        {"ADC_X8_OFFSET", 3.4134e-01},
        {"ADC_X8_SLOPE_TEMP", 6.6980e-10},
        {"ADC_X8_OFFSET_TEMP", -7.6619e-07},
        {"ADC_X16_SLOPE", 1.4331e-04},
        {"ADC_X16_OFFSET", 4.2615e-01},
        {"ADC_X16_SLOPE_TEMP", 1.5782e-09},
        {"ADC_X16_OFFSET_TEMP", -1.3777e-06},
        {"VDDMON_SLOPE", 2.3257e+00},
        {"VDDMON_SLOPE_TEMP", 6.5729e-05},
        {"CDAC0_SLOPE", 2.7212e+05},
        {"CDAC0_OFFSET", -1.3486e-02},
        {"CDAC0_R0", 2.5227e+06},
        {"CDAC0_SLOPE_TEMP", -1.5463e+02},
        {"CDAC0_OFFSET_TEMP", -4.8661e-04},
        {"CDAC0_R0_TEMP", 9.5716e+03},
        {"CDAC1_SLOPE", 2.7223e+05},
        {"CDAC1_OFFSET", -1.3486e-02},
        {"CDAC1_R0", 2.5227e+06},
        {"CDAC1_SLOPE_TEMP", -1.5643e+02},
        {"CDAC1_OFFSET_TEMP", -4.8661e-04},
        {"CDAC1_R0_TEMP", 9.5716e+03},
        {"CDAC2_SLOPE", 2.7227e+05},
        {"CDAC2_OFFSET", -1.3486e-02},
        {"CDAC2_R0", 2.5227e+06},
        {"CDAC2_SLOPE_TEMP", -1.5756e+02},
        {"CDAC2_OFFSET_TEMP", -4.8661e-04},
        {"CDAC2_R0_TEMP", 9.5716e+03},
        {"CDAC3_SLOPE", 2.7193e+05},
        {"CDAC3_OFFSET", -1.3486e-02},
        {"CDAC3_R0", 2.5227e+06},
        {"CDAC3_SLOPE_TEMP", -1.5537e+02},
        {"CDAC3_OFFSET_TEMP", -4.8661e-04},
        {"CDAC3_R0_TEMP", 9.5716e+03},
        {"CDAC4_SLOPE", 2.7277e+05},
        {"CDAC4_OFFSET", -1.3486e-02},
        {"CDAC4_R0", 2.5227e+06},
        {"CDAC4_SLOPE_TEMP", -1.6278e+02},
        {"CDAC4_OFFSET_TEMP", -4.8661e-04},
        {"CDAC4_R0_TEMP", 9.5716e+03},
        {"CDAC5_SLOPE", 2.7295e+05},
        {"CDAC5_OFFSET", -1.3486e-02},
        {"CDAC5_R0", 2.5227e+06},
        {"CDAC5_SLOPE_TEMP", -1.6169e+02},
        {"CDAC5_OFFSET_TEMP", -4.8661e-04},
        {"CDAC5_R0_TEMP", 9.5716e+03},
        {"CDAC6_SLOPE", 2.7342e+05},
        {"CDAC6_OFFSET", -1.3486e-02},
        {"CDAC6_R0", 2.5227e+06},
        {"CDAC6_SLOPE_TEMP", -1.6537e+02},
        {"CDAC6_OFFSET_TEMP", -4.8661e-04},
        {"CDAC6_R0_TEMP", 9.5716e+03},
        {"CDAC7_SLOPE", 2.7328e+05},
        {"CDAC7_OFFSET", -1.3486e-02},
        {"CDAC7_R0", 2.5227e+06},
        {"CDAC7_SLOPE_TEMP", -1.6303e+02},
        {"CDAC7_OFFSET_TEMP", -4.8661e-04},
        {"CDAC7_R0_TEMP", 9.5716e+03},
        {"TEMPERATURE_SLOPE", 4.1320e+02},
        {"TEMPERATURE_OFFSET", -1.8545e+02},
        {"TEMPERATURE_UNCALVREF_SLOPE", 4.5960e-01},
        {"TEMPERATURE_UNCALVREF_OFFSET", -2.1253e+02},
    };
};
} // namespace Ph2_HwDescription

#endif
