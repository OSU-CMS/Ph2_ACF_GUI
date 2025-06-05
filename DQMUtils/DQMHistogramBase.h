/*!
  \file                DQMHistogramBase.h
  \brief               base class to create and fill monitoring histograms
  \author              Fabio Ravera, Lorenzo Uplegger
  \version             1.0
  \date                6/5/19
  Support :            mail to : fabio.ravera@cern.ch

*/

#ifndef __DQMHISTOGRAMBASE_H__
#define __DQMHISTOGRAMBASE_H__

#include <boost/any.hpp>
#include <memory>
#include <string>
#include <vector>

#include "HWDescription/RD53.h"
#include "Parser/FileParser.h"
#include "RootUtils/CanvasContainer.h"
#include "RootUtils/HistContainer.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/RD53Shared.h"

#include <TCanvas.h>
#include <TFile.h>
#include <TGaxis.h>
#include <TPad.h>
#include <TStyle.h>

class DetectorDataContainer;
class DetectorContainer;

/*!
 * \class DQMHistogramBase
 * \brief Base class for monitoring histograms
 */

namespace user_detail
{
template <typename>
struct sfinae_true_DQMHistogramBase : std::true_type
{
};

template <typename T>
static auto test_SetZTitle(int) -> sfinae_true_DQMHistogramBase<decltype(std::declval<T>().SetZTitle(""))>;

template <typename>
static auto test_SetZTitle(long) -> std::false_type;
} // namespace user_detail

// SFINAE: check if object T has SetZTitle
template <typename T>
struct has_SetZTitle : decltype(user_detail::test_SetZTitle<T>(0))
{
};

// Functor for SetZTitle - default case
template <typename T, bool hasSetZTitle = false>
struct CallSetZTitle
{
    void operator()(T* thePlot, const char* theTitle) { return; }
};

// Functor for SetZTitle - case when SetZTitle is defined
template <typename T>
struct CallSetZTitle<T, true>
{
    void operator()(T* thePlot, const char* theTitle)
    {
        thePlot->SetZTitle(theTitle);
        return;
    }
};

class DQMHistogramBase
{
  public:
    /*!
     * constructor
     */
    DQMHistogramBase()
    {
        const int NRGBs = 5;
        const int NCont = 255;

        double stops[NRGBs] = {0.00, 0.34, 0.61, 0.84, 1.00};
        double red[NRGBs]   = {0.00, 0.00, 0.87, 1.00, 0.51};
        double green[NRGBs] = {0.00, 0.81, 1.00, 0.20, 0.00};
        double blue[NRGBs]  = {0.51, 1.00, 0.12, 0.00, 0.00};

        TColor::CreateGradientColorTable(NRGBs, stops, red, green, blue, NCont);
        gStyle->SetNumberContours(NCont);
    }

    /*!
     * destructor
     */
    virtual ~DQMHistogramBase() {}

    /*!
     * \brief Book histograms
     * \param theDetectorStructure : Container of the Detector structure
     */
    virtual void book(TFile* outputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap) = 0;

    /*!
     * \brief Book histograms
     * \param configurationFileName : xml configuration file
     */
    virtual bool fill(std::string& inputStream) = 0;

    /*!
     * \brief SAve histograms
     * \param outFile : ouput file name
     */
    virtual void process() = 0;

    /*!
     * \brief Book histograms
     * \param configurationFileName : xml configuration file
     */
    virtual void reset(void) = 0;

  private:
    std::vector<std::unique_ptr<TGaxis>> axes;

  protected:
    template <typename Hist>
    void bookChipImplementer(TFile*                       theOutputFile,
                             const DetectorContainer&     theDetectorStructure,
                             DetectorDataContainer&       dataContainer,
                             const CanvasContainer<Hist>& histContainer,
                             const char*                  XTitle = nullptr,
                             const char*                  YTitle = nullptr,
                             const char*                  ZTitle = nullptr)
    {
        if(XTitle != nullptr) histContainer.fTheHistogram->GetXaxis()->SetTitle(XTitle);
        if(YTitle != nullptr) histContainer.fTheHistogram->GetYaxis()->SetTitle(YTitle);
        if(ZTitle != nullptr)
        {
            CallSetZTitle<Hist, has_SetDirectory<Hist>::value> setZTitleFunctor;
            setZTitleFunctor(histContainer.fTheHistogram, ZTitle);
        }

        dataContainer.reset();
        RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, dataContainer, histContainer);
    }

    template <typename Hist>
    void bookOpticalGroupImplementer(TFile*                       theOutputFile,
                                     const DetectorContainer&     theDetectorStructure,
                                     DetectorDataContainer&       dataContainer,
                                     const CanvasContainer<Hist>& histContainer,
                                     const char*                  XTitle = nullptr,
                                     const char*                  YTitle = nullptr,
                                     const char*                  ZTitle = nullptr)
    {
        if(XTitle != nullptr) histContainer.fTheHistogram->GetXaxis()->SetTitle(XTitle);
        if(YTitle != nullptr) histContainer.fTheHistogram->GetYaxis()->SetTitle(YTitle);
        if(ZTitle != nullptr)
        {
            CallSetZTitle<Hist, has_SetDirectory<Hist>::value> setZTitleFunctor;
            setZTitleFunctor(histContainer.fTheHistogram, ZTitle);
        }

        dataContainer.reset();
        RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, dataContainer, histContainer);
    }

    template <typename Hist>
    void drawChip(DetectorDataContainer& HistDataContainer, const std::string& opt = "", const std::string& additionalAxisType = "", const char* additionalAxisTitle = "", bool isNoise = false)
    {
        for(auto cBoard: HistDataContainer)
            for(auto cOpticalGroup: *cBoard)
                for(auto cHybrid: *cOpticalGroup)
                    for(auto cChip: *cHybrid)
                    {
                        auto canvas = cChip->getSummary<CanvasContainer<Hist>>().fCanvas;
                        auto hist   = cChip->getSummary<CanvasContainer<Hist>>().fTheHistogram;

                        canvas->cd();
                        if(opt.find("logz") != std::string::npos) canvas->SetLogz();
                        hist->Draw(opt.c_str());
                        canvas->Modified();
                        canvas->Update();

                        if(additionalAxisType != "")
                        {
                            auto myPad = static_cast<TPad*>(canvas->GetPad(0));
                            myPad->SetTopMargin(0.16);

                            if(additionalAxisType == "electron")
                                axes.emplace_back(new TGaxis(myPad->GetUxmin(),
                                                             myPad->GetUymax(),
                                                             myPad->GetUxmax(),
                                                             myPad->GetUymax(),
                                                             RD53Shared::firstChip->VCal2Charge(hist->GetXaxis()->GetBinLowEdge(1), isNoise),
                                                             RD53Shared::firstChip->VCal2Charge(hist->GetXaxis()->GetBinLowEdge(hist->GetXaxis()->GetNbins()), isNoise),
                                                             510,
                                                             "-"));

                            axes.back()->SetTitle(additionalAxisTitle);
                            axes.back()->SetTitleOffset(1.2);
                            axes.back()->SetTitleSize(0.035);
                            axes.back()->SetTitleFont(40);
                            axes.back()->SetLabelOffset(0.001);
                            axes.back()->SetLabelSize(0.035);
                            axes.back()->SetLabelFont(42);
                            axes.back()->SetLabelColor(kRed);
                            axes.back()->SetLineColor(kRed);
                            axes.back()->Draw();

                            canvas->Modified();
                            canvas->Update();
                        }
                    }
    }

    template <typename Hist>
    void drawOpticalGroup(DetectorDataContainer& HistDataContainer, const std::string& opt = "", const std::string& additionalAxisType = "", const char* additionalAxisTitle = "", bool isNoise = false)
    {
        for(auto cBoard: HistDataContainer)
            for(auto cOpticalGroup: *cBoard)
            {
                auto canvas = cOpticalGroup->getSummary<CanvasContainer<Hist>>().fCanvas;
                auto hist   = cOpticalGroup->getSummary<CanvasContainer<Hist>>().fTheHistogram;

                canvas->cd();
                if(opt.find("logz") != std::string::npos) canvas->SetLogz();
                hist->Draw(opt.c_str());
                canvas->Modified();
                canvas->Update();

                if(additionalAxisType != "")
                {
                    auto myPad = static_cast<TPad*>(canvas->GetPad(0));
                    myPad->SetTopMargin(0.16);

                    if(additionalAxisType == "electron")
                        axes.emplace_back(new TGaxis(myPad->GetUxmin(),
                                                     myPad->GetUymax(),
                                                     myPad->GetUxmax(),
                                                     myPad->GetUymax(),
                                                     RD53Shared::firstChip->VCal2Charge(hist->GetXaxis()->GetBinLowEdge(1), isNoise),
                                                     RD53Shared::firstChip->VCal2Charge(hist->GetXaxis()->GetBinLowEdge(hist->GetXaxis()->GetNbins()), isNoise),
                                                     510,
                                                     "-"));

                    axes.back()->SetTitle(additionalAxisTitle);
                    axes.back()->SetTitleOffset(1.2);
                    axes.back()->SetTitleSize(0.035);
                    axes.back()->SetTitleFont(40);
                    axes.back()->SetLabelOffset(0.001);
                    axes.back()->SetLabelSize(0.035);
                    axes.back()->SetLabelFont(42);
                    axes.back()->SetLabelColor(kRed);
                    axes.back()->SetLineColor(kRed);
                    axes.back()->Draw();

                    canvas->Modified();
                    canvas->Update();
                }
            }
    }

    template <typename T>
    T findValueInSettings(const Ph2_Parser::SettingsMap& settingsMap, const std::string name, T defaultValue = T()) const
    {
        auto setting = settingsMap.find(name);
        return (setting != std::end(settingsMap) ? boost::any_cast<T>(setting->second) : defaultValue);
    }
};

#endif
