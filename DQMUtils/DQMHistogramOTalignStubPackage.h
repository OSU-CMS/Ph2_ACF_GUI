/*!
        \file                DQMHistogramOTalignStubPackage.h
        \brief               DQM class for OTalignStubPackage
        \author              Fabio Ravera
        \date                26/01/24
*/

#ifndef DQMHistogramOTalignStubPackage_h_
#define DQMHistogramOTalignStubPackage_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTalignStubPackage
 * \brief Class for OTalignStubPackage monitoring histograms
 */
class DQMHistogramOTalignStubPackage : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTalignStubPackage();

    /*!
     * destructor
     */
    ~DQMHistogramOTalignStubPackage();

    /*!
     * \brief Book histograms
     * \param theOutputFile : where histograms will be saved
     * \param theDetectorStructure : Detector container as obtained after file parsing, used to create histograms for
     * all board/chip/hybrid/channel \param pSettingsMap : setting as for Tool setting map in case coe informations are
     * needed (i.e. FitSCurve)
     */
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap) override;

    /*!
     * \brief fill : fill histograms from TCP stream, need to be overwritten to avoid compilation errors, but it is not
     * needed if you do not fo into the SoC \param dataBuffer : vector of char with the TCP datastream
     */
    bool fill(std::string& inputStream) override;

    /*!
     * \brief process : do something with the histogram like colors, fit, drawing canvases, etc
     */
    void process() override;

    /*!
     * \brief Reset histogram
     */
    void reset(void) override;

    void fillBestStubPackageDelay(DetectorDataContainer& theBestStubPackageDelayContainer);

  private:
    DetectorContainer*    fDetectorContainer;
    DetectorDataContainer fBestStubPackageDelayHistogramContainer;
};
#endif
