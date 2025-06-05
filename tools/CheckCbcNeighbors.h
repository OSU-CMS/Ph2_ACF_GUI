/*!
 *
 * \file CheckCbcNeighbors.h
 * \brief CheckCbcNeighbors class
 * \author Lesya Horyn
 * \date 11/07/22
 *
 */

#ifndef CheckCbcNeighbors_h__
#define CheckCbcNeighbors_h__

#include "tools/Tool.h"
#include <map>
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here

class CheckCbcNeighbors : public Tool
{
  public:
    CheckCbcNeighbors();
    ~CheckCbcNeighbors();

    void Initialise(void);

    // State machine
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();
    bool TestCbcNeighbors();
    // returns number of stubs
    bool CheckStubs(uint8_t hybridId, uint8_t chipId);

  private:
    //
    uint32_t fNEvents;

    const std::vector<uint8_t> fSharedTopHigh{250, 252};
    const std::vector<uint8_t> fSharedBottomLow{1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25};

    const std::vector<uint8_t> fSharedBottomHigh{233, 235, 235, 237, 239, 241, 243, 245, 247, 249, 251, 253};
    const std::vector<uint8_t> fSharedTopLow{0, 2, 4};

    void UnmaskChannels(std::vector<uint8_t> pToUnmask, ChannelGroup<1, NCHANNELS>& pChannelMask);
};
#endif
