/*!
  \file                  HistContainer.h
  \brief                 Header file of histogram container
  \author                Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef HistContainer_H
#define HistContainer_H

#include "RootUtils/PlotContainer.h"
#include "Utils/Container.h"

#include <iostream>

template <class Hist>
class HistContainer : public PlotContainer
{
  public:
    HistContainer() : fTheHistogram(nullptr) {}

    HistContainer(const HistContainer<Hist>& container)                  = delete;
    HistContainer<Hist>& operator=(const HistContainer<Hist>& container) = delete;

    template <class... Args>
    HistContainer(Args... args)
    {
        fTheHistogram = new Hist(args...);
        fTheHistogram->SetDirectory(0);
    }

    ~HistContainer()
    {
        if(fHasToBeDeletedManually) delete fTheHistogram;
        fTheHistogram = nullptr;
    }

    HistContainer(HistContainer<Hist>&& container)
    {
        fHasToBeDeletedManually = container.fHasToBeDeletedManually;
        fTheHistogram           = container.fTheHistogram;
        container.fTheHistogram = nullptr;
    }

    HistContainer<Hist>& operator=(HistContainer<Hist>&& container)
    {
        fHasToBeDeletedManually = container.fHasToBeDeletedManually;
        fTheHistogram           = container.fTheHistogram;
        container.fTheHistogram = nullptr;
        return *this;
    }

    void initialize(std::string name, std::string title, const PlotContainer* reference) override
    {
        fHasToBeDeletedManually = false;

        fTheHistogram = new Hist(*(static_cast<const HistContainer<Hist>*>(reference)->fTheHistogram));

        fTheHistogram->SetName(name.c_str());
        fTheHistogram->SetTitle(title.c_str());
    }

    void print(void) { std::cout << "HistContainer " << fTheHistogram->GetName() << std::endl; }

    void setNameTitle(std::string histogramName, std::string histogramTitle) override { fTheHistogram->SetNameTitle(histogramName.c_str(), histogramTitle.c_str()); }

    std::string getName() const override { return fTheHistogram->GetName(); }

    std::string getTitle() const override { return fTheHistogram->GetTitle(); }

    Hist* fTheHistogram;
};

#endif
