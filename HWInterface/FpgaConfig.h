/*!
   \file        FpgaConfig.h
   \brief       FPGA configuration by uploading the firware in MCS file format
   \version             1.0
   \author      Christian Bonnin
   \date        02/03/2015
   Support :        mail to : christian.bonnin@iphc.cnrs.fr
*/

#ifndef _FPGACONFIG_H_
#define _FPGACONFIG_H_

#include "HWInterface/RegManager.h"
#include <string>

namespace Ph2_HwInterface
{
/*!
 * \brief Upload MCS files into Flash EPROM as FPGA configuration
 * @author cbonnin
 */
class FpgaConfig
{
  public:
    /*! \brief Constructor from a RegManager
     * \param pbbi Reference to the RegManager
     */
    FpgaConfig(RegManager&& pbbi);
    virtual ~FpgaConfig() {};

    FpgaConfig(const FpgaConfig& theFpgaConfig) = delete;
    FpgaConfig(FpgaConfig&& theFpgaConfig)      = default;

    /*! \brief Launch the firmware download in a separate thread
     * \param strConfig FPGA configuration number or name
     * \param pstrFile absolute path to the configuration file
     */
    uint32_t getUploadingFpga() const { return numUploadingFpga; }
    /*! \brief Percentage of the upload already done
     * \return 0 to 100 percentage value
     */
    uint32_t getProgressValue() const { return progressValue; }
    /*! \brief Description of the upload current status
     * \return String describing the upload status containing the block number and the estimated remaining time
     */
    const std::string& getProgressString() const { return progressString; }

  protected:
    uint32_t    progressValue, numUploadingFpga;
    std::string progressString;
    RegManager  fwManager;
};
} // namespace Ph2_HwInterface
#endif
