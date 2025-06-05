/*!
  \file                  RD53BMuxReader.cc
  \brief
  \author
  \version               1.0
  \date
  Support:               none
*/

#include "RD53BMuxReader.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void RD53BMuxReader::ConfigureCalibration() { LOG(INFO) << GREEN << "[RD53BMuxReader::ConfigureCalibration]" << RESET; }

void RD53BMuxReader::Running()
{
    theCurrentRun = this->fRunNumber;
    LOG(INFO) << GREEN << "[RD53BMuxReader::Running] Starting run: " << BOLDYELLOW << theCurrentRun << RESET;

    RD53BMuxReader::run();
    // RD53BMuxReader::analyze();
    // RD53BMuxReader::sendData();
}

void RD53BMuxReader::sendData() {}

void RD53BMuxReader::Stop()
{
    LOG(INFO) << GREEN << "[RD53BMuxReader::Stop] Stopping" << RESET;

    Tool::Stop();

    RD53BMuxReader::draw();
    // this->closeFileHandler();

    RD53RunProgress::reset();
}

void RD53BMuxReader::localConfigure(const std::string& histoFileName, int currentRun)
{
    LOG(INFO) << GREEN << "[RD53BMuxReader::localConfigure] Starting run: " << BOLDYELLOW << theCurrentRun << RESET;
}

void RD53BMuxReader::run()
{
    auto chipInterface = static_cast<RD53Interface*>(this->fReadoutChipInterface);

    CalibBase::prepareChipQueryForEnDis("chipSubset"); //  ??
    std::vector<std::string> muxlist = {"Iref",
                                        "NTC_VOLT",
                                        "ANA_IN_CURR",
                                        "ANA_SHUNT_CURR",
                                        "DIG_IN_CURR",
                                        "DIG_SHUNT_CURR",
                                        "VIND",
                                        "VINA",
                                        "VDDD",
                                        "VDDA",
                                        "VOFS",
                                        "VrefA",
                                        "VrefD",
                                        "Vref_CORE",
                                        "Vref_PRE",
                                        "POLY_TEMPSENS_TOP",
                                        "POLY_TEMPSENS_BOTTOM",
                                        "TEMPSENS_ANA_SLDO",
                                        "TEMPSENS_DIG_SLDO",
                                        "TEMPSENS_CENTER",
                                        "RADSENS_ANA_SLDO",
                                        "RADSENS_DIG_SLDO",
                                        "RADSENS_CENTER",
                                        "VCAL_HI",
                                        "VCAL_MED",
                                        "LIN_FE_REF_KRUMCURR",
                                        "LIN_FE_GDAC_MAIN",
                                        "LIN_FE_GDAC_LEFT",
                                        "LIN_FE_GDAC_RIGHT"

    };
    const float              R_IMUX  = 4.99;  // kOhm, R17(ABCD) on TEPX hdis
    const float              V_REF   = 0.845; // TEPX  84.5 k x Iref x 2.5, nominal according to RD53B manual
    // =>  the factor appearing in all current measurements, V_REF / 4096 / R_IMUX
    // is equal to  R_VREF_ADC / R_IMUX / 4096 x I_REF

    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
            {
                std::vector<unsigned int>                                         chip_ids;
                std::vector<float>                                                chip_currents;
                std::map<std::string, std::vector<std::pair<float, std::string>>> summary;
                for(const auto cChip: *cHybrid)
                {
                    LOG(INFO) << GREEN << "[RD53BMuxReader::run]  board/opticalGroup/hybrid/chip = " << BOLDYELLOW << cBoard->getId() << "/" << cOpticalGroup->getId() << "/" << cHybrid->getId() << "/"
                              << +cChip->getId() << RESET;
                    chip_ids.push_back(cChip->getId());
                    float Icroc = 0;

                    // "calibrate" the current measurement offset, assuming the ntc dac has no offset
                    float        x_sum = 0, y_sum = 0, x2_sum = 0, xy_sum = 0;
                    unsigned int n = 0;
                    for(unsigned int idac = 10; idac < 100; idac += 10)
                    {
                        chipInterface->WriteChipReg(cChip, "DAC_NTC", idac);
                        const auto adc = chipInterface->ReadChipADC(cChip, "NTC_CURR");
                        if((adc > 0) && (adc < 4096))
                        {
                            n += 1;
                            x_sum += idac;
                            y_sum += adc;
                            x2_sum += idac * idac;
                            xy_sum += adc * idac;
                        }
                    }
                    float offset = (x2_sum * y_sum - x_sum * xy_sum) / (x2_sum * n - x_sum * x_sum);
                    chipInterface->WriteChipReg(cChip, "DAC_NTC", 100); // back to the default value
                    if(offset > 50)
                    {
                        LOG(INFO) << "ignoring offset " << offset;
                        offset = 0;
                    }

                    // raw ADC: for a list of "observables" see RD53BInterface::getADCobservable  in HWInterface/RD53BInterface.cc
                    const auto sampleNtimes                    = cChip->getRegItem("SAMPLE_N_TIMES").fValue;
                    cChip->getRegItem("SAMPLE_N_TIMES").fValue = 1; // local averaging
                    for(const auto& mux: muxlist)
                    {
                        uint32_t           adc_sum       = 0;
                        const unsigned int n_measurement = 12; // TODO configurable
                        unsigned int       n_valid       = 0;
                        std::stringstream  line;

                        // get the mean value of n_measurement tries
                        std::vector<float> adc_values;
                        for(unsigned int n = 0; n < n_measurement; n++)
                        {
                            const auto value = chipInterface->ReadChipADC(cChip, mux);
                            if((value > 0) && (value < 4096))
                            {
                                line << std::fixed << std::setw(5) << value;
                                adc_values.push_back(value);
                                adc_sum += value;
                                n_valid += 1;
                            }
                            else { line << RED << std::fixed << std::setw(5) << value << YELLOW; }
                        }

                        float adc_mean = 0;
                        if(n_valid > 0)
                        {
                            adc_mean = float(adc_sum) / n_valid;
                            // further outlier rejection
                            for(float T = 32; T > 0.9; T /= 2)
                            {
                                float sum_w = 0, sum_wadc = 0;
                                for(const auto& adc: adc_values)
                                {
                                    float q = 0.5 * pow((adc - adc_mean) / (4 * T), 2);
                                    if(q < 10)
                                    {
                                        float w = 1. / (1. + exp(q));
                                        sum_w += w;
                                        sum_wadc += adc * w;
                                    }
                                }

                                if(sum_w > 0) { adc_mean = sum_wadc / sum_w; }
                                else
                                {
                                    line << RED << " no valid mean found" << YELLOW;
                                    break;
                                }
                            }
                        }

                        float value = adc_mean * V_REF / 4096; // nominal 12 bit ADC

                        std::string unit = "  ";
                        if(mux == "Iref")
                        {
                            value = (adc_mean - offset) * V_REF / 4096 / R_IMUX * 1e3;
                            unit  = "uA";
                        }
                        else if((mux == "ANA_IN_CURR") || (mux == "DIG_IN_CURR"))
                        {
                            value = 21.0 * (adc_mean - offset) * V_REF / 4096 / R_IMUX; // scale factor from RD53B manual, table 27
                            unit  = "A ";
                            Icroc += value;
                        }
                        else if((mux == "ANA_SHUNT_CURR") || (mux == "DIG_SHUNT_CURR"))
                        {
                            if(adc_mean > 0)
                            {
                                value = 21.52 * (adc_mean - offset) * V_REF / 4096 / R_IMUX; // scale factor from RD53B manual, table 27
                            }
                            else { value = 0; }
                            unit = "A ";
                        }
                        else if((mux == "VINA") || (mux == "VIND") || (mux == "VOFS"))
                        {
                            value = 4 * adc_mean * V_REF / 4096;
                            unit  = "V ";
                        }
                        else if((mux == "VDDA") || (mux == "VDDD"))
                        {
                            value = 2 * adc_mean * V_REF / 4096;
                            unit  = "V ";
                        }
                        else
                        {
                            value = adc_mean * V_REF / 4096;
                            unit  = "V ";
                        }

                        LOG(INFO) << BOLDBLUE << std::setw(20) << mux << " ADC = " << BOLDYELLOW << line.str() << "  |   " << std::setw(7) << std::setprecision(1) << adc_mean << "   " << std::setw(9)
                                  << std::setprecision(3) << value << " " << unit << RESET;
                        summary[mux].push_back(make_pair(value, unit));
                    } // mux list
                    chip_currents.push_back(Icroc);
                    cChip->getRegItem("SAMPLE_N_TIMES").fValue = sampleNtimes; // restore the original vaue

                } // chips

                std::stringstream header, current;
                float             module_current = 0;
                for(unsigned int c = 0; c < chip_ids.size(); c++)
                {
                    header << std::setw(9) << chip_ids[c] << "   ";
                    current << std::setw(9) << std::setprecision(3) << chip_currents[c] << " A ";
                    module_current += chip_currents[c];
                }
                LOG(INFO) << BOLDBLUE << std::setw(20) << "chip id"
                          << "  | " << header.str() << RESET;
                LOG(INFO) << BOLDBLUE << std::setw(20) << "chip current"
                          << "  | " << current.str() << "  sum = " << std::setw(9) << std::setprecision(3) << module_current << " A" << RESET;
                for(const auto& mux: muxlist)
                {
                    std::stringstream line;
                    for(const auto& result: summary[mux]) { line << std::setw(9) << std::setprecision(3) << result.first << " " << std::setw(2) << result.second; }
                    LOG(INFO) << BOLDBLUE << std::setw(20) << mux << "  | " << line.str() << RESET;
                }
            }

    // ##################
    // # Reset sequence #
    // ##################
    LOG(INFO) << "RD53BMuxReader resetting/reconfiguring board" << RESET;
    for(const auto cBoard: *fDetectorContainer)
    {
        static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->ResetBoard();
        static_cast<RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->ConfigureBoard(cBoard);
        this->ConfigureIT(cBoard);
        this->ConfigureFrontendIT(cBoard);
    }

    LOG(INFO) << "RD53BMuxReader more resetting" << RESET;

    fDetectorContainer->resetReadoutChipQueryFunction();
    fDetectorContainer->setEnabledAll(true);
    LOG(INFO) << "RD53BMuxReader done" << RESET;
}

void RD53BMuxReader::draw(bool saveData) { LOG(INFO) << GREEN << "[RD53BMuxReader::draw]" << RESET; }

void RD53BMuxReader::analyze() { LOG(INFO) << GREEN << "[RD53BMuxReader::analyze]" << RESET; }

void RD53BMuxReader::fillHisto() { LOG(INFO) << GREEN << "[RD53BMuxReader::fillHisto]" << RESET; }
