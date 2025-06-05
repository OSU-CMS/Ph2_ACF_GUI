#ifndef __D19cBackendAlignmentFWInterface_H__
#define __D19cBackendAlignmentFWInterface_H__

#include "HWInterface/BeBoardFWInterface.h"
#include <string>
namespace Ph2_HwDescription
{
class BeBoard;
}

namespace Ph2_HwInterface
{
class D19cPSTestBoardFWInterface : public BeBoardFWInterface
{
  public:
    D19cPSTestBoardFWInterface(const char* puHalConfigFileName, uint32_t pBoardId, FileHandler* pFileHandler, Ph2_HwDescription::BeBoard* theBoard);
    D19cPSTestBoardFWInterface(const char* pId, const char* pUri, const char* pAddressTable, FileHandler* pFileHandler, Ph2_HwDescription::BeBoard* theBoard);
    ~D19cPSTestBoardFWInterface();

  public:
    // power functions
    void ReadPower_SSA(uint8_t mpaid = 0, uint8_t ssaid = 0);
    void PSInterfaceBoard_PowerOn(uint8_t mpaid = 0, uint8_t ssaid = 0);
    void PSInterfaceBoard_PowerOff();
    void PSInterfaceBoard_PowerOn_SSA(float VDDPST = 1.25, float DVDD = 1.0, float AVDD = 1.25, float VBF = 0.3, float BG = 0.0, uint8_t ENABLE = 145);
    void PSInterfaceBoard_PowerOff_SSA(uint8_t mpaid = 0, uint8_t ssaid = 0);
    void PSInterfaceBoard_PowerOn_MPASSA(float VDDPST = 1.25, float DVDD = 1.25, float AVDD = 1.25, float VBG = 0.3, float VBF = 0.3, uint8_t mpaid = 1, uint8_t ssaid = 0);
    void PSInterfaceBoard_PowerOn_MPA(float VDDPST = 1.25, float DVDD = 1.2, float AVDD = 1.23, float VBG = 0.3, uint8_t mpaid = 0, uint8_t ssaid = 0);
    void PSInterfaceBoard_PowerOff_MPA(uint8_t mpaid = 0, uint8_t ssaid = 0);
    void KillI2C();

  private:
    // wait
    uint32_t fWait_us{100};
    // I2C functions
    uint32_t PSInterfaceBoard_SendI2CCommand_READ(uint32_t slave_id, uint32_t board_id, uint32_t read, uint32_t register_address, uint32_t data);
    void     PSInterfaceBoard_SendI2CCommand(uint32_t slave_id, uint32_t board_id, uint32_t read, uint32_t register_address, uint32_t data);
    void     PSInterfaceBoard_ConfigureI2CMaster(uint32_t pEnabled = 1, uint32_t pFrequency = 4);
    void     PSInterfaceBoard_SetSlaveMap();

    // don't understand what this does
    // TO-DO - check if its needed
    // void SSAEqualizeDACs(uint8_t pChipId);
};
} // namespace Ph2_HwInterface
#endif
