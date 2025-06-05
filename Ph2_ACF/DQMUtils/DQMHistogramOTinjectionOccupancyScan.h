/*!
        \file                DQMHistogramOTinjectionOccupancyScan.h
        \brief               DQM class for OTinjectionOccupancyScan
        \author              Fabio Ravera
        \date                15/05/24
*/

#ifndef DQMHistogramOTinjectionOccupancyScan_h_
#define DQMHistogramOTinjectionOccupancyScan_h_
#include "DQMUtils/DQMHistogramOTMeasureOccupancy.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTinjectionOccupancyScan
 * \brief Class for OTinjectionOccupancyScan monitoring histograms
 */
class DQMHistogramOTinjectionOccupancyScan : public DQMHistogramOTMeasureOccupancy
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTinjectionOccupancyScan();

    /*!
     * destructor
     */
    ~DQMHistogramOTinjectionOccupancyScan();

    /*!
     * \brief Book histograms
     * \param theOutputFile : where histograms will be saved
     * \param theDetectorStructure : Detector container as obtained after file parsing, used to create histograms for
     * all board/chip/hybrid/channel \param pSettingsMap : setting as for Tool setting map in case coe informations are
     * needed (i.e. FitSCurve)
     */
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap) override;
};
#endif
