/*!
        \file                DQMHistogramSignalScanFit.h
        \brief               base class to create and fill monitoring histograms
        \author              Fabio Ravera, Lorenzo Uplegger
        \version             1.0
        \date                6/5/19
        Support :            mail to : fabio.ravera@cern.ch
 */

#include "DQMUtils/DQMHistogramSignalScanFit.h"
#include "RootUtils/RootContainerFactory.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"

#include "Utils/EmptyContainer.h"
#include "Utils/Occupancy.h"
#include "Utils/ThresholdAndNoise.h"
#include "Utils/Utilities.h"

//========================================================================================================================
DQMHistogramSignalScanFit::DQMHistogramSignalScanFit() {}

//========================================================================================================================
DQMHistogramSignalScanFit::~DQMHistogramSignalScanFit() {}

//========================================================================================================================
void DQMHistogramSignalScanFit::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    ContainerFactory::copyStructure(theDetectorStructure, fDetectorData);
}

//========================================================================================================================
bool DQMHistogramSignalScanFit::fill(std::string& inputStream) { return false; }

//========================================================================================================================
void DQMHistogramSignalScanFit::process() {}

//========================================================================================================================

void DQMHistogramSignalScanFit::reset(void) {}
