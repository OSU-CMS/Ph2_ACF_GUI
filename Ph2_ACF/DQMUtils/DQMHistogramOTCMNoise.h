/*!
        \file                DQMHistogramOTCMNoise.h
        \brief               DQM class for OTCMNoise
        \author              Lesya Horyn, Martin Delcourt
        \date                17/02/22
*/

#ifndef DQMHistogramOTCMNoise_h_
#define DQMHistogramOTCMNoise_h_
#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/Container.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/DataContainer.h"

class TFile;

/*!
 * \class DQMHistogramOTCMNoise
 * \brief Class for OTCMNoise monitoring histograms
 */
class DQMHistogramOTCMNoise : public DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramOTCMNoise();

    /*!
     * destructor
     */
    ~DQMHistogramOTCMNoise();

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

    // Fill correlation between top & bottom sensors, split by detector structure
    void fillSensorChipCorrelationPlots(DetectorDataContainer& theHitData, float threshold);
    void fillSensorHybridCorrelationPlots(DetectorDataContainer& theHitData, float threshold);
    void fillSensorModuleCorrelationPlots(DetectorDataContainer& theHitData, float threshold);

    void fill2DHitPlots(DetectorDataContainer& theHitData, float threshold);
    void fill2DHitLightPlots(DetectorDataContainer& theHitData, float threshold);
    void fillHybridCorrelationPlots(DetectorDataContainer& theHybridData, float threshold);
    void fillChipCorrelationPlots(DetectorDataContainer& theHybridData, float threshold);
    void fillHitProfile(DetectorDataContainer& theHitData, float threshold); // Not used at the moment

    // Fill number of hits distribution, split by detector structure
    void fillChipHitPlots(DetectorDataContainer& theHitData, bool pFitDistributions, float threshold);
    void fillChipHitPlots(DetectorDataContainer& theHitData, float threshold);
    void fillHybridHitPlots(DetectorDataContainer& theHitData, float threshold);
    void fillModuleHitPlots(DetectorDataContainer& theHitData, float threshold);

    template <typename T1, typename T2, typename T3, typename T4>
    bool processInputStream(std::string streamName, std::string& inputStream, void (DQMHistogramOTCMNoise::*function)(DetectorDataContainer&, float))
    {
        ContainerSerialization theSerializer(streamName);
        try
        {
            if(theSerializer.attachDeserializer(inputStream))
            {
                // LOG(INFO) << "Matched stream " << streamName << "!" << RESET;
                float                 fThreshold    = 0;
                DetectorDataContainer fDetectorData = theSerializer.deserializeOpticalGroupContainer<T1, T2, T3, T4>(fDetectorContainer, fThreshold);
                (this->*function)(fDetectorData, fThreshold);
                return true;
            }
        }
        catch(const std::exception& e) // reference to the base of a polymorphic object
        {
            LOG(ERROR) << ERROR_FORMAT << " Unable to read stream " << streamName << ": " << e.what() << RESET;
        }
        return false;
    }

    template <typename T1, typename T2, typename T3, typename T4>
    bool processInputStreamChip(std::string streamName, std::string& inputStream, void (DQMHistogramOTCMNoise::*function)(DetectorDataContainer&, float))
    {
        ContainerSerialization theSerializer(streamName);
        try
        {
            if(theSerializer.attachDeserializer(inputStream))
            {
                LOG(INFO) << "Matched stream " << streamName << "!" << RESET;
                float                 fThreshold    = 0;
                DetectorDataContainer fDetectorData = theSerializer.deserializeChipContainer<T1, T2>(fDetectorContainer, fThreshold);
                (this->*function)(fDetectorData, fThreshold);
                return true;
            }
        }
        catch(const std::exception& e)
        {
            LOG(INFO) << BOLDRED << " Unable to read chip-level stream " << streamName << ": " << e.what() << RESET;
        }
        return false;
    }

    /*!
     * \brief process : do something with the histogram like colors, fit, drawing canvases, etc
     */
    void process() override;

    /*!
     * \brief Reset histogram
     */
    void reset(void) override;

  private:
    DetectorContainer*                     fDetectorContainer;
    std::map<float, DetectorDataContainer> fChipHitHistograms;
    std::map<float, DetectorDataContainer> fChipHitHistogramsBottom;
    std::map<float, DetectorDataContainer> fChipHitHistogramsTop;
    std::map<float, DetectorDataContainer> fHybridHitHistograms;
    std::map<float, DetectorDataContainer> fHybridHitHistogramsBottom;
    std::map<float, DetectorDataContainer> fHybridHitHistogramsTop;
    std::map<float, DetectorDataContainer> fModuleHitHistograms;
    std::map<float, DetectorDataContainer> fModuleHitHistogramsBottom;
    std::map<float, DetectorDataContainer> fModuleHitHistogramsTop;
    std::map<float, DetectorDataContainer> f2DChipHitHistograms;
    std::map<float, DetectorDataContainer> f2DHybridHitHistograms;
    std::map<float, DetectorDataContainer> f2DModuleHitHistograms;
    std::map<float, DetectorDataContainer> f2DModuleHitHistogramsLight;
    std::map<float, DetectorDataContainer> f2DModuleHitHistogramsBottom;
    std::map<float, DetectorDataContainer> f2DModuleHitHistogramsTop;
    std::map<float, DetectorDataContainer> f2DHybridHitHistograms_chip;
    std::map<float, DetectorDataContainer> f2DModuleHitHistograms_chip;
    std::map<float, DetectorDataContainer> f2DModuleHitHistogramsBottom_chip;
    std::map<float, DetectorDataContainer> f2DModuleHitHistogramsTop_chip;
    std::map<float, DetectorDataContainer> f2DModuleSensorCorrelation;
    std::map<float, DetectorDataContainer> f2DHybridSensorCorrelation;
    std::map<float, DetectorDataContainer> f2DChipSensorCorrelation;
    std::map<float, DetectorDataContainer> f2DHybridCorrelation;
    std::map<float, DetectorDataContainer> f2DChipCorrelation;

    uint32_t           fNevents;
    bool               f2DHistograms;
    bool               f2DHistogramsLight;
    std::vector<float> fListOfThresholds{0};

    // fitting function
    void   fitCMNoise(TH1F* pHitCountHist, TF1* pFit, uint32_t pRange);
    double findMaximum(TH1F* pHistogram);
    double inverse_hitProbability(double probability);
};
#endif
