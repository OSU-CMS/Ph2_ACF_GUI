/*!
  \file                  GraphContainer.h
  \brief                 Header file of Graphogram container
  \author                Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef GraphContainer_H
#define GraphContainer_H

#include "RootUtils/PlotContainer.h"
#include "Utils/Container.h"
#include <TDirectory.h>

#include <iostream>

template <class Graph>
class GraphContainer : public PlotContainer
{
  public:
    GraphContainer() : fTheGraph(nullptr) {}

    GraphContainer(const GraphContainer<Graph>& container)                   = delete;
    GraphContainer<Graph>& operator=(const GraphContainer<Graph>& container) = delete;

    template <class... Args>
    GraphContainer(Args... args)
    {
        fTheGraph = new Graph(args...);
    }

    ~GraphContainer()
    {
        if(fHasToBeDeletedManually) delete fTheGraph;
        fTheGraph = nullptr;
    }

    GraphContainer(GraphContainer<Graph>&& container)
    {
        fHasToBeDeletedManually = container.fHasToBeDeletedManually;
        fTheGraph               = container.fTheGraph;
        container.fTheGraph     = nullptr;
    }

    GraphContainer<Graph>& operator=(GraphContainer<Graph>&& container)
    {
        fHasToBeDeletedManually = container.fHasToBeDeletedManually;
        fTheGraph               = container.fTheGraph;
        container.fTheGraph     = nullptr;
        return *this;
    }

    void initialize(std::string name, std::string title, const PlotContainer* reference) override
    {
        fHasToBeDeletedManually = false;

        fTheGraph = new Graph(*(static_cast<const GraphContainer<Graph>*>(reference)->fTheGraph));

        fTheGraph->SetName(name.c_str());
        fTheGraph->SetTitle(title.c_str());

        gDirectory->Append(fTheGraph);
    }

    void print(void) { std::cout << "GraphContainer " << fTheGraph->GetName() << std::endl; }

    void setNameTitle(std::string GraphogramName, std::string GraphogramTitle) override { fTheGraph->SetNameTitle(GraphogramName.c_str(), GraphogramTitle.c_str()); }

    std::string getName() const override { return fTheGraph->GetName(); }

    std::string getTitle() const override { return fTheGraph->GetTitle(); }

    Graph* fTheGraph;
};

#endif
