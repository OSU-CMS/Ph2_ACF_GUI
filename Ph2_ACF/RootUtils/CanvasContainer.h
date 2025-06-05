/*!
  \file                  CanvasContainer.h
  \brief                 Header file of canvas container
  \author                Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef CanvasContainer_H
#define CanvasContainer_H

#include "PlotContainer.h"
#include "Utils/Container.h"

#include <TCanvas.h>
#include <TDirectory.h>

#include <iostream>

namespace user_detail
{
template <typename>
struct sfinae_true_CanvasContainer : std::true_type
{
};

template <typename T>
static auto test_SetDirectory(int) -> sfinae_true_CanvasContainer<decltype(std::declval<T>().SetDirectory(0))>;
template <typename>
static auto test_SetDirectory(long) -> std::false_type;
} // namespace user_detail

// SFINAE: check if object T has SetDirectory
template <typename T>
struct has_SetDirectory : decltype(user_detail::test_SetDirectory<T>(0))
{
};

// Functor for SetDirectory - default case
template <typename T, bool hasSetDirectory = false>
struct CallSetDirectory
{
    void operator()(T* thePlot) { return; }
};

// Functor for SetDirectory - case when SetDirectory is defined
template <typename T>
struct CallSetDirectory<T, true>
{
    void operator()(T* thePlot) { thePlot->SetDirectory(0); }
};

template <class Hist>
class CanvasContainer : public PlotContainer
{
  public:
    CanvasContainer() : fTheHistogram(nullptr), fCanvas(nullptr) {}

    CanvasContainer(const CanvasContainer<Hist>& container)                  = delete;
    CanvasContainer<Hist>& operator=(const CanvasContainer<Hist>& container) = delete;

    template <class... Args>
    CanvasContainer(Args... args)
    {
        fTheHistogram = new Hist(args...);
        CallSetDirectory<Hist, has_SetDirectory<Hist>::value> setDirectoryFunctor;
        setDirectoryFunctor(fTheHistogram);
        fCanvas = nullptr;
    }

    ~CanvasContainer()
    {
        if(fHasToBeDeletedManually == true)
        {
            delete fTheHistogram;
            if(fCanvas != nullptr) delete fCanvas;
        }

        fTheHistogram = nullptr;
        fCanvas       = nullptr;
    }

    CanvasContainer(CanvasContainer<Hist>&& container)
    {
        fHasToBeDeletedManually = container.fHasToBeDeletedManually;
        fTheHistogram           = container.fTheHistogram;
        container.fTheHistogram = nullptr;
        fCanvas                 = container.fCanvas;
        container.fCanvas       = nullptr;
    }

    CanvasContainer<Hist>& operator=(CanvasContainer<Hist>&& container)
    {
        fHasToBeDeletedManually = container.fHasToBeDeletedManually;
        fTheHistogram           = container.fTheHistogram;
        container.fTheHistogram = nullptr;
        fCanvas                 = container.fCanvas;
        container.fCanvas       = nullptr;
        return *this;
    }

    void initialize(std::string name, std::string title, const PlotContainer* reference) override
    {
        fHasToBeDeletedManually = false;

        fCanvas = new TCanvas(name.c_str(), title.c_str());

        fTheHistogram = new Hist(*(static_cast<const CanvasContainer<Hist>*>(reference)->fTheHistogram));
        fTheHistogram->SetName(name.c_str());
        fTheHistogram->SetTitle(title.c_str());
        CallSetDirectory<Hist, has_SetDirectory<Hist>::value> setDirectoryFunctor;
        setDirectoryFunctor(fTheHistogram);

        gDirectory->Append(fCanvas);
    }

    void print(void) { std::cout << "CanvasContainer " << fTheHistogram->GetName() << std::endl; }

    void setNameTitle(std::string histogramName, std::string histogramTitle) override { fTheHistogram->SetNameTitle(histogramName.c_str(), histogramTitle.c_str()); }

    std::string getName() const override { return fTheHistogram->GetName(); }

    std::string getTitle() const override { return fTheHistogram->GetTitle(); }

    Hist*    fTheHistogram;
    TCanvas* fCanvas;
};

#endif
