#ifndef _D19cMuxBackplaneFWInterface_H__
#define __D19cMuxBackplaneFWInterface_H__

#include "HWInterface/BeBoardFWInterface.h"
#include <string>

namespace Ph2_HwDescription
{
class BeBoard;
}
namespace Ph2_HwInterface
{
class D19cMuxBackplaneFWInterface : public BeBoardFWInterface
{
  public:
    D19cMuxBackplaneFWInterface(const char* puHalConfigFileName, uint32_t pBoardId, FileHandler* pFileHandler, Ph2_HwDescription::BeBoard* theBoard);
    D19cMuxBackplaneFWInterface(const char* pId, const char* pUri, const char* pAddressTable, FileHandler* pFileHandler, Ph2_HwDescription::BeBoard* theBoard);
    ~D19cMuxBackplaneFWInterface();

  public:
    void     DisconnectMultiplexingSetup(uint8_t pWait_ms = 100);
    uint32_t ScanMultiplexingSetup(uint8_t pWait_ms = 100);
    void     ConfigureMultiplexingSetup(int BackplaneNum, int CardNum, bool InterlockEnabled, uint8_t pWait_ms = 100);

  private:
};
} // namespace Ph2_HwInterface
#endif
