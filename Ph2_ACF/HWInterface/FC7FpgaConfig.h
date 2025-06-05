/*!
   \file        FC7FpgaConfig.h
   \brief       FPGA configuration by uploading the firware in MCS file format into a GLIB board
   \version             1.0
   \author      Christian Bonnin
   \date        02/03/2015
   Support :        mail to : christian.bonnin@iphc.cnrs.fr
*/
#ifndef _FC7FpgaConfig_H_
#define _FC7FpgaConfig_H_

#include "HWInterface/FpgaConfig.h"
#include <vector>

namespace Ph2_HwInterface
{
class RegManager;
}
namespace fc7
{
class MmcPipeInterface;
}

namespace Ph2_HwInterface
{
/*!
 * \brief Upload MCS files into Flash EPROM as FPGA configuration
 * @author cbonnin
 */
class FC7FpgaConfig : public FpgaConfig
{
  private:
    fc7::MmcPipeInterface* lNode;

  public:
    FC7FpgaConfig(Ph2_HwInterface::RegManager&& pbbi);
    ~FC7FpgaConfig();
    FC7FpgaConfig(const FC7FpgaConfig&) = delete;
    FC7FpgaConfig(FC7FpgaConfig&& theFC7FpgaConfig);

    /*! \brief Launch the firmware upload in a separate thread
     * \param strConfig FPGA configuration name
     * \param pstrFile absolute path to the .bit or .bin file
     */
    void flashProm(const std::string& strConfig, const std::string& pstrFile);

    /*! \brief Launch the firmware download in a separate thread
     * \param strConfig FPGA configuration name
     * \param pstrFile absolute path to the .bin file
     */
    void downloadFpgaConfig(const std::string& strConfig, const std::string& pstrFile);

    /*! \brief Jump to an FPGA configuration
     * \param strConfig FPGA configuration name
     */
    void jumpToFpgaConfig(const std::string& strImage);

    void downloadImage(const std::string& strImage, const std::string& strDestFile);
    /*! \brief Get the list of available FPGA configuration (or firmware images)*/
    std::vector<std::string> getFpgaConfigList();
    /*! \brief Delete one Fpga configuration (or firmware image)*/
    void deleteFpgaConfig(const std::string& strId);
    /*! \brief Board hard reset */
    void rebootBoard();

  private:
    /// Sets the read mode as asynchronous.
    void confAsyncRead();
    /// Locks or unlocks a block of the flash (Xilinx DS617(v3.0.1) page 75, figure 43).
    void blockLockOrUnlock(uint32_t block_number, char operation);
    /// Erases a block of the flash (Xilinx DS617(v3.0.1) page 73, figure 41).
    void blockErase(uint32_t block_number);
    /// Writes up to 32 words to the flash (Xilinx DS617(v3.0.1) page 71, figure 39).
    void bufferProgram(uint32_t block_number, uint32_t data_address, std::vector<uint32_t>& write_buffer, uint32_t words);
    /*! \brief Main uploading loop
     * \param pstrFile Absolute path the .bit configuration file
     */
    void dumpFromFileIntoSD(const std::string& strImage, const std::string& pstrFile);
    void verifyImageName(const std::string& strImage);

    void checkIfUploading();
};
} // namespace Ph2_HwInterface
#endif
