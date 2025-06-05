/*!
  \file                  MetadataHandlerIT.h
  \brief                 Header file of Metadata Handler for IT
  \author                Mauro DINARDO
  \version               1.0
  \date                  09/03/23
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef MetadataHandlerIT_H
#define MetadataHandlerIT_H

#include "Utils/ContainerSerialization.h"
#include "Utils/RD53Shared.h"
#include "tools/MetadataHandler.h"

#ifdef __USE_ROOT__
#include "DQMUtils/DQMMetadataIT.h"
#endif

class MetadataHandlerIT : public MetadataHandler
{
  public:
    MetadataHandlerIT(std::string startOfTestTime);
    ~MetadataHandlerIT();

    void initMetadataHardwareSpecific() override;
    void fillInitialConditionsHardwareSpecific() override;
    void fillFinalConditionsHardwareSpecific() override;

  private:
    void fillBeginOfCalib(DetectorDataContainer& theEndOfCalibContainer);
    void fillEndOfCalib(DetectorDataContainer& theEndOfCalibContainer);
};

#endif
