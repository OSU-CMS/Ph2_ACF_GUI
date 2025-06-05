#ifndef CONTAINER_RECICLE_BIN_H
#define CONTAINER_RECICLE_BIN_H

#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/DataContainer.h"

template <typename... Ts>
using TestType = decltype(ContainerFactory::copyAndInitStructure(std::declval<const DetectorContainer&>(), std::declval<DetectorDataContainer&>(), std::declval<Ts&>()...))(const DetectorContainer&,
                                                                                                                                                                            DetectorDataContainer&,
                                                                                                                                                                            Ts&...);

template <typename... Args>
class ContainerRecycleBin
{
  public:
    ContainerRecycleBin() { ; }

    ContainerRecycleBin(const ContainerRecycleBin&) = delete;
    ContainerRecycleBin(ContainerRecycleBin&&)      = delete;

    ContainerRecycleBin& operator=(const ContainerRecycleBin&) = delete;
    ContainerRecycleBin& operator=(ContainerRecycleBin&&)      = delete;

    ~ContainerRecycleBin() { clean(); }

    void clean()
    {
        for(auto container: fRecycleBin)
        {
            delete container;
            container = nullptr;
        }
        fRecycleBin.clear();
    }

    void setDetectorContainer(DetectorContainer* theDetector) { fDetectorContainer = theDetector; }

    DetectorDataContainer* get(TestType<Args...> function, Args... theInitArguments)
    {
        if(fRecycleBin.size() == 0)
        {
            DetectorDataContainer* theDataContainer = new DetectorDataContainer();
            function(*fDetectorContainer, *theDataContainer, theInitArguments...);
            return theDataContainer;
        }
        else
        {
            DetectorDataContainer* availableContainer = fRecycleBin.back();
            fRecycleBin.pop_back();
            ContainerFactory::reinitializeContainer<Args...>(*availableContainer, theInitArguments...);
            return availableContainer;
        }
    }

    void free(DetectorDataContainer* theDataContainer) { fRecycleBin.push_back(theDataContainer); }

  private:
    std::vector<DetectorDataContainer*> fRecycleBin;
    DetectorContainer*                  fDetectorContainer;
};

#endif