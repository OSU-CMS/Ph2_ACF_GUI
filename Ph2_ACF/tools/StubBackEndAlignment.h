/*!
 *
 * \file BackEndAlignment.h
 * \brief CIC FE alignment class, automated alignment procedure for CICs
 * connected to FEs
 * \author Sarah SEIF EL NASR-STOREY
 * \date 28 / 06 / 19
 *
 * \Support : sarah.storey@cern.ch
 *
 */

#ifndef StubBackEndAlignment_h__
#define StubBackEndAlignment_h__

#include "tools/OTTool.h"

#include <map>
const uint8_t FAILED_STUBBACKEND_ALIGNMENT = 100;
class StubBackEndAlignment : public OTTool
{
  public:
    StubBackEndAlignment();
    ~StubBackEndAlignment();

    void Initialise();
    bool Align();
    bool FindStubLatency();
    bool FindPackageDelay();
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;

    // get alignment results
    bool getStatus() const { return fSuccess; }

  protected:
  private:
    bool FindStubLatency(Ph2_HwDescription::BeBoard* pBoard);
    bool FindPackageDelay(Ph2_HwDescription::BeBoard* pBoard);
};
#endif
