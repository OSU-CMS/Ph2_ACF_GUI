/*

        \file                          PlotContainer.h
        \brief                         Generic PlotContainer for DQM
        \author                        Fabio Ravera, Lorenzo Uplegger
        \version                       1.0
        \date                          18/06/19
        Support :                      mail to : fabio.ravera@cern.ch

 */

#ifndef __PLOT_CONTAINER_H__
#define __PLOT_CONTAINER_H__

#include <iostream>

class PlotContainer //: public streammable
{
  public:
    PlotContainer() { ; }
    PlotContainer(const PlotContainer& container)            = delete;
    PlotContainer& operator=(const PlotContainer& container) = delete;
    PlotContainer(PlotContainer&& container)                 = default;
    PlotContainer& operator=(PlotContainer&& container)      = default;

    virtual ~PlotContainer() { ; }

    virtual void        setNameTitle(std::string histogramName, std::string histogramTitle)             = 0;
    virtual std::string getName() const                                                                 = 0;
    virtual std::string getTitle() const                                                                = 0;
    virtual void        initialize(std::string name, std::string title, const PlotContainer* reference) = 0;

  protected:
    bool fHasToBeDeletedManually{true}; // if associated to a File, ROOT takes the pointer and destroys it :-(
};

#endif
