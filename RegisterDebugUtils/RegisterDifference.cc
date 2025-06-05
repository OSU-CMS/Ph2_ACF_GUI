#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>

enum class ChipType
{
    LpGBT = 0,
    CIC,
    MPA,
    SSA,
    CBC,
    VTRx
};

std::map<std::string, ChipType> theChipTypeMap{{"LpGBT", ChipType::LpGBT}, {"CIC", ChipType::CIC}, {"MPA", ChipType::MPA}, {"SSA", ChipType::SSA}, {"CBC", ChipType::CBC}, {"VTRx", ChipType::VTRx}};

enum class RegisterType
{
    ReadOnly,
    Utility,
    User
};

std::map<ChipType, std::vector<std::pair<std::regex, RegisterType>>> listOfFreeRegistersMap;

void initializeFreeRegisters()
{
    listOfFreeRegistersMap[ChipType::LpGBT] = {std::make_pair(std::regex("^ConfigPins$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^I2CSlaveAddress$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^EPRX[0-6]Locked$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^EPRX[0-6]CurrentPhase[13][02]$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^EPRXEcCurrentPhase$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^EPRX[0-6]DLLStatus$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^I2CM[0-2]Ctrl$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^I2CM[0-2]Mask$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^I2CM[0-2]Status$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^I2CM[0-2]TranCnt$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^I2CM[0-2]Read.*"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^PSStatus$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^PIOIn[HL]$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^FUSEStatus$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^FUSEValues[A-D]$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^ProcessMonitorStatus$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^PMFreq[A-C]$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^SEUCount[HL]$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^CLKGStatus[0-9]$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^DLDPFecCorrectionCount[0-3]$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^ADCStatus[HL]$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^EOMStatus$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^EOMCounterValue[HL]$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^EOMCounter40M[HL]$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^BERTStatus$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^BERTResult[0-4]$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^ROM$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^PORBOR$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^PUSM.*"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^CRCValue[0-3]$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^FailedCRC$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^TOValue$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^SCStatus$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^FAState$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^FAHeader.*"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^FALossOfLockCount$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^ConfigErrorCounter[HL]$"), RegisterType::ReadOnly),
                                               std::make_pair(std::regex("^FUSEControl$"), RegisterType::Utility),
                                               std::make_pair(std::regex("^FUSEBlowData[A-D]$"), RegisterType::Utility),
                                               std::make_pair(std::regex("^FUSEBlowAdd[HL]$"), RegisterType::Utility),
                                               std::make_pair(std::regex("^FuseMagic$"), RegisterType::Utility),
                                               std::make_pair(std::regex("^I2CM[0-1]Address$"), RegisterType::Utility),
                                               std::make_pair(std::regex("^I2CM[0-2]Cmd$"), RegisterType::Utility),
                                               std::make_pair(std::regex("^I2CM[0-2]Data[0-3]$"), RegisterType::Utility),
                                               std::make_pair(std::regex("^POWERUP2$"), RegisterType::Utility),
                                               std::make_pair(std::regex("^EPRX[0-6][0-3]ChnCntr_phase$"), RegisterType::Utility)};

    listOfFreeRegistersMap[ChipType::CIC] = {std::make_pair(std::regex("^EfuseValue[0-3]$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^ASYNC_CNTL_BLOCK[0-3]$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^SYNC_CNTL_BLOCK[0-3]$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^scPhaseSelectB[0-3]o[0-5]$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^scDllInstantLock[01]$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^scDllLocked[01]$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^scChannelLocked[0-5]$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^timingStatusBits$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^BX0_DELAY$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^WA_DELAY\\d{2}$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^MASK_BLOCK[0-3]$"), RegisterType::Utility)};

    listOfFreeRegistersMap[ChipType::CBC] = {std::make_pair(std::regex("^ChipIDFuse[1-3]$"), RegisterType::ReadOnly), std::make_pair(std::regex("^BandgapFuse$"), RegisterType::ReadOnly)};

    listOfFreeRegistersMap[ChipType::MPA] = {std::make_pair(std::regex("^EfuseValue[0-3]$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex(".*ync_SEUcnt.*"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^ErrorL1$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^Ofcnt$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^DLLlocked$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex(".*_[ML]SB.*"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^L1_.*_.*"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^OF_.*_count$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex(".*BIST_.*"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^EfuseProg[0-3]$"), RegisterType::Utility),
                                             std::make_pair(std::regex("^Mask$"), RegisterType::Utility),
                                             std::make_pair(std::regex(".*_ALL"), RegisterType::Utility)};

    listOfFreeRegistersMap[ChipType::SSA] = {std::make_pair(std::regex("^SEUcnt$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^Ring_oscillator$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^ADC_out$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^bist_output$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^AC_ReadCounter$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^status_reg$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^Fuse_Value_b[0-3]$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^AC_ReadCounter[LM]SB_S.*"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^status_reg$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex(".*_Cnt_[LH]$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^bist_output$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^ADC_out_[LH]$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^Ring_oscillator_out_loc[TB][LCR]_T[12]_[HL]$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^bist_memory_sram_output_[HL]_[0-9A-F]$"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex(".*ync_SEUcnt_.*"), RegisterType::ReadOnly),
                                             std::make_pair(std::regex("^mask_strip$"), RegisterType::Utility),
                                             std::make_pair(std::regex("^mask_peri_[AD]$"), RegisterType::Utility),
                                             std::make_pair(std::regex("^Fuse_Prog_b[0-3]$"), RegisterType::Utility),
                                             std::make_pair(std::regex("^ENFLAGS$"), RegisterType::Utility),
                                             std::make_pair(std::regex("^StripControl2$"), RegisterType::Utility),
                                             std::make_pair(std::regex("^THTRIMMING$"), RegisterType::Utility),
                                             std::make_pair(std::regex("^DigCalibPattern_[LH]$"), RegisterType::Utility),
                                             std::make_pair(std::regex("^AC_ReadCounter[LM]SB$"), RegisterType::Utility)};

    listOfFreeRegistersMap[ChipType::VTRx] = {std::make_pair(std::regex("^STATUS$"), RegisterType::ReadOnly),
                                              std::make_pair(std::regex("^ID$"), RegisterType::ReadOnly),
                                              std::make_pair(std::regex("^UID[0-3]$"), RegisterType::ReadOnly),
                                              std::make_pair(std::regex("^SEU[0-3]$"), RegisterType::Utility)};
}

bool matchWithPatternList(ChipType theChipType, std::string registerName)
{
    for(const auto pattern: listOfFreeRegistersMap[theChipType])
    {
        if(std::regex_search(registerName, pattern.first)) { return true; }
    }
    return false;
}

// Function to extract the last hex number from a line
std::pair<std::string, uint8_t> getRegNameAndValue(const std::string& line, ChipType theChipType)
{
    std::istringstream input(line);
    std::string        name, pageString, addressString, defValueString, valueString;
    input >> name >> pageString >> addressString >> defValueString >> valueString;
    if(matchWithPatternList(theChipType, name)) return std::make_pair<std::string, uint8_t>("DUMMY", 0);
    uint8_t value = (theChipType == ChipType::LpGBT) ? strtoul(defValueString.c_str(), 0, 16) : strtoul(valueString.c_str(), 0, 16);
    return std::make_pair(name, value);
}

bool isValidLine(const std::string& line)
{
    // Check if the line starts with '*' or '#'
    return (line.empty() || line[0] != '*' && line[0] != '#');
}

int main(int argc, char* argv[])
{
    if(argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <file1_path> <file2_path> <chipType>" << std::endl;
        return 1;
    }

    std::cout << "Comparing file " << argv[1] << " with file " << argv[2] << std::endl;

    std::ifstream file1(argv[1]);
    std::ifstream file2(argv[2]);
    std::string   chipTypeName(argv[3]);

    ChipType theChipType;
    try
    {
        theChipType = theChipTypeMap.at(chipTypeName);
    }
    catch(const std::exception& e)
    {
        std::cerr << "Cannot recognize chip type " << chipTypeName << '\n';
        return 0;
    }

    if(!file1.is_open() || !file2.is_open())
    {
        std::cerr << "Error opening input files." << std::endl;
        return 1;
    }

    initializeFreeRegisters();

    // Map to store last hex values for each register from both files
    std::unordered_map<std::string, uint8_t> registerMap;

    std::string line;
    while(std::getline(file1, line))
    {
        if(isValidLine(line)) { registerMap.emplace(getRegNameAndValue(line, theChipType)); }
    }

    while(std::getline(file2, line))
    {
        if(isValidLine(line))
        {
            auto nameAndValue = getRegNameAndValue(line, theChipType);
            auto name         = nameAndValue.first;
            auto value        = nameAndValue.second;

            // Check if the register exists in file1
            if(registerMap.find(name) != registerMap.end())
            {
                // Compare the last hex values
                if(registerMap[name] != value)
                {
                    // Print the result
                    std::cout << std::hex << "Modified register: " << name << " 0x" << +registerMap[name] << " -> 0x" << +value << std::dec << std::endl;
                }
            }
            else { std::cerr << "Register " << name << " not found in file1." << std::endl; }
        }
    }

    file1.close();
    file2.close();

    std::cout << "End of comparison \n" << std::endl;

    return 0;
}
