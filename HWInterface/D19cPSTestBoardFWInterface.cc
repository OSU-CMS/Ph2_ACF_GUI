#include "HWInterface/D19cPSTestBoardFWInterface.h"

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
D19cPSTestBoardFWInterface::D19cPSTestBoardFWInterface(const char* puHalConfigFileName, uint32_t pBoardId, FileHandler* pFileHandler, BeBoard* theBoard)
    : BeBoardFWInterface(puHalConfigFileName, pBoardId, theBoard)
{
}
D19cPSTestBoardFWInterface::D19cPSTestBoardFWInterface(const char* pId, const char* pUri, const char* pAddressTable, FileHandler* pFileHandler, BeBoard* theBoard)
    : BeBoardFWInterface(pId, pUri, pAddressTable, theBoard)
{
}
D19cPSTestBoardFWInterface::~D19cPSTestBoardFWInterface() {}

void D19cPSTestBoardFWInterface::PSInterfaceBoard_PowerOn_SSA(float VDDPST, float DVDD, float AVDD, float VBF, float BG, uint8_t ENABLE)
{
    // this->getBoardInfo();
    this->PSInterfaceBoard_PowerOn(0, 0);

    uint32_t write   = 0;
    uint32_t SLOW    = 2;
    uint32_t i2cmux  = 0;
    uint32_t pcf8574 = 1;
    uint32_t dac7678 = 4;
    std::this_thread::sleep_for(std::chrono::milliseconds(750));

    PSInterfaceBoard_SetSlaveMap();
    PSInterfaceBoard_ConfigureI2CMaster(1, SLOW);
    std::this_thread::sleep_for(std::chrono::milliseconds(750));

    float Vc = 0.0003632813;

    LOG(INFO) << "ssa vdd on";

    float Vlimit = 1.32;
    if(VDDPST > Vlimit) VDDPST = Vlimit;
    float    diffvoltage = 1.5 - VDDPST;
    uint32_t setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;

    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01); // to SCO on PCA9646
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x33, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    LOG(INFO) << "ssa vddD on";
    Vlimit = 1.32;
    if(DVDD > Vlimit) DVDD = Vlimit;
    diffvoltage = 1.5 - DVDD;
    setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01); // to SCO on PCA9646
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x31, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    LOG(INFO) << "ssa vddA on";
    Vlimit = 1.32;
    if(AVDD > Vlimit) AVDD = Vlimit;
    diffvoltage = 1.5 - AVDD;
    setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01); // to SCO on PCA9646
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x35, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    LOG(INFO) << "ssa BG on";
    Vlimit = 1.32;
    if(BG > Vlimit) BG = Vlimit;
    float Vc2  = 4095 / 1.5;
    setvoltage = int(round(BG * Vc2));
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01); // to SCO on PCA9646
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x36, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    LOG(INFO) << "ssa VBF on";
    Vlimit = 0.5;
    if(VBF > Vlimit) VBF = Vlimit;
    setvoltage = int(round(VBF * Vc2));
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01); // to SCO on PCA9646
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x37, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    uint32_t VAL = (ENABLE);
    LOG(INFO) << BOLDRED << VAL << "  writeme!" << RESET;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x02);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    PSInterfaceBoard_SendI2CCommand(pcf8574, 0, write, 0, VAL); // set reset bit

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    PSInterfaceBoard_ConfigureI2CMaster(0, SLOW);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}
void D19cPSTestBoardFWInterface::PSInterfaceBoard_SetSlaveMap()
{
    std::vector<std::vector<uint32_t>> i2c_slave_map;
    i2c_slave_map.push_back({0x70, 0, 1, 1, 0, 1}); // 0  PCA9646
    i2c_slave_map.push_back({0x20, 0, 1, 1, 0, 1}); // 1  PCF8574
    i2c_slave_map.push_back({0x24, 0, 1, 1, 0, 1}); // 2  PCF8574
    i2c_slave_map.push_back({0x14, 0, 2, 3, 0, 1}); // 3  LTC2487
    i2c_slave_map.push_back({0x48, 1, 2, 2, 0, 0}); // 4  DAC7678
    i2c_slave_map.push_back({0x40, 1, 2, 2, 0, 1}); // 5  INA226
    i2c_slave_map.push_back({0x41, 1, 2, 2, 0, 1}); // 6  INA226
    i2c_slave_map.push_back({0x42, 1, 2, 2, 0, 1}); // 7  INA226
    i2c_slave_map.push_back({0x44, 1, 2, 2, 0, 1}); // 8  INA226
    i2c_slave_map.push_back({0x45, 1, 2, 2, 0, 1}); // 9  INA226
    i2c_slave_map.push_back({0x46, 1, 2, 2, 0, 1}); // 10  INA226
    i2c_slave_map.push_back({0x40, 2, 1, 1, 1, 0}); // 11  ????
    i2c_slave_map.push_back({0x20, 2, 1, 1, 1, 0}); // 12  ????
    i2c_slave_map.push_back({0x0, 0, 1, 1, 0, 0});  // 13  ????
    i2c_slave_map.push_back({0x0, 0, 1, 1, 0, 0});  // 14  ????
    i2c_slave_map.push_back({0x5F, 1, 1, 1, 1, 0}); // 15  CBC3

    LOG(INFO) << "Updating the Slave ID Map (mpa ssa board) ";

    for(int ism = 0; ism < 16; ism++)
    {
        uint32_t    shifted_i2c_address             = i2c_slave_map.at(ism).at(0) << 25;
        uint32_t    shifted_register_address_nbytes = i2c_slave_map.at(ism).at(1) << 6;
        uint32_t    shifted_data_wr_nbytes          = i2c_slave_map.at(ism).at(2) << 4;
        uint32_t    shifted_data_rd_nbytes          = i2c_slave_map.at(ism).at(3) << 2;
        uint32_t    shifted_stop_for_rd_en          = i2c_slave_map.at(ism).at(4) << 1;
        uint32_t    shifted_nack_en                 = i2c_slave_map.at(ism).at(5) << 0;
        uint32_t    final_command = shifted_i2c_address + shifted_register_address_nbytes + shifted_data_wr_nbytes + shifted_data_rd_nbytes + shifted_stop_for_rd_en + shifted_nack_en;
        std::string curreg        = "fc7_daq_cnfg.mpa_ssa_board_block.slave_" + std::to_string(ism) + "_config";
        WriteReg(curreg, final_command);
    }
}

void D19cPSTestBoardFWInterface::PSInterfaceBoard_ConfigureI2CMaster(uint32_t pEnabled, uint32_t pFrequency)
{
    // wait for all commands to be executed
    std::chrono::microseconds cWait(fWait_us);
    while(!ReadReg("fc7_daq_stat.command_processor_block.i2c.command_fifo.empty")) { std::this_thread::sleep_for(cWait); }

    if(pEnabled > 0)
        LOG(INFO) << "Enabling the MPA SSA Board I2C master";
    else
        LOG(INFO) << "Disabling the MPA SSA Board I2C master";

    // setting the values
    WriteReg("fc7_daq_cnfg.physical_interface_block.i2c.master_en", int(not pEnabled));
    WriteReg("fc7_daq_cnfg.mpa_ssa_board_block.i2c_master_en", pEnabled);
    WriteReg("fc7_daq_cnfg.mpa_ssa_board_block.i2c_freq", pFrequency);
    std::this_thread::sleep_for(cWait);

    // resetting the fifos and the board
    WriteReg("fc7_daq_ctrl.command_processor_block.i2c.control.reset", 1);
    WriteReg("fc7_daq_ctrl.command_processor_block.i2c.control.reset_fifos", 1);
    WriteReg("fc7_daq_ctrl.mpa_ssa_board_block.reset", 1);
    std::this_thread::sleep_for(cWait);
}

void D19cPSTestBoardFWInterface::PSInterfaceBoard_SendI2CCommand(uint32_t slave_id, uint32_t board_id, uint32_t read, uint32_t register_address, uint32_t data)
{
    std::chrono::microseconds cWait(fWait_us);

    uint32_t shifted_command_type     = 1 << 31;
    uint32_t shifted_word_id_0        = 0;
    uint32_t shifted_slave_id         = slave_id << 21;
    uint32_t shifted_board_id         = board_id << 20;
    uint32_t shifted_read             = read << 16;
    uint32_t shifted_register_address = register_address;

    uint32_t shifted_word_id_1 = 1 << 26;
    uint32_t shifted_data      = data;

    uint32_t word_0 = shifted_command_type + shifted_word_id_0 + shifted_slave_id + shifted_board_id + shifted_read + shifted_register_address;
    uint32_t word_1 = shifted_command_type + shifted_word_id_1 + shifted_data;

    WriteReg("fc7_daq_ctrl.command_processor_block.i2c.command_fifo", word_0);
    std::this_thread::sleep_for(cWait);
    WriteReg("fc7_daq_ctrl.command_processor_block.i2c.command_fifo", word_1);
    std::this_thread::sleep_for(cWait);

    int readempty = ReadReg("fc7_daq_stat.command_processor_block.i2c.reply_fifo.empty");
    while(readempty > 0)
    {
        std::this_thread::sleep_for(cWait);
        readempty = ReadReg("fc7_daq_stat.command_processor_block.i2c.reply_fifo.empty");
    }

    int reply_err  = ReadReg("fc7_daq_ctrl.command_processor_block.i2c.mpa_ssa_i2c_reply.err");
    int reply_data = ReadReg("fc7_daq_ctrl.command_processor_block.i2c.mpa_ssa_i2c_reply.data");
    if(reply_err == 1)
        LOG(ERROR) << "Error code: " << std::hex << reply_data << std::dec;
    else
    {
        if(read == 1)
            LOG(INFO) << BOLDBLUE << "Data that was read is: " << reply_data << RESET;
        else
            LOG(DEBUG) << BOLDBLUE << "Successful write transaction" << RESET;
    }
}

uint32_t D19cPSTestBoardFWInterface::PSInterfaceBoard_SendI2CCommand_READ(uint32_t slave_id, uint32_t board_id, uint32_t read, uint32_t register_address, uint32_t data)
{
    std::chrono::microseconds cWait(fWait_us);

    uint32_t shifted_command_type     = 1 << 31;
    uint32_t shifted_word_id_0        = 0;
    uint32_t shifted_slave_id         = slave_id << 21;
    uint32_t shifted_board_id         = board_id << 20;
    uint32_t shifted_read             = read << 16;
    uint32_t shifted_register_address = register_address;

    uint32_t shifted_word_id_1 = 1 << 26;
    uint32_t shifted_data      = data;

    uint32_t word_0 = shifted_command_type + shifted_word_id_0 + shifted_slave_id + shifted_board_id + shifted_read + shifted_register_address;
    uint32_t word_1 = shifted_command_type + shifted_word_id_1 + shifted_data;

    WriteReg("fc7_daq_ctrl.command_processor_block.i2c.command_fifo", word_0);
    std::this_thread::sleep_for(cWait);
    WriteReg("fc7_daq_ctrl.command_processor_block.i2c.command_fifo", word_1);
    std::this_thread::sleep_for(cWait);

    int readempty = ReadReg("fc7_daq_stat.command_processor_block.i2c.reply_fifo.empty");
    LOG(INFO) << BOLDBLUE << readempty << RESET;
    while(readempty > 0)
    {
        std::cout << ".";
        std::this_thread::sleep_for(cWait);
        readempty = ReadReg("fc7_daq_stat.command_processor_block.i2c.reply_fifo.empty");
    }
    std::cout << std::endl;

    uint32_t reply = ReadReg("fc7_daq_ctrl.command_processor_block.i2c.mpa_ssa_i2c_reply");
    // LOG (INFO) << BOLDRED << std::hex << reply << std::dec << RESET;
    uint32_t reply_err  = ReadReg("fc7_daq_ctrl.command_processor_block.i2c.mpa_ssa_i2c_reply.err");
    uint32_t reply_data = ReadReg("fc7_daq_ctrl.command_processor_block.i2c.mpa_ssa_i2c_reply.data");

    if(reply_err == 1)
        LOG(ERROR) << "Error code: " << std::hex << reply_data << std::dec;
    else
    {
        if(read == 1)
        {
            LOG(INFO) << BOLDBLUE << "Data that was read is: " << std::hex << reply_data << std::dec << "   ecode: " << reply_err << RESET;
            return reply & 0xFFFFFF;
        }
        else
            LOG(DEBUG) << BOLDBLUE << "Successful write transaction" << RESET;
    }

    return 0;
}
void D19cPSTestBoardFWInterface::PSInterfaceBoard_PowerOn(uint8_t mpaid, uint8_t ssaid)
{
    uint32_t write       = 0;
    uint32_t SLOW        = 2;
    uint32_t i2cmux      = 0;
    uint32_t powerenable = 2;

    PSInterfaceBoard_SetSlaveMap();

    LOG(INFO) << "Interface Board Power ON";

    PSInterfaceBoard_ConfigureI2CMaster(1, SLOW);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x02);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    PSInterfaceBoard_SendI2CCommand(powerenable, 0, write, 0, 0x00); // There is an inverter! Be Careful!
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    PSInterfaceBoard_ConfigureI2CMaster(0, SLOW);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void D19cPSTestBoardFWInterface::PSInterfaceBoard_PowerOff()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    uint32_t write       = 0;
    uint32_t SLOW        = 2;
    uint32_t i2cmux      = 0;
    uint32_t powerenable = 2;

    PSInterfaceBoard_SetSlaveMap();

    LOG(INFO) << "Interface Board Power OFF";

    PSInterfaceBoard_ConfigureI2CMaster(1, SLOW);
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x02);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PSInterfaceBoard_SendI2CCommand(powerenable, 0, write, 0, 0x01);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PSInterfaceBoard_ConfigureI2CMaster(0, SLOW);
}

void D19cPSTestBoardFWInterface::ReadPower_SSA(uint8_t mpaid, uint8_t ssaid)
{
    uint32_t read     = 1;
    uint32_t write    = 0;
    uint32_t SLOW     = 2;
    uint32_t i2cmux   = 0;
    uint32_t ina226_7 = 7;
    uint32_t ina226_6 = 6;
    uint32_t ina226_5 = 5;

    LOG(INFO) << BOLDBLUE << "power information:" << RESET;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PSInterfaceBoard_SetSlaveMap();
    PSInterfaceBoard_ConfigureI2CMaster(1, SLOW);

    LOG(INFO) << BOLDBLUE << " - - - VDD:" << RESET;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x08);
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    uint32_t dread2 = PSInterfaceBoard_SendI2CCommand_READ(ina226_7, 0, read, 0x02, 0);
    LOG(INFO) << BOLDRED << "BIT VAL OF VDD = " << dread2 << RESET;
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    float    vret   = float(dread2) * 0.00125;
    uint32_t dread1 = PSInterfaceBoard_SendI2CCommand_READ(ina226_7, 0, read, 0x01, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    float iret = float(dread1) * 0.00250 / 0.1;
    float pret = vret * iret;
    LOG(INFO) << BOLDGREEN << "V = " << vret << "V, I = " << iret << "mA, P = " << pret << "mW" << RESET;

    LOG(INFO) << BOLDBLUE << " - - - Digital:" << RESET;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x08);
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    dread2 = PSInterfaceBoard_SendI2CCommand_READ(ina226_6, 0, read, 0x02, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    vret   = float(dread2) * 0.00125;
    dread1 = PSInterfaceBoard_SendI2CCommand_READ(ina226_6, 0, read, 0x01, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    iret = float(dread1) * 0.00250 / 0.1;
    pret = vret * iret;
    LOG(INFO) << BOLDGREEN << "V = " << vret << "V, I = " << iret << "mA, P = " << pret << "mW" << RESET;

    LOG(INFO) << BOLDBLUE << " - - - Analog:" << RESET;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x08);
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    dread2 = PSInterfaceBoard_SendI2CCommand_READ(ina226_5, 0, read, 0x02, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    vret   = float(dread2) * 0.00125;
    dread1 = PSInterfaceBoard_SendI2CCommand_READ(ina226_5, 0, read, 0x01, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    iret = float(dread1) * 0.00250 / 0.1;
    pret = vret * iret;
    LOG(INFO) << BOLDGREEN << "V = " << vret << "V, I = " << iret << "mA, P = " << pret << "mW" << RESET;

    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x04);
    PSInterfaceBoard_ConfigureI2CMaster(0);
}

void D19cPSTestBoardFWInterface::PSInterfaceBoard_PowerOn_MPA(float VDDPST, float DVDD, float AVDD, float VBG, uint8_t mpaid, uint8_t ssaid)
{
    uint32_t                  write   = 0;
    uint32_t                  SLOW    = 2;
    uint32_t                  i2cmux  = 0;
    uint32_t                  pcf8574 = 1;
    uint32_t                  dac7678 = 4;
    std::chrono::milliseconds cWait(10);
    this->getBoardInfo();
    this->PSInterfaceBoard_PowerOn(0, 0);
    PSInterfaceBoard_SetSlaveMap();
    PSInterfaceBoard_ConfigureI2CMaster(1, SLOW);

    float Vc = 0.0003632813;

    LOG(INFO) << "mpa vdd on";

    float Vlimit = 1.32;
    if(VDDPST > Vlimit) VDDPST = Vlimit;
    float    diffvoltage = 1.5 - VDDPST;
    uint32_t setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;

    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);           // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x34, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(cWait);

    LOG(INFO) << "mpa vddD on";
    Vlimit = 1.2;
    if(DVDD > Vlimit) DVDD = Vlimit;
    diffvoltage = 1.5 - DVDD;
    setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);           // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x30, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(cWait);

    LOG(INFO) << "mpa vddA on";
    Vlimit = 1.32;
    if(AVDD > Vlimit) AVDD = Vlimit;
    diffvoltage = 1.5 - AVDD;
    setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);           // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x32, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(cWait);

    LOG(INFO) << "mpa VBG on";
    Vlimit = 0.5;
    if(VBG > Vlimit) VBG = Vlimit;
    float Vc2  = 4095 / 1.5;
    setvoltage = int(round(VBG * Vc2));
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);           // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x36, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(cWait);

    LOG(INFO) << "mpa enable";
    uint32_t val2 = (mpaid << 5) + 16;
    // uint32_t val2 = (mpaid << 5) + (ssaid << 1) + 1; // reset bit for MPA
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x02);  // route to 2nd PCF8574
    PSInterfaceBoard_SendI2CCommand(pcf8574, 0, write, 0, val2); // set reset bit
    std::this_thread::sleep_for(cWait);

    // disable the i2c master at the end (first set the mux to the chip
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x04);
    PSInterfaceBoard_ConfigureI2CMaster(0, SLOW);
}

void D19cPSTestBoardFWInterface::PSInterfaceBoard_PowerOn_MPASSA(float VDDPST, float DVDD, float AVDD, float VBG, float VBF, uint8_t mpaid, uint8_t ssaid)
{
    this->getBoardInfo();
    this->PSInterfaceBoard_PowerOn(0, 0);

    uint32_t write   = 0;
    uint32_t SLOW    = 2;
    uint32_t i2cmux  = 0;
    uint32_t pcf8574 = 1;
    uint32_t dac7678 = 4;
    std::this_thread::sleep_for(std::chrono::milliseconds(750));
    PSInterfaceBoard_SetSlaveMap();
    PSInterfaceBoard_ConfigureI2CMaster(1, SLOW);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    float Vc = 0.0003632813;

    LOG(INFO) << "mpa vdd on";

    float Vlimit = 1.32;
    if(VDDPST > Vlimit) VDDPST = Vlimit;
    float    diffvoltage = 1.5 - VDDPST;
    uint32_t setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;

    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);           // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x34, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    LOG(INFO) << "ssa vdd on";

    Vlimit = 1.32;
    if(VDDPST > Vlimit) VDDPST = Vlimit;
    diffvoltage = 1.5 - VDDPST;
    setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;

    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01); // to SCO on PCA9646
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x33, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    LOG(INFO) << "mpa vddD on";
    Vlimit = 1.2;
    if(DVDD > Vlimit) DVDD = Vlimit;
    diffvoltage = 1.5 - DVDD;
    setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);           // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x30, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    LOG(INFO) << "ssa vddD on";
    Vlimit = 1.32;
    if(DVDD > Vlimit) DVDD = Vlimit;
    diffvoltage = 1.5 - DVDD;
    setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01); // to SCO on PCA9646
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x31, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    LOG(INFO) << "mpa vddA on";
    Vlimit = 1.32;
    if(AVDD > Vlimit) AVDD = Vlimit;
    diffvoltage = 1.5 - AVDD;
    setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);           // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x32, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    LOG(INFO) << "ssa vddA on";
    Vlimit = 1.32;
    if(AVDD > Vlimit) AVDD = Vlimit;
    diffvoltage = 1.5 - AVDD;
    setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01); // to SCO on PCA9646
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x35, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    LOG(INFO) << "mpa VBG on";
    Vlimit = 0.5;
    if(VBG > Vlimit) VBG = Vlimit;
    float Vc2  = 4095 / 1.5;
    setvoltage = int(round(VBG * Vc2));
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);           // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x36, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    /*LOG(INFO) << "ssa VBG on";
    Vlimit = 1.32;
    if(VBG > Vlimit) VBG = Vlimit;
    Vc2  = 4095 / 1.5;
    setvoltage = int(round(VBG * Vc2));
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01); // to SCO on PCA9646
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x36, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));*/

    LOG(INFO) << "ssa VBF on";
    Vlimit = 0.5;
    if(VBF > Vlimit) VBF = Vlimit;
    setvoltage = int(round(VBF * Vc2));
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01); // to SCO on PCA9646
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x37, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x02);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    PSInterfaceBoard_SendI2CCommand(pcf8574, 0, write, 0, 145); // set reset bit

    /*LOG(INFO) << "mpa enable";
    //uint32_t val2 = (mpaid << 5) + 16;
    uint32_t val2 = (mpaid << 5) + (ssaid << 1) + 1; // reset bit for MPA
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x02);  // route to 2nd PCF8574
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    PSInterfaceBoard_SendI2CCommand(pcf8574, 0, write, 0, val2); // set reset bit
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));*/

    // disable the i2c master at the end (first set the mux to the chip
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x04);
    PSInterfaceBoard_ConfigureI2CMaster(0, SLOW);
}

void D19cPSTestBoardFWInterface::PSInterfaceBoard_PowerOff_SSA(uint8_t mpaid, uint8_t ssaid)
{
    uint32_t                  write   = 0;
    uint32_t                  SLOW    = 2;
    uint32_t                  i2cmux  = 0;
    uint32_t                  pcf8574 = 1; // MPA and SSA address and reset 8 bit port
    uint32_t                  dac7678 = 4;
    float                     Vc      = 0.0003632813; // V/Dac step
    std::chrono::milliseconds cWait(1500);

    PSInterfaceBoard_SetSlaveMap();
    PSInterfaceBoard_ConfigureI2CMaster(1, SLOW);

    LOG(INFO) << "ssa disable";
    uint32_t val = (mpaid << 5) + (ssaid << 1);                 // reset bit for MPA
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x02); // route to 2nd PCF8574
    PSInterfaceBoard_SendI2CCommand(pcf8574, 0, write, 0, val); // set reset bit
    std::this_thread::sleep_for(cWait);

    LOG(INFO) << "ssa VBF off";
    uint32_t setvoltage = 0;
    setvoltage          = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);           // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x37, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(cWait);

    LOG(INFO) << "ssa vddA off";
    float diffvoltage = 1.5;
    setvoltage        = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);  // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x35, 0); // tx to DAC C
    std::this_thread::sleep_for(cWait);

    LOG(INFO) << "ssa vddD off";
    diffvoltage = 1.5;
    setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);  // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x31, 0); // tx to DAC C
    std::this_thread::sleep_for(cWait);

    LOG(INFO) << "ssa vdd off";
    diffvoltage = 1.5;
    setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);  // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x33, 0); // tx to DAC C
    std::this_thread::sleep_for(cWait);

    this->PSInterfaceBoard_PowerOff();
}

void D19cPSTestBoardFWInterface::PSInterfaceBoard_PowerOff_MPA(uint8_t mpaid, uint8_t ssaid)
{
    uint32_t                  write   = 0;
    uint32_t                  SLOW    = 2;
    uint32_t                  i2cmux  = 0;
    uint32_t                  pcf8574 = 1; // MPA and SSA address and reset 8 bit port
    uint32_t                  dac7678 = 4;
    float                     Vc      = 0.0003632813; // V/Dac step
    std::chrono::milliseconds cWait(1000);

    PSInterfaceBoard_SetSlaveMap();
    PSInterfaceBoard_ConfigureI2CMaster(1, SLOW);

    LOG(INFO) << "mpa disable";
    uint32_t val = (mpaid << 5) + (ssaid << 1);                 // reset bit for MPA
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x02); // route to 2nd PCF8574
    PSInterfaceBoard_SendI2CCommand(pcf8574, 0, write, 0, val); // set reset bit
    std::this_thread::sleep_for(cWait);

    LOG(INFO) << "mpa VBG off";
    uint32_t setvoltage = 0;
    setvoltage          = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);           // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x36, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(cWait);

    LOG(INFO) << "mpa vddA off";
    float diffvoltage = 1.5;
    setvoltage        = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);           // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x32, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(cWait);

    LOG(INFO) << "mpa vddA off";
    diffvoltage = 1.5;
    setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);           // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x30, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(cWait);

    LOG(INFO) << "mpa vdd off";
    diffvoltage = 1.5;
    setvoltage  = int(round(diffvoltage / Vc));
    if(setvoltage > 4095) setvoltage = 4095;
    setvoltage = setvoltage << 4;
    PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);           // to SCO on PCA9646
    PSInterfaceBoard_SendI2CCommand(dac7678, 0, write, 0x34, setvoltage); // tx to DAC C
    std::this_thread::sleep_for(cWait);
}

void D19cPSTestBoardFWInterface::KillI2C()
{
    PSInterfaceBoard_SendI2CCommand(0, 0, 0, 0, 0x04);
    PSInterfaceBoard_ConfigureI2CMaster(0);
}
// I don't understand what this does
// void D19cPSTestBoardFWInterface::SSAEqualizeDACs(uint8_t pChipId)
// {
//     uint32_t write   = 0;
//     uint32_t read    = 1;
//     uint32_t SLOW    = 2;
//     uint32_t i2cmux  = 0;
//     uint32_t ltc2487 = 3;

//     uint16_t chipSelect = 0x0;
//     if(pChipId == 1) { chipSelect = 0xb180; }
//     if(pChipId == 0) { chipSelect = 0xb080; }
//     PSInterfaceBoard_SetSlaveMap();
//     PSInterfaceBoard_ConfigureI2CMaster(1, SLOW);
//     PSInterfaceBoard_SendI2CCommand(i2cmux, 0, write, 0, 0x01);
//     std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     PSInterfaceBoard_SendI2CCommand(ltc2487, 0, write, 0, chipSelect);
//     std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     uint32_t readSSA = PSInterfaceBoard_SendI2CCommand_READ(ltc2487, 0, read, 0x0, 0); // read value in reg:
//     std::this_thread::sleep_for(std::chrono::milliseconds(50));

//     readSSA = (readSSA >> 6) & 0x0000FFFF;
//     LOG(INFO) << RED << "Value read back: " << (float(readSSA) / 43371.0) << RESET;

//     ReadPower_SSA();
// }
} // namespace Ph2_HwInterface