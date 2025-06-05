/*!
  \file                  TEPXQuadNTC.cc
  \brief                 Read out TEPX Quad NTC
  \author                PSI
  \version               1.0
  \date                  04/03/24
  Support:               none
*/

#include "TEPXQuadNTC.h"
#include "Utils/ContainerSerialization.h"
#include <vector>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

// define function for Linear Regression with least mean squares method
std::pair<double, double> LinReg(const std::vector<double>& x, const std::vector<double>& y, bool verbose = false)
{
    int    n = x.size();
    int    i;
    double x_sum = 0, x2_sum = 0, y_sum = 0, xy_sum = 0;

    // iterating through x and y
    for(i = 0; i < n; i++)
    {
        x_sum += x[i];
        y_sum += y[i];
        x2_sum += pow(x[i], 2);
        xy_sum += x[i] * y[i];
    }

    double slope, intercept;
    slope     = (n * xy_sum - x_sum * y_sum) / (n * x2_sum - x_sum * x_sum);
    intercept = (x2_sum * y_sum - x_sum * xy_sum) / (x2_sum * n - x_sum * x_sum);
    return std::make_pair(slope, intercept);
}

float ntc_funct(double voltage, double current)
{
    const float T0C         = 273.15; // [Kelvin]
    const float T25C        = 298.15; // [Kelvin]
    const float R25C        = 10;     // [kOhm]
    const int   beta        = 3435;
    float       resistance  = 1e3 * voltage / current;                                // [kOhm]
    float       temperature = 1. / (1. / T25C + log(resistance / R25C) / beta) - T0C; // [Celsius]
    return temperature;
}

void TEPXQuadNTC::ConfigureCalibration() { LOG(INFO) << GREEN << "[TEPXQuadNTC::ConfigureCalibration]" << RESET; }

void TEPXQuadNTC::Running()
{
    theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[TEPXQuadNTC::Running] Starting run: " << BOLDYELLOW << theCurrentRun << RESET;

    TEPXQuadNTC::run();
    // TEPXQuadNTC::analyze();
    // TEPXQuadNTC::sendData();
}

void TEPXQuadNTC::sendData() {}

void TEPXQuadNTC::Stop()
{
    LOG(INFO) << GREEN << "[TEPXQuadNTC::Stop] Stopping" << RESET;

    Tool::Stop();

    TEPXQuadNTC::draw();
    // this->closeFileHandler();

    RD53RunProgress::reset();
}

void TEPXQuadNTC::localConfigure(const std::string& histoFileName, int currentRun) { LOG(INFO) << GREEN << "[TEPXQuadNTC::localConfigure] Starting run: " << BOLDYELLOW << theCurrentRun << RESET; }

float TEPXQuadNTC::TfromR(float RkOhm, const float& R25C, const float& beta)
{
    const float T0C  = 273.15; // [Kelvin]
    const float T25C = 298.15; // [Kelvin]

    if((abs(R25C - 10.) < 0.001) && (abs(beta - 3380.) < 0.1))
    {
        // Murata NTHCG83, use fit to vendor data
        double y  = RkOhm - R25C;
        float  RB = 10.0 + 1.01884 * y + 0.00238757 * pow(y, 2) - 1.5062e-05 * pow(y, 3) + 5.02968e-08 * pow(y, 4);
        return 1. / (1. / T25C + log(RB / R25C) / beta) - T0C; // [Celsius]
    }
    else
    {
        return 1. / (1. / T25C + log(RkOhm / R25C) / beta) - T0C; // [Celsius]
    }
}

void TEPXQuadNTC::run()
{
    fVerbose           = true;
    auto chipInterface = static_cast<RD53Interface*>(this->fReadoutChipInterface);

    CalibBase::prepareChipQueryForEnDis("chipSubset"); //  ??
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
            {
                std::map<int, double> ntc_temp_from_slope_map;
                for(const auto cChip: *cHybrid)
                {
                    float R25NTC = cChip->getRegItem("RNTCAT25C").fValue / 1000.;
                    float beta   = cChip->getRegItem("NTCBETA").fValue;
                    LOG(INFO) << GREEN << "[TEPXQuadNTC::run]  board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/"
                              << +cChip->getId() << "   R25NTC=" << R25NTC << "   NTCBETA=" << beta << RESET;
                    if(!((abs(R25NTC - 10.) < 0.001) && (abs(beta - 3380.) < 0.1)) && !((abs(R25NTC - 1.5) < 0.001) && (abs(beta - 3500.) < 0.1)))
                    {
                        LOG(WARNING) << RED << "[TEPXQuadNTC::run]  NTC parameters, "
                                     << "   R25NTC=" << R25NTC << "   NTCBETA=" << beta << ",  do not correspond to a known NTC type." << RESET;
                    }
                    // raw ADC: for a list of "observables" see RD53BInterface::getADCobservable  in HWInterface/RD53BInterface.cc
                    // create arrays to store values of each chip
                    std::vector<double> ntc_adc_list;
                    std::vector<double> r_ref_list;
                    std::vector<double> dac_ntc_list;

                    unsigned int num_dac_step  = 5;
                    unsigned int dac_ntc_step  = 200;
                    bool         saturation    = true;
                    unsigned int max_r_ref_adc = 0;
                    unsigned int max_ntc_adc   = 0;
                    while((dac_ntc_step > 25) and saturation)
                    {
                        const unsigned int dac_ntc = dac_ntc_step * num_dac_step;
                        unsigned int       numtry  = 0;
                        while(numtry++ < 5)
                        {
                            chipInterface->WriteChipReg(cChip, "DAC_NTC", dac_ntc);
                            const auto ntc_adc   = chipInterface->ReadChipADC(cChip, "NTC_VOLT");
                            const auto r_ref_adc = chipInterface->ReadChipADC(cChip, "NTC_CURR");
                            if((ntc_adc < 4095) && (r_ref_adc < 4095))
                            {
                                saturation    = false;
                                max_r_ref_adc = r_ref_adc;
                                max_ntc_adc   = ntc_adc;
                                break;
                            }
                        }
                        dac_ntc_step /= 2;
                    }
                    if(saturation) { LOG(WARNING) << "failed to find a working range without saturation"; }
                    else { LOG(INFO) << "ntc dac step size set to " << dac_ntc_step << "  max dac = " << num_dac_step * dac_ntc_step << "  ref=" << max_r_ref_adc << "  ntc = " << max_ntc_adc; }

                    if(fVerbose) { LOG(INFO) << "DAC     ADC_ntc  ADC_ref    R_ntc (kOhm)  T(R_ntc) (C)"; }

                    for(unsigned int step = 1; step <= num_dac_step; step++)
                    {
                        unsigned int dac_ntc = step * dac_ntc_step;
                        chipInterface->WriteChipReg(cChip, "DAC_NTC", dac_ntc);
                        const auto ntc_adc   = chipInterface->ReadChipADC(cChip, "NTC_VOLT");
                        const auto r_ref_adc = chipInterface->ReadChipADC(cChip, "NTC_CURR");
                        if((ntc_adc > 0) && (ntc_adc < 4095) && (r_ref_adc > 0) && (r_ref_adc < 4095))
                        {
                            r_ref_list.push_back(r_ref_adc);
                            ntc_adc_list.push_back(ntc_adc);
                            dac_ntc_list.push_back(dac_ntc);
                            if(fVerbose)
                            {
                                float R_ntc = fR_ref * ntc_adc / r_ref_adc;
                                LOG(INFO) << std::fixed << std::setw(4) << dac_ntc << " " << std::fixed << std::setw(8) << ntc_adc << " " << std::fixed << std::setw(8) << r_ref_adc << " "
                                          << std::fixed << std::setw(12) << std::setprecision(3) << R_ntc << std::fixed << std::setw(12) << std::setprecision(1) << TfromR(R_ntc, R25NTC, beta);
                            }
                        }
                        else { LOG(INFO) << RED << "ERROR reading ntc" << RESET; }
                    }

                    // for each chip calculate temperature  from slopes of ntc_adc/r_ref_adc values
                    if(dac_ntc_list.size() > 1)
                    {
                        const auto fit_ntc                      = LinReg(dac_ntc_list, ntc_adc_list);
                        const auto fit_ref                      = LinReg(dac_ntc_list, r_ref_list);
                        double     R_ntc_slope                  = fR_ref * fit_ntc.first / fit_ref.first;
                        ntc_temp_from_slope_map[cChip->getId()] = TfromR(R_ntc_slope, R25NTC, beta);
                        if(fVerbose)
                        {
                            LOG(INFO) << "offset  " << std::fixed << std::setw(8) << std::setprecision(2) << fit_ntc.second << " " << std::fixed << std::setw(8) << std::setprecision(2)
                                      << fit_ref.second << " ";
                            LOG(INFO) << "slope" << std::fixed << std::setw(8) << std::setprecision(2) << fit_ntc.first << " " << std::fixed << std::setw(8) << std::setprecision(2) << fit_ref.first
                                      << " " << std::fixed << std::setw(12) << std::setprecision(3) << R_ntc_slope << std::fixed << std::setw(12) << std::setprecision(1)
                                      << TfromR(R_ntc_slope, R25NTC, beta);
                        }
                    }
                    else
                    {
                        ntc_temp_from_slope_map[cChip->getId()] = 99.; // nan("")
                    }
                    chipInterface->WriteChipReg(cChip, "DAC_NTC", 100);
                }
                LOG(INFO) << "NTC result for ChipID 15-12  (C)   : " << std::setw(8) << std::setprecision(1) << std::fixed << ntc_temp_from_slope_map[15] << std::setw(8) << std::setprecision(1)
                          << std::fixed << ntc_temp_from_slope_map[14] << std::setw(8) << std::setprecision(1) << std::fixed << ntc_temp_from_slope_map[13] << std::setw(8) << std::setprecision(1)
                          << std::fixed << ntc_temp_from_slope_map[12];
            }
    // ##################
    // # Reset sequence #
    // ##################
    LOG(INFO) << "TEPXQuadNTC resetting/reconfiguring board" << RESET;
    for(const auto cBoard: *fDetectorContainer)
    {
        static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->ResetBoard();
        static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->ConfigureBoard(cBoard);
        this->ConfigureIT(cBoard);
        this->ConfigureFrontendIT(cBoard); // spits out an error about HITOR_2_CNT readback
    }
    fDetectorContainer->resetReadoutChipQueryFunction();
    fDetectorContainer->setEnabledAll(true);
    LOG(INFO) << "TEPXQuadNTC done" << RESET;
}

void TEPXQuadNTC::draw(bool saveData) { LOG(INFO) << GREEN << "[TEPXQuadNTC::draw]" << RESET; }

void TEPXQuadNTC::analyze() { LOG(INFO) << GREEN << "[TEPXQuadNTC::analyze]" << RESET; }

void TEPXQuadNTC::fillHisto() { LOG(INFO) << GREEN << "[TEPXQuadNTC::fillHisto]" << RESET; }
