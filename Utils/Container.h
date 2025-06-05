/*
  \file                  Container.h
  \brief                 Containers for DAQ
  \author                Fabio Ravera, Lorenzo Uplegger
  \version               1.0
  \date                  08/04/19
  Support:               email to fabio.ravera@cern.ch
*/

#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include "Utils/ChannelGroupHandler.h"
#include "Utils/Exception.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/vector.hpp>
#include <functional>
#include <iostream>
#include <map>
#include <typeinfo>
#include <vector>

class ChannelContainerBase;
template <typename T>
class ChannelContainer;
class ChipContainer;
class CalibBase;

class BaseContainer
{
  public:
    BaseContainer(uint16_t id = -1) : id_(id), isEnabled_(true) {}

    BaseContainer(const BaseContainer&) = delete;
    BaseContainer(BaseContainer&& theCopyContainer)
    {
        id_        = theCopyContainer.id_;
        isEnabled_ = theCopyContainer.isEnabled_;
    }

    virtual ~BaseContainer() {}
    uint16_t                     getId(void) const { return id_; }
    virtual void                 cleanDataStored(void)            = 0;
    virtual const BaseContainer* getElement(uint16_t theId) const = 0;
    bool                         isEnabled() const { return isEnabled_; }
    void                         setEnabled(bool enable) { isEnabled_ = enable; }
    virtual void                 setEnabledAll(bool enable) = 0;

  protected:
    virtual void setId(uint16_t id) { id_ = id; }
    uint16_t     id_;

  private:
    bool isEnabled_;
};

template <class T>
class Container
    : public std::vector<T*>
    , public BaseContainer
{
  public:
    Container(uint16_t id) : BaseContainer(id) {}
    Container(const Container&) = delete;
    Container(Container&& theCopyContainer) : std::vector<T*>(std::move(theCopyContainer)), BaseContainer(std::move(theCopyContainer)) {}

    virtual ~Container() { reset(); }
    void reset()
    {
        for(auto object: *this)
        {
            delete object;
            object = nullptr;
        }
        this->clear();
        idObjectMap_.clear();
    }

    T*& getObject(uint16_t id)
    {
        if(idObjectMap_.find(id) == idObjectMap_.end())
        {
            std::string errorMessage = std::string(__PRETTY_FUNCTION__) + " Error: Object with ID " + std::to_string(id) + " not found";
            throw Exception(std::move(errorMessage));
        }
        return idObjectMap_[id];
    }

    const T* const& getObject(uint16_t id) const
    {
        if(idObjectMap_.find(id) == idObjectMap_.end())
        {
            std::string errorMessage = std::string(__PRETTY_FUNCTION__) + " Error: Object with ID " + std::to_string(id) + " not found";
            throw Exception(std::move(errorMessage));
        }
        return idObjectMap_.at(id);
    }

    T*& getFirstObject()
    {
        if(this->size() == 0)
        {
            std::string errorMessage = std::string(__PRETTY_FUNCTION__) + " Error: no enabled element present for ID " + std::to_string(getId());
            throw Exception(std::move(errorMessage));
        }
        return idObjectMap_.begin()->second;
    }

    const T* const& getFirstObject() const
    {
        if(this->size() == 0)
        {
            std::string errorMessage = std::string(__PRETTY_FUNCTION__) + " Error: no enabled element present for ID " + std::to_string(getId());
            throw Exception(std::move(errorMessage));
        }
        return idObjectMap_.begin()->second;
    }

    void cleanDataStored() override
    {
        for(auto container: *this) { container->cleanDataStored(); }
    }

    void setEnabledAll(bool enable) override
    {
        setEnabled(enable);
        for(auto& container: *this) { container->setEnabledAll(enable); }
    }

    const BaseContainer* getElement(uint16_t theId) const override { return Container<T>::getObject(theId); }

  protected:
    virtual T* addObject(uint16_t objectId, T* object)
    {
        try
        {
            this->getObject(objectId);
        }
        catch(std::exception& ex)
        {
            std::vector<T*>::push_back(object);
            Container::idObjectMap_[objectId] = this->back();
            return this->back();
        }
        delete object;
        object         = nullptr;
        std::string ex = std::string(__PRETTY_FUNCTION__) + " : Object ID " + std::to_string(objectId) + " already present";
        throw Exception(ex.c_str());
        return object;
    }
    std::map<uint16_t, T*> idObjectMap_;

  private:
    T* at(size_t index) { return this->std::vector<T*>::at(index); }
    T* at(size_t index) const { return this->std::vector<T*>::at(index); }

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive& boost::serialization::base_object<std::vector<T*>>(*this);
    }
};

class ChannelContainerBase
{
  public:
    ChannelContainerBase() {}
    virtual ~ChannelContainerBase() {}
    virtual void normalize(uint32_t numberOfEvents) {}

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
    }
};
BOOST_SERIALIZATION_ASSUME_ABSTRACT(ChannelContainerBase)

template <typename T>
class ChannelContainer
    : public std::vector<T>
    , public ChannelContainerBase
{
  public:
    ChannelContainer(uint32_t size) : std::vector<T>(size) {}
    ChannelContainer(uint32_t size, T initialValue) : std::vector<T>(size, initialValue) {}
    ChannelContainer() {}

    T& getChannel(unsigned int channel) { return this->at(channel); }

    friend std::ostream& operator<<(std::ostream& os, const ChannelContainer& channelContainer)
    {
        for(auto& channel: channelContainer) os << channel;
        return os;
    }

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive& boost::serialization::base_object<ChannelContainerBase>(*this);
        theArchive& boost::serialization::base_object<std::vector<T>>(*this);
    }
};

class ChipContainer : public BaseContainer
{
  public:
    ChipContainer(uint16_t id) : BaseContainer(id), nOfRows_(0), nOfCols_(1), container_(nullptr) {}
    ChipContainer(uint16_t id, unsigned int numberOfRows, unsigned int numberOfCols = 1) : BaseContainer(id), nOfRows_(numberOfRows), nOfCols_(numberOfCols), container_(nullptr) {}

    ChipContainer(const ChipContainer&) = delete;
    ChipContainer(ChipContainer&& theCopyContainer) : BaseContainer(std::move(theCopyContainer))
    {
        container_                  = theCopyContainer.container_;
        nOfRows_                    = theCopyContainer.nOfRows_;
        nOfCols_                    = theCopyContainer.nOfCols_;
        theCopyContainer.container_ = nullptr;
    }

    virtual ~ChipContainer() { reset(); }

    template <typename T>
    typename ChannelContainer<T>::iterator begin()
    {
        return static_cast<ChannelContainer<T>*>(container_)->begin();
    }
    template <typename T>
    typename ChannelContainer<T>::iterator end()
    {
        return static_cast<ChannelContainer<T>*>(container_)->end();
    }

    template <typename T>
    typename ChannelContainer<T>::const_iterator begin() const
    {
        return static_cast<ChannelContainer<T>*>(container_)->begin();
    }
    template <typename T>
    typename ChannelContainer<T>::const_iterator end() const
    {
        return static_cast<ChannelContainer<T>*>(container_)->end();
    }

    void setNumberOfChannels(unsigned int numberOfRows, unsigned int numberOfCols)
    {
        nOfRows_ = numberOfRows;
        nOfCols_ = numberOfCols;
    }
    virtual const std::shared_ptr<ChannelGroupBase> getChipOriginalMask() const { return nullptr; };
    virtual const std::shared_ptr<ChannelGroupBase> getChipCurrentMask() const { return nullptr; };

    unsigned int size(void) const { return nOfRows_ * nOfCols_; }
    unsigned int getNumberOfRows() const { return nOfRows_; }
    unsigned int getNumberOfCols() const { return nOfCols_; }

    template <class T>
    T& getChannel(unsigned int row, unsigned int col)
    {
        return static_cast<ChannelContainer<T>*>(container_)->getChannel(row + col * nOfRows_);
    }

    template <class T>
    const T& getChannel(unsigned int row, unsigned int col) const
    {
        return static_cast<ChannelContainer<T>*>(container_)->getChannel(row + col * nOfRows_);
    }

    template <typename T>
    ChannelContainer<T>* getChannelContainer()
    {
        return static_cast<ChannelContainer<T>*>(container_);
    }

    template <typename T>
    const ChannelContainer<T>* getChannelContainer() const
    {
        return static_cast<ChannelContainer<T>*>(container_);
    }

    template <typename T>
    void setChannelContainer(T* container)
    {
        container_ = container;
    }
    template <typename T>
    bool isChannelContainerType()
    {
        ChannelContainer<T>* tmpChannelContainer = dynamic_cast<ChannelContainer<T>*>(container_);
        if(tmpChannelContainer == nullptr) { return false; }
        else
            return true;

        /* const std::type_info& containerTypeId = typeid(container_); */
        /* const std::type_info& templateTypeId = typeid(T*); */

        /* return (containerTypeId.hash_code() == templateTypeId.hash_code()); */
    }

    void cleanDataStored() override
    {
        delete container_;
        container_ = nullptr;
    }

    void setEnabledAll(bool enable) override { setEnabled(enable); }

    const BaseContainer* getElement(uint16_t index) const override
    {
        std::cerr << __PRETTY_FUNCTION__ << " This function should never be called!!! Aborting...";
        abort();
        return nullptr;
    }

    void reset()
    {
        if(container_ != nullptr)
        {
            delete container_;
            container_ = nullptr;
        }
    }

    ChannelContainerBase* getChannelContainer() { return container_; }

    bool hasChannelContainer() const { return (container_ != nullptr); }

  protected:
    unsigned int          nOfRows_;
    unsigned int          nOfCols_;
    ChannelContainerBase* container_;
};

namespace Ph2_HwDescription
{
class ReadoutChip;
class Hybrid;
class OpticalGroup;
class BeBoard;
} // namespace Ph2_HwDescription

class DetectorContainer;

template <typename T, typename HW>
class HWDescriptionContainer : public Container<T>
{
    friend DetectorContainer;
    friend CalibBase;

  public:
    HWDescriptionContainer(uint16_t id) : Container<T>(id) {}
    ~HWDescriptionContainer() {}

    struct QueryFunction
    {
        bool operator()(const T* x)
        {
            if(!fQueryFunction) return true;
            return fQueryFunction(x);
        }
        std::function<bool(const T*)> fQueryFunction;
    };

    typedef boost::filter_iterator<QueryFunction, typename std::vector<T*>::iterator> FilterIter;

    class MyIterator : public FilterIter
    {
      public:
        MyIterator(HWDescriptionContainer<T, HW>* theHWDescriptionContainer, typename std::vector<T*>::iterator theIterator, typename std::vector<T*>::iterator theIteratorEnd)
            : FilterIter(theHWDescriptionContainer->fQueryFunction, theIterator, theIteratorEnd)
        {
        }
        HW* operator*() { return static_cast<HW*>(FilterIter::operator*()); }
    };

    typedef boost::filter_iterator<QueryFunction, typename std::vector<T*>::const_iterator> ConstFilterIter;

    class MyConstIterator : public ConstFilterIter
    {
      public:
        MyConstIterator(const HWDescriptionContainer<T, HW>* theHWDescriptionContainer, typename std::vector<T*>::const_iterator theIterator, typename std::vector<T*>::const_iterator theIteratorEnd)
            : ConstFilterIter(theHWDescriptionContainer->fQueryFunction, theIterator, theIteratorEnd)
        {
        }
        HW* const operator*() const { return static_cast<HW* const>(ConstFilterIter::operator*()); }
    };

    virtual MyIterator begin() { return MyIterator(this, std::vector<T*>::begin(), std::vector<T*>::end()); }

    virtual MyIterator end() { return MyIterator(this, std::vector<T*>::end(), std::vector<T*>::end()); }

    virtual MyConstIterator begin() const { return MyConstIterator(this, std::vector<T*>::begin(), std::vector<T*>::end()); }

    virtual MyConstIterator end() const { return MyConstIterator(this, std::vector<T*>::end(), std::vector<T*>::end()); }

    template <typename theHW = HW> // Small trick to make sure that it is not instantiated before HW forward declaration is defined
    theHW* getObject(size_t theId)
    {
        return static_cast<theHW*>(Container<T>::getObject(theId));
    }

    template <typename theHW = HW> // Small trick to make sure that it is not instantiated before HW forward declaration is defined
    const theHW* getObject(size_t theId) const
    {
        return static_cast<const theHW*>(Container<T>::getObject(theId));
    }

    template <typename theHW = HW> // Small trick to make sure that it is not instantiated before HW forward declaration is defined
    theHW* getFirstObject()
    {
        return static_cast<theHW*>(*(begin()));
    }

    template <typename theHW = HW> // Small trick to make sure that it is not instantiated before HW forward declaration is defined
    const theHW* getFirstObject() const
    {
        return static_cast<const theHW*>(*(begin()));
    }

    uint16_t size() const
    {
        if(!fQueryFunction.fQueryFunction) return std::vector<T*>::size();
        uint16_t theSize = 0;
        for(__attribute__((unused)) auto object: *this) ++theSize;
        return theSize;
        // return std::count_if(this->begin(), this->begin(), fQueryFunction.fQueryFunction);
    }

    uint16_t fullSize() const { return std::vector<T*>::size(); }

    void resetQueryFunction()
    {
        fQueryFunctionMap.clear();
        fQueryFunction.fQueryFunction = 0;
    }
    void addQueryFunction(std::function<bool(const T*)> theInputQueryFunction, const std::string& functionName)
    {
        fQueryFunctionMap[functionName] = theInputQueryFunction;
        updateQueryFunction();
    }
    void removeQueryFunction(const std::string& functionName)
    {
        fQueryFunctionMap.erase(functionName);
        updateQueryFunction();
    }

  protected:
    QueryFunction fQueryFunction;

  private:
    T*&      operator[](size_t pos) { return this->std::vector<T*>::operator[](pos); }
    const T& operator[](size_t pos) const { return this->std::vector<T*>::operator[](pos); }
    T*       at(size_t index) { return this->std::vector<T*>::at(index); }
    T*       at(size_t index) const { return this->std::vector<T*>::at(index); }

    std::map<std::string, std::function<bool(const T*)>> fQueryFunctionMap;

    void updateQueryFunction()
    {
        if(fQueryFunctionMap.size() == 0)
            fQueryFunction.fQueryFunction = 0;
        else
        {
            for(auto nameAndFunction: fQueryFunctionMap)
            {
                if(fQueryFunction.fQueryFunction != 0)
                {
                    auto theCurrentQueryFunction  = fQueryFunction.fQueryFunction;
                    fQueryFunction.fQueryFunction = [theCurrentQueryFunction, nameAndFunction](const T* container)
                    { return (theCurrentQueryFunction(container) && nameAndFunction.second(container)); };
                }
                else
                    fQueryFunction.fQueryFunction = nameAndFunction.second;
            }
        }
    }
};

class HybridContainer : public HWDescriptionContainer<ChipContainer, Ph2_HwDescription::ReadoutChip>
{
  public:
    HybridContainer(uint16_t id) : HWDescriptionContainer<ChipContainer, Ph2_HwDescription::ReadoutChip>(id) {}
    template <typename T>
    T* addChipContainer(uint16_t id, T* chip)
    {
        return static_cast<T*>(HWDescriptionContainer<ChipContainer, Ph2_HwDescription::ReadoutChip>::addObject(id, chip));
    }
};

class OpticalGroupContainer : public HWDescriptionContainer<HybridContainer, Ph2_HwDescription::Hybrid>
{
  public:
    OpticalGroupContainer(uint16_t id) : HWDescriptionContainer<HybridContainer, Ph2_HwDescription::Hybrid>(id) {}
    template <class T>
    T* addHybridContainer(uint16_t id, T* hybrid)
    {
        return static_cast<T*>(HWDescriptionContainer<HybridContainer, Ph2_HwDescription::Hybrid>::addObject(id, hybrid));
    }
};

class BoardContainer : public HWDescriptionContainer<OpticalGroupContainer, Ph2_HwDescription::OpticalGroup>
{
  public:
    BoardContainer(uint16_t id) : HWDescriptionContainer<OpticalGroupContainer, Ph2_HwDescription::OpticalGroup>(id) {}
    template <class T>
    T* addOpticalGroupContainer(uint16_t id, T* opticalGroup)
    {
        return static_cast<T*>(HWDescriptionContainer<OpticalGroupContainer, Ph2_HwDescription::OpticalGroup>::addObject(id, opticalGroup));
    }
};

class DetectorContainer : public HWDescriptionContainer<BoardContainer, Ph2_HwDescription::BeBoard>
{
  public:
    DetectorContainer(uint16_t id = 0) : HWDescriptionContainer<BoardContainer, Ph2_HwDescription::BeBoard>(id) {}
    template <class T>
    T* addBoardContainer(uint16_t id, T* board)
    {
        return static_cast<T*>(HWDescriptionContainer<BoardContainer, Ph2_HwDescription::BeBoard>::addObject(id, board));
    }

    void resetBoardQueryFunction() { this->resetQueryFunction(); }
    void resetOpticalGroupQueryFunction()
    {
        for(uint16_t boardIndex = 0; boardIndex < this->std::vector<BoardContainer*>::size(); ++boardIndex)
        {
            auto theBoard = (*this)[boardIndex];
            theBoard->resetQueryFunction();
        }
    }
    void resetHybridQueryFunction()
    {
        for(uint16_t boardIndex = 0; boardIndex < this->std::vector<BoardContainer*>::size(); ++boardIndex)
        {
            auto theBoard = (*this)[boardIndex];
            for(uint16_t opticalGroupIndex = 0; opticalGroupIndex < theBoard->std::vector<OpticalGroupContainer*>::size(); ++opticalGroupIndex)
            {
                auto theOpticalGroup = (*theBoard)[opticalGroupIndex];
                theOpticalGroup->resetQueryFunction();
            }
        }
    }
    void resetReadoutChipQueryFunction()
    {
        for(uint16_t boardIndex = 0; boardIndex < this->std::vector<BoardContainer*>::size(); ++boardIndex)
        {
            auto theBoard = (*this)[boardIndex];
            for(uint16_t opticalGroupIndex = 0; opticalGroupIndex < theBoard->std::vector<OpticalGroupContainer*>::size(); ++opticalGroupIndex)
            {
                auto theOpticalGroup = (*theBoard)[opticalGroupIndex];
                for(uint16_t hybridIndex = 0; hybridIndex < theOpticalGroup->std::vector<HybridContainer*>::size(); ++hybridIndex)
                {
                    auto theHybrid = (*theOpticalGroup)[hybridIndex];
                    theHybrid->resetQueryFunction();
                }
            }
        }
    }
    void resetAllQueryFunction()
    {
        this->resetQueryFunction();
        for(uint16_t boardIndex = 0; boardIndex < this->std::vector<BoardContainer*>::size(); ++boardIndex)
        {
            auto theBoard = (*this)[boardIndex];
            theBoard->resetQueryFunction();
            for(uint16_t opticalGroupIndex = 0; opticalGroupIndex < theBoard->std::vector<OpticalGroupContainer*>::size(); ++opticalGroupIndex)
            {
                auto theOpticalGroup = (*theBoard)[opticalGroupIndex];
                theOpticalGroup->resetQueryFunction();
                for(uint16_t hybridIndex = 0; hybridIndex < theOpticalGroup->std::vector<HybridContainer*>::size(); ++hybridIndex)
                {
                    auto theHybrid = (*theOpticalGroup)[hybridIndex];
                    theHybrid->resetQueryFunction();
                }
            }
        }
    }

    void addBoardQueryFunction(std::function<bool(const BoardContainer*)> theQueryFunction, const std::string& functionName) { this->addQueryFunction(theQueryFunction, functionName); }
    void addOpticalGroupQueryFunction(std::function<bool(const OpticalGroupContainer*)> theQueryFunction, const std::string& functionName)
    {
        for(uint16_t boardIndex = 0; boardIndex < this->std::vector<BoardContainer*>::size(); ++boardIndex)
        {
            auto theBoard = (*this)[boardIndex];
            theBoard->addQueryFunction(theQueryFunction, functionName);
        }
    }
    void addHybridQueryFunction(std::function<bool(const HybridContainer*)> theQueryFunction, const std::string& functionName)
    {
        for(uint16_t boardIndex = 0; boardIndex < this->std::vector<BoardContainer*>::size(); ++boardIndex)
        {
            auto theBoard = (*this)[boardIndex];
            for(uint16_t opticalGroupIndex = 0; opticalGroupIndex < theBoard->std::vector<OpticalGroupContainer*>::size(); ++opticalGroupIndex)
            {
                auto theOpticalGroup = (*theBoard)[opticalGroupIndex];
                theOpticalGroup->addQueryFunction(theQueryFunction, functionName);
            }
        }
    }
    void addReadoutChipQueryFunction(std::function<bool(const ChipContainer*)> theQueryFunction, const std::string& functionName)
    {
        for(uint16_t boardIndex = 0; boardIndex < this->std::vector<BoardContainer*>::size(); ++boardIndex)
        {
            auto theBoard = (*this)[boardIndex];
            for(uint16_t opticalGroupIndex = 0; opticalGroupIndex < theBoard->std::vector<OpticalGroupContainer*>::size(); ++opticalGroupIndex)
            {
                auto theOpticalGroup = (*theBoard)[opticalGroupIndex];
                for(uint16_t hybridIndex = 0; hybridIndex < theOpticalGroup->std::vector<HybridContainer*>::size(); ++hybridIndex)
                {
                    auto theHybrid = (*theOpticalGroup)[hybridIndex];
                    theHybrid->addQueryFunction(theQueryFunction, functionName);
                }
            }
        }
    }

    void removeBoardQueryFunction(const std::string& functionName) { this->removeQueryFunction(functionName); }
    void removeOpticalGroupQueryFunction(const std::string& functionName)
    {
        for(uint16_t boardIndex = 0; boardIndex < this->std::vector<BoardContainer*>::size(); ++boardIndex)
        {
            auto theBoard = (*this)[boardIndex];
            theBoard->removeQueryFunction(functionName);
        }
    }
    void removeHybridQueryFunction(const std::string& functionName)
    {
        for(uint16_t boardIndex = 0; boardIndex < this->std::vector<BoardContainer*>::size(); ++boardIndex)
        {
            auto theBoard = (*this)[boardIndex];
            for(uint16_t opticalGroupIndex = 0; opticalGroupIndex < theBoard->std::vector<OpticalGroupContainer*>::size(); ++opticalGroupIndex)
            {
                auto theOpticalGroup = (*theBoard)[opticalGroupIndex];
                theOpticalGroup->removeQueryFunction(functionName);
            }
        }
    }
    void removeReadoutChipQueryFunction(const std::string& functionName)
    {
        for(uint16_t boardIndex = 0; boardIndex < this->std::vector<BoardContainer*>::size(); ++boardIndex)
        {
            auto theBoard = (*this)[boardIndex];
            for(uint16_t opticalGroupIndex = 0; opticalGroupIndex < theBoard->std::vector<OpticalGroupContainer*>::size(); ++opticalGroupIndex)
            {
                auto theOpticalGroup = (*theBoard)[opticalGroupIndex];
                for(uint16_t hybridIndex = 0; hybridIndex < theOpticalGroup->std::vector<HybridContainer*>::size(); ++hybridIndex)
                {
                    auto theHybrid = (*theOpticalGroup)[hybridIndex];
                    theHybrid->removeQueryFunction(functionName);
                }
            }
        }
    }
};

#endif
