/*!
  \file                           FrontEndDescription.cc
  \brief                          FrontEndDescription base class to describe all parameters common to all FE Components
  in the DAQ chain \author                         Lorenzo BIDEGAIN \version                        1.0 \date 25/06/14
  Support :                       mail to : lorenzo.bidegain@gmail.com
*/

#include "FrontEndDescription.h"

namespace Ph2_HwDescription
{
FrontEndDescription::FrontEndDescription(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, bool pStatus, FrontEndType pType)
    : fBeBoardId(pBeBoardId), fFMCId(pFMCId), fOpticalGroupId(pOpticalGroupId), fHybridId(pHybridId), fStatus(pStatus), fType(pType)
{
}

FrontEndDescription::FrontEndDescription() : fBeBoardId(0), fFMCId(0), fOpticalGroupId(0), fHybridId(0), fStatus(true), fType(FrontEndType::UNDEFINED) {}

FrontEndDescription::FrontEndDescription(const FrontEndDescription& pFeDesc)
    : fBeBoardId(pFeDesc.fBeBoardId), fFMCId(pFeDesc.fFMCId), fOpticalGroupId(pFeDesc.fOpticalGroupId), fHybridId(pFeDesc.fHybridId), fStatus(pFeDesc.fStatus), fType(pFeDesc.fType)
{
}

FrontEndDescription::~FrontEndDescription() {}
} // namespace Ph2_HwDescription
