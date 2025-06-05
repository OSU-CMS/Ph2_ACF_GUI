/*!
  Filename :                              Hybrid.cc
  Content :                               Hybrid Description class
  Programmer :                    Lorenzo BIDEGAIN
  Version :               1.0
  Date of Creation :              25/06/14
  Support :                               mail to : lorenzo.bidegain@gmail.com
*/

#include "Hybrid.h"

namespace Ph2_HwDescription
{
// Default C'tor
Hybrid::Hybrid() : FrontEndDescription(), HybridContainer(0) {}

Hybrid::Hybrid(const FrontEndDescription& pFeDesc, uint8_t pHybridId) : FrontEndDescription(pFeDesc), HybridContainer(pHybridId) {}

Hybrid::Hybrid(uint8_t pBeId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId) : FrontEndDescription(pBeId, pFMCId, pOpticalGroupId, pHybridId), HybridContainer(pHybridId) {}

} // namespace Ph2_HwDescription
