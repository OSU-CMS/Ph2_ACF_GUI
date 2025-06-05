/*

        \file                          Container.h
        \brief                         containers for DAQ
        \author                        Fabio Ravera, Lorenzo Uplegger
        \version                       1.0
        \date                          06/06/19
        Support :                      mail to : fabio.ravera@cern.ch

 */

#ifndef __DATA_CONTAINER_H__
#define __DATA_CONTAINER_H__

#include "Utils/ChannelGroupHandler.h"
#include "Utils/ConsoleColor.h"
#include "Utils/Container.h"
#include "Utils/EmptyContainer.h"
#include "Utils/easylogging++.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <cxxabi.h>
#include <iostream>
#include <map>
#include <type_traits>
#include <vector>

template <typename T>
class ChannelContainer;
class ChipContainer;
class SummaryContainerBase;

class SummaryBase
{
  public:
    SummaryBase() { ; }
    virtual ~SummaryBase() { ; }
    virtual void  makeSummaryOfChannels(const ChipContainer*                    theChipContainer,
                                        const std::shared_ptr<ChannelGroupBase> chipOriginalMask,
                                        const std::shared_ptr<ChannelGroupBase> cTestChannelGroup,
                                        const uint32_t                          numberOfEvents)                                                                                                         = 0;
    virtual void  makeSummaryOfSummary(const SummaryContainerBase* theSummaryList, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents) = 0;
    virtual void* getSummaryPointer()                                                                                                                                          = 0;

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
    }
};
BOOST_SERIALIZATION_ASSUME_ABSTRACT(SummaryBase)

class SummaryContainerBase
{
  public:
    SummaryContainerBase() { ; }
    virtual ~SummaryContainerBase() { ; }
    virtual void emplace_back(SummaryBase* theSummary) = 0;
};

template <typename T>
class SummaryContainer
    : public std::vector<T*>
    , public SummaryContainerBase
{
  public:
    SummaryContainer() { ; }
    ~SummaryContainer() { ; }
    void emplace_back(SummaryBase* theSummary) override { std::vector<T*>::emplace_back(theSummary); }
};

// Summary class forward declaration
template <class S, class C>
class Summary;

// Functor for summarize channels - default case
template <class S, class C, bool hasAverageFunction = false>
struct ChannelSummarizer
{
    void operator()(Summary<S, C>&                          theSummary,
                    const ChipContainer*                    theChipContainer,
                    const std::shared_ptr<ChannelGroupBase> chipOriginalMask,
                    const std::shared_ptr<ChannelGroupBase> cTestChannelGroup,
                    const uint32_t                          numberOfEvents)
    {
        int32_t status;
        LOG(ERROR) << __PRETTY_FUNCTION__ << " Member function makeChannelAverage<C> does not exist for " << abi::__cxa_demangle(typeid(S).name(), 0, 0, &status) << " \nAborting...";
        abort();
    }
};

// Functor for summarize summaries - default case
template <class S, class C, bool hasAverageFunction = false>
struct SummarySummarizer
{
    void operator()(Summary<S, C>& theSummary, const SummaryContainerBase* theSummaryList, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents)
    {
        int32_t status = 0;
        LOG(ERROR) << __PRETTY_FUNCTION__ << " Member function makeSummaryAverage does not exist for " << abi::__cxa_demangle(typeid(S).name(), 0, 0, &status) << " \nAborting...";
        abort();
    }
};

// // SFINAE: check if object T has makeChannelAverage<S> member function
// template <typename T, typename S>
// class has_makeChannelAverage
// {
//     typedef char one;
//     struct two { char x[2]; };

//     template <typename C, typename D> static one test( decltype(&C::template makeChannelAverage<D>) ) ;
//     template <typename C, typename D> static two test(...);

// public:
//     enum { value = sizeof(test<T,S>(0)) == sizeof(char) };
// };

namespace user_detail
{
template <typename>
struct sfinae_true : std::true_type
{
};

template <typename T, typename S, typename... A0>
static auto test_makeChannelAverage(int) -> sfinae_true<decltype(std::declval<T>().template makeChannelAverage<S>(std::declval<A0>()...))>;
template <typename, typename... A0>
static auto test_makeChannelAverage(long) -> std::false_type;

template <typename T, typename... A0>
static auto test_makeSummaryAverage(int) -> sfinae_true<decltype(std::declval<T>().makeSummaryAverage(std::declval<A0>()...))>;
template <typename, typename... A0>
static auto test_makeSummaryAverage(long) -> std::false_type;

template <typename T, typename... A0>
static auto test_normalize(int) -> sfinae_true<decltype(std::declval<T>().normalize(std::declval<A0>()...))>;
template <typename, typename... A0>
static auto test_normalize(long) -> std::false_type;

} // namespace user_detail

class ChannelGroupBase;

// SFINAE: check if object T has makeChannelAverage<S> member function
template <typename T, typename S>
struct has_makeChannelAverage
    : decltype(user_detail::test_makeChannelAverage<T, S, const ChipContainer*, const std::shared_ptr<ChannelGroupBase>, const std::shared_ptr<ChannelGroupBase>, const uint32_t>(0))
{
};

// SFINAE: check if object T has makeSummaryAverage member function
template <typename T, typename Arg>
struct has_makeSummaryAverage : decltype(user_detail::test_makeSummaryAverage<T, const std::vector<Arg>*, const std::vector<uint32_t>&, const uint32_t>(0))
{
};

// SFINAE: check if object T has normalize member function
template <typename T>
struct has_normalize : decltype(user_detail::test_normalize<T, const uint32_t>(0))
{
};

// template <typename T>
// class has_normalize
// {
//     typedef char one;
//     struct two { char x[2]; };

//     template <typename C> static one test( decltype(&C::normalize) ) ;
//     template <typename C> static two test(...);

// public:
//     enum { value = sizeof(test<T>(0)) == sizeof(char) };
// };

template <class S, class C>
class Summary : public SummaryBase
{
  public:
    Summary() { ; }
    Summary(const S& theSummary) { theSummary_ = theSummary; }
    Summary(S&& theSummary)
    {
        theSummary_            = std::move(theSummary);
        theSummary.theSummary_ = nullptr;
    }
    Summary& operator=(S&& theSummary) { theSummary_ = std::move(theSummary); }
    Summary(const Summary<S, C>& summary) { theSummary_ = summary.theSummary_; }

    Summary& operator=(const Summary& summary) { theSummary_ = summary.theSummary_; }
    Summary& operator=(const Summary&& summary) { theSummary_ = std::move(summary.theSummary_); }

    ~Summary() { ; }

    void makeSummaryOfChannels(const ChipContainer*                    theChipContainer,
                               const std::shared_ptr<ChannelGroupBase> chipOriginalMask,
                               const std::shared_ptr<ChannelGroupBase> cTestChannelGroup,
                               const uint32_t                          numberOfEvents) override
    {
        ChannelSummarizer<S, C, has_makeChannelAverage<S, C>::value> theChannelSummarizer;
        theChannelSummarizer(*this, theChipContainer, chipOriginalMask, cTestChannelGroup, numberOfEvents);
    }

    void makeSummaryOfSummary(const SummaryContainerBase* theSummaryList, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents) override
    {
        SummarySummarizer<S, C, has_makeSummaryAverage<S, C>::value> theSummarySummarizer;
        theSummarySummarizer(*this, theSummaryList, theNumberOfEnabledChannelsList, numberOfEvents);
    }

    void* getSummaryPointer() { return static_cast<void*>(&theSummary_); }

    S theSummary_;

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive.template register_type<Summary<S, C>>();
        theArchive& boost::serialization::base_object<SummaryBase>(*this);
        theArchive & theSummary_;
    }
};

// Functor for summarize channels - case when makeChannelAverage<C> is defined
template <class S, class C>
struct ChannelSummarizer<S, C, true>
{
    void operator()(Summary<S, C>&                          theSummary,
                    const ChipContainer*                    theChipContainer,
                    const std::shared_ptr<ChannelGroupBase> chipOriginalMask,
                    const std::shared_ptr<ChannelGroupBase> cTestChannelGroup,
                    const uint32_t                          numberOfEvents)
    {
        theSummary.theSummary_.template makeChannelAverage<C>(theChipContainer, chipOriginalMask, cTestChannelGroup, numberOfEvents);
    }
};

// Functor for summarize summaries - case when makeSummaryAverage is defined
template <class S, class C>
struct SummarySummarizer<S, C, true>
{
    void operator()(Summary<S, C>& theSummary, const SummaryContainerBase* theSummaryList, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents)
    {
        const SummaryContainer<SummaryBase>* tmpSummaryContainer = static_cast<const SummaryContainer<SummaryBase>*>(theSummaryList);
        std::vector<C>                       tmpSummaryVector;
        for(auto summary: *tmpSummaryContainer) { tmpSummaryVector.emplace_back(std::move(*static_cast<C*>(summary->getSummaryPointer()))); }
        theSummary.theSummary_.makeSummaryAverage(&tmpSummaryVector, theNumberOfEnabledChannelsList, numberOfEvents);
        delete theSummaryList;
    }
};

class BaseDataContainer
{
  public:
    BaseDataContainer() : summary_{nullptr} { ; }

    virtual ~BaseDataContainer()
    {
        if(summary_ != nullptr)
        {
            delete summary_;
            summary_ = nullptr;
        }
    }

    BaseDataContainer(const BaseDataContainer&) = delete;
    BaseDataContainer(BaseDataContainer&& theCopyContainer)
    {
        summary_                  = theCopyContainer.summary_;
        theCopyContainer.summary_ = nullptr;
    }

    // virtual void initialize() = 0;
    virtual uint32_t normalizeAndAverageContainers(const BaseContainer* theContainer, const BaseDataContainer* theChannelGroupContainer, const uint32_t numberOfEvents) = 0;
    virtual void     resetNormalizationStatus()                                                                                                                         = 0;

    template <typename T>
    bool isSummaryContainerType()
    {
        T* tmpSummaryContainer = dynamic_cast<T*>(summary_);
        if(tmpSummaryContainer == nullptr) { return false; }
        else
            return true;

        /* const std::type_info& containerTypeId = typeid(summary_); */
        /* const std::type_info& templateTypeId = typeid(T*); */

        /* return (containerTypeId.hash_code() == templateTypeId.hash_code()); */
    }

    template <typename S, typename T = EmptyContainer>
    S& getSummary()
    {
        return static_cast<Summary<S, T>*>(summary_)->theSummary_;
    }

    template <typename S, typename T = EmptyContainer>
    const S& getSummary() const
    {
        return static_cast<Summary<S, T>*>(summary_)->theSummary_;
    }

    template <typename S, typename T = EmptyContainer>
    Summary<S, T>* getSummaryContainer()
    {
        return static_cast<Summary<S, T>*>(summary_);
    }

    template <typename S, typename T = EmptyContainer>
    const Summary<S, T>* getSummaryContainer() const
    {
        return static_cast<Summary<S, T>*>(summary_);
    }

    bool hasSummary() { return (summary_ != nullptr); }

    template <typename T>
    void setSummaryContainer(T* summary)
    {
        summary_ = summary;
    }

    SummaryBase* summary_;
    bool         isNormalized{false};

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive & summary_;
    }
};
BOOST_SERIALIZATION_ASSUME_ABSTRACT(BaseDataContainer)

template <class T>
class DataContainer
    : public Container<T>
    , public BaseDataContainer
{
  public:
    DataContainer(uint16_t id) : Container<T>(id) { ; }
    // DataContainer(unsigned int size) : Container<T>(size) {}
    virtual ~DataContainer() { ; }

    DataContainer(const DataContainer&) = delete;
    DataContainer(DataContainer&& theCopyContainer) : Container<T>(std::move(theCopyContainer)), BaseDataContainer(std::move(theCopyContainer)) {}

    void remapIdtoPointer()
    {
        for(auto object: *this)
        {
            Container<T>::idObjectMap_[object->getId()] = object;
            object->remapIdtoPointer();
        }
    }

    void setId(uint16_t id) override { BaseContainer::setId(id); }

    template <typename S, typename V>
    void initialize()
    {
        if(!std::is_same<S, EmptyContainer>::value) summary_ = new Summary<S, V>();
    }
    template <typename S, typename V>
    void initialize(S& theSummary)
    {
        if(!std::is_same<S, EmptyContainer>::value) summary_ = new Summary<S, V>(theSummary);
    }

    template <typename S, typename V>
    void resetSummary()
    {
        if(!std::is_same<S, EmptyContainer>::value) static_cast<Summary<S, V>*>(summary_)->theSummary_ = S();
        isNormalized = false;
    }
    template <typename S, typename V>
    void resetSummary(S& theSummary)
    {
        if(!std::is_same<S, EmptyContainer>::value) static_cast<Summary<S, V>*>(summary_)->theSummary_ = theSummary;
        isNormalized = false;
    }

    SummaryContainerBase* getAllObjectSummaryContainers() const
    {
        SummaryContainerBase* SummaryContainerList = new SummaryContainer<SummaryBase>;
        for(auto container: *this) SummaryContainerList->emplace_back(container->summary_);
        return SummaryContainerList;
    }

    uint32_t normalizeAndAverageContainers(const BaseContainer* theContainer, const BaseDataContainer* theChannelGroupContainer, const uint32_t numberOfEvents) override
    {
        uint32_t              numberOfEnabledChannels_ = 0;
        std::vector<uint32_t> theNumberOfEnabledChannelsList;
        for(auto container: *this)
        {
            uint32_t numberOfContainerEnabledChannels = 0;
            if(container != nullptr)
            {
                numberOfContainerEnabledChannels = container->normalizeAndAverageContainers(
                    theContainer->getElement(container->getId()), static_cast<const DataContainer<T>*>(theChannelGroupContainer)->getObject(container->getId()), numberOfEvents);
            }
            theNumberOfEnabledChannelsList.emplace_back(numberOfContainerEnabledChannels);
            numberOfEnabledChannels_ += numberOfContainerEnabledChannels;
        }
        if(!isNormalized)
        {
            isNormalized = true;
            if(summary_ != nullptr) summary_->makeSummaryOfSummary(getAllObjectSummaryContainers(), theNumberOfEnabledChannelsList,
                                                                   numberOfEvents); // sum of chip container needed!!!
        }
        return numberOfEnabledChannels_;
    }

    void resetNormalizationStatus() override
    {
        isNormalized = false;
        for(auto container: *this) container->resetNormalizationStatus();
    }

    void cleanDataStored() override
    {
        delete summary_;
        summary_ = nullptr;
        for(auto container: *this) { container->cleanDataStored(); }
    }

    // T*& getObject(size_t id) {return std::vector<T*>::at(index);}
    // const T* const& getObject(size_t id) const {return std::vector<T*>::at(index);}

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive& this->id_;
        theArchive& boost::serialization::base_object<BaseDataContainer>(*this);
        theArchive& boost::serialization::base_object<Container<T>>(*this);
    }

    T*&             at(size_t index) { return this->std::vector<T*>::at(index); }
    const T* const& at(size_t index) const { return this->std::vector<T*>::at(index); }
};

template <typename T>
class ChannelDataContainer;

// Functor for mormalizing channels - default case
template <class T, bool hasAverageFunction = false>
struct ChannelNormalizer
{
    void operator()(ChannelDataContainer<T>& theChannelDataContainer, const uint32_t numberOfEvents)
    {
        int32_t status;
        LOG(ERROR) << __PRETTY_FUNCTION__ << " normalize function is not defined for " << abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
    }
};

template <typename T>
class ChannelDataContainer : public ChannelContainer<T> //, public ChannelContainerBase
{
  public:
    ChannelDataContainer(uint32_t size) : ChannelContainer<T>(size) {}
    ChannelDataContainer(uint32_t size, T initialValue) : ChannelContainer<T>(size, initialValue) {}
    ChannelDataContainer() : ChannelContainer<T>() {}

    void normalize(uint32_t numberOfEvents) override
    {
        ChannelNormalizer<T, has_normalize<T>::value> theChannelNormalizer;
        theChannelNormalizer(*this, numberOfEvents);
    }

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive& boost::serialization::base_object<ChannelContainer<T>>(*this);
    }
};

// Functor for mormalizing channels - case when normalize is defined
template <class T>
struct ChannelNormalizer<T, true>
{
    void operator()(ChannelDataContainer<T>& theChannelDataContainer, const uint32_t numberOfEvents)
    {
        for(auto& channel: theChannelDataContainer) channel.normalize(numberOfEvents);
    }
};

class ChipDataContainer
    : public ChipContainer
    , public BaseDataContainer
{
  public:
    ChipDataContainer(uint16_t id) : ChipContainer(id) {}
    ChipDataContainer() : ChipContainer(65535) {}

    ChipDataContainer(uint16_t id, unsigned int numberOfRows, unsigned int numberOfCols = 1) : ChipContainer(id, numberOfRows, numberOfCols) {}

    ChipDataContainer(const ChipDataContainer&) = delete;
    ChipDataContainer(ChipDataContainer&& theCopyContainer) : ChipContainer(std::move(theCopyContainer)), BaseDataContainer(std::move(theCopyContainer)) {}

    virtual ~ChipDataContainer() { ; }

    void remapIdtoPointer() {};

    void setId(uint16_t id) override { BaseContainer::setId(id); }

    template <typename S, typename V>
    void initialize()
    {
        if(!std::is_same<S, EmptyContainer>::value) summary_ = new Summary<S, V>();
        initializeChannels<V>();
    }
    template <typename S, typename V>
    void initialize(S& theSummary, V& initialValue)
    {
        if(!std::is_same<S, EmptyContainer>::value) summary_ = new Summary<S, V>(theSummary);
        initializeChannels<V>(initialValue);
    }

    template <typename V>
    void initializeChannels()
    {
        if(!std::is_same<V, EmptyContainer>::value) container_ = new ChannelDataContainer<V>(nOfRows_ * nOfCols_);
    }
    template <typename V>
    void initializeChannels(V& initialValue)
    {
        if(!std::is_same<V, EmptyContainer>::value) container_ = new ChannelDataContainer<V>(nOfRows_ * nOfCols_, initialValue);
    }

    template <typename S, typename V>
    void resetSummary()
    {
        if(!std::is_same<S, EmptyContainer>::value) static_cast<Summary<S, V>*>(summary_)->theSummary_ = S();
        isNormalized = false;
    }
    template <typename S, typename V>
    void resetSummary(S& theSummary)
    {
        if(!std::is_same<S, EmptyContainer>::value) static_cast<Summary<S, V>*>(summary_)->theSummary_ = theSummary;
        isNormalized = false;
    }

    template <typename V>
    void resetChannels()
    {
        if(!std::is_same<V, EmptyContainer>::value)
            for(auto& channel: *this->getChannelContainer<V>()) channel = V();
        isNormalized = false;
    }
    template <typename V>
    void resetChannels(V& initialValue)
    {
        if(!std::is_same<V, EmptyContainer>::value)
            for(auto& channel: *this->getChannelContainer<V>()) channel = initialValue;
        isNormalized = false;
    }

    uint32_t normalizeAndAverageContainers(const BaseContainer* theContainer, const BaseDataContainer* theChannelGroupContainer, const uint32_t numberOfEvents)
    {
        // std::cout << " Index " << theContainer->getId()
        //     << " # of enabled channels " << cTestChannelGroup->getNumberOfEnabledChannels(static_cast<const ChipContainer*>(theContainer)->getChipOriginalMask())
        //     << " # of events " << numberOfEvents
        //     << "\n";
        if(!isNormalized)
        {
            isNormalized = true;
            if(container_ != nullptr) container_->normalize(numberOfEvents);
            if(summary_ != nullptr)
                summary_->makeSummaryOfChannels(this,
                                                static_cast<const ChipContainer*>(theContainer)->getChipOriginalMask(),
                                                theChannelGroupContainer->getSummary<std::shared_ptr<ChannelGroupHandler>>()->allChannelGroup(),
                                                numberOfEvents);
        }
        return theChannelGroupContainer->getSummary<std::shared_ptr<ChannelGroupHandler>>()->allChannelGroup()->getNumberOfEnabledChannels(
            static_cast<const ChipContainer*>(theContainer)->getChipOriginalMask());
    }

    void resetNormalizationStatus() override { isNormalized = false; }

    void cleanDataStored() override
    {
        delete summary_;
        summary_ = nullptr;
        ChipContainer::cleanDataStored();
    }

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive & id_;
        theArchive & nOfRows_;
        theArchive & nOfCols_;
        theArchive& boost::serialization::base_object<BaseDataContainer>(*this);
        theArchive & container_;
    }
};

class HybridDataContainer : public DataContainer<ChipDataContainer>
{
  public:
    HybridDataContainer(uint16_t id) : DataContainer<ChipDataContainer>(id) {}
    HybridDataContainer() : DataContainer<ChipDataContainer>(65535u) {}

    HybridDataContainer(const HybridDataContainer&) = delete;
    HybridDataContainer(HybridDataContainer&& theCopyContainer) : DataContainer<ChipDataContainer>(std::move(theCopyContainer)) {}

    template <typename T>
    T* addChipDataContainer(uint16_t id, T* chip)
    {
        return static_cast<T*>(DataContainer<ChipDataContainer>::addObject(id, chip));
    }
    ChipDataContainer* addChipDataContainer(uint16_t id, uint16_t row, uint16_t col = 1)
    {
        try
        {
            DataContainer<ChipDataContainer>::getObject(id);
        }
        catch(std::exception& ex)
        {
            return DataContainer<ChipDataContainer>::addObject(id, new ChipDataContainer(id, row, col));
        }
        return DataContainer<ChipDataContainer>::getObject(id);
    }

    ChipDataContainer*& getChip(uint16_t chipId) { return getObject(chipId); }

    const ChipDataContainer* const& getChip(uint16_t chipId) const { return getObject(chipId); }

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive& boost::serialization::base_object<DataContainer<ChipDataContainer>>(*this);
    }
};

class OpticalGroupDataContainer : public DataContainer<HybridDataContainer>
{
  public:
    OpticalGroupDataContainer(uint16_t id) : DataContainer<HybridDataContainer>(id) {}
    OpticalGroupDataContainer() : DataContainer<HybridDataContainer>(65535u) {}
    OpticalGroupDataContainer(const OpticalGroupDataContainer&) = delete;
    OpticalGroupDataContainer(OpticalGroupDataContainer&& theCopyContainer) : DataContainer<HybridDataContainer>(std::move(theCopyContainer)) {}

    template <class T>
    T* addHybridDataContainer(uint16_t id, T* hybrid)
    {
        return static_cast<T*>(DataContainer<HybridDataContainer>::addObject(id, hybrid));
    }
    HybridDataContainer* addHybridDataContainer(uint16_t id)
    {
        try
        {
            DataContainer<HybridDataContainer>::getObject(id);
        }
        catch(std::exception& ex)
        {
            return DataContainer<HybridDataContainer>::addObject(id, new HybridDataContainer(id));
        }
        LOG(WARNING) << GREEN << "Object Id already present: " << BOLDYELLOW << id << RESET;
        return DataContainer<HybridDataContainer>::getObject(id);
    }

    HybridDataContainer*& getHybrid(uint16_t hybridId) { return getObject(hybridId); }

    const HybridDataContainer* const& getHybrid(uint16_t hybridId) const { return getObject(hybridId); }

    ChipDataContainer*& getChip(uint16_t hybridId, uint16_t chipId) { return getHybrid(hybridId)->getChip(chipId); }

    const ChipDataContainer* const& getChip(uint16_t hybridId, uint16_t chipId) const { return getHybrid(hybridId)->getChip(chipId); }

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive& boost::serialization::base_object<DataContainer<HybridDataContainer>>(*this);
    }
};

class BoardDataContainer : public DataContainer<OpticalGroupDataContainer>
{
  public:
    BoardDataContainer(uint16_t id) : DataContainer<OpticalGroupDataContainer>(id) {}
    BoardDataContainer() : DataContainer<OpticalGroupDataContainer>(65535u) {}
    BoardDataContainer(const BoardDataContainer&) = delete;
    BoardDataContainer(BoardDataContainer&& theCopyContainer) : DataContainer<OpticalGroupDataContainer>(std::move(theCopyContainer)) {}

    template <class T>
    T* addOpticalGroupDataContainer(uint16_t id, T* opticalGroup)
    {
        return static_cast<T*>(DataContainer<OpticalGroupDataContainer>::addObject(id, opticalGroup));
    }
    OpticalGroupDataContainer* addOpticalGroupDataContainer(uint16_t id)
    {
        try
        {
            DataContainer<OpticalGroupDataContainer>::getObject(id);
        }
        catch(std::exception& ex)
        {
            return DataContainer<OpticalGroupDataContainer>::addObject(id, new OpticalGroupDataContainer(id));
        }
        LOG(WARNING) << GREEN << "Object Id already present: " << BOLDYELLOW << id << RESET;
        return DataContainer<OpticalGroupDataContainer>::getObject(id);
    }

    OpticalGroupDataContainer*& getOpticalGroup(uint16_t opticalGroupId) { return getObject(opticalGroupId); }

    const OpticalGroupDataContainer* const& getOpticalGroup(uint16_t opticalGroupId) const { return getObject(opticalGroupId); }

    HybridDataContainer*& getHybrid(uint16_t opticalGroupId, uint16_t hybridId) { return getOpticalGroup(opticalGroupId)->getHybrid(hybridId); }

    const HybridDataContainer* const& getHybrid(uint16_t opticalGroupId, uint16_t hybridId) const { return getOpticalGroup(opticalGroupId)->getHybrid(hybridId); }

    ChipDataContainer*& getChip(uint16_t opticalGroupId, uint16_t hybridId, uint16_t chipId) { return getOpticalGroup(opticalGroupId)->getHybrid(hybridId)->getChip(chipId); }

    const ChipDataContainer* const& getChip(uint16_t opticalGroupId, uint16_t hybridId, uint16_t chipId) const { return getOpticalGroup(opticalGroupId)->getHybrid(hybridId)->getChip(chipId); }

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive& boost::serialization::base_object<DataContainer<OpticalGroupDataContainer>>(*this);
    }
};

class DetectorDataContainer : public DataContainer<BoardDataContainer>
{
  public:
    DetectorDataContainer(uint16_t id = 0) : DataContainer<BoardDataContainer>(id) {}
    DetectorDataContainer(const DetectorDataContainer&) = delete;
    DetectorDataContainer(DetectorDataContainer&& theCopyContainer) : DataContainer<BoardDataContainer>(std::move(theCopyContainer)) {}

    ~DetectorDataContainer() {}
    template <class T>
    T* addBoardDataContainer(uint16_t id, T* board)
    {
        return static_cast<T*>(DataContainer<BoardDataContainer>::addObject(id, board));
    }
    BoardDataContainer* addBoardDataContainer(uint16_t id)
    {
        try
        {
            DataContainer<BoardDataContainer>::getObject(id);
        }
        catch(std::exception& ex)
        {
            return DataContainer<BoardDataContainer>::addObject(id, new BoardDataContainer(id));
        }
        LOG(WARNING) << GREEN << "Object Id already present: " << BOLDYELLOW << id << RESET;
        return DataContainer<BoardDataContainer>::getObject(id);
    }

    BoardDataContainer*& getBoard(uint16_t boardId) { return getObject(boardId); }

    const BoardDataContainer* const& getBoard(uint16_t boardId) const { return getObject(boardId); }

    OpticalGroupDataContainer*& getOpticalGroup(uint16_t boardId, uint16_t opticalGroupId) { return getBoard(boardId)->getOpticalGroup(opticalGroupId); }

    const OpticalGroupDataContainer* const& getOpticalGroup(uint16_t boardId, uint16_t opticalGroupId) const { return getBoard(boardId)->getOpticalGroup(opticalGroupId); }

    HybridDataContainer*& getHybrid(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId) { return getBoard(boardId)->getOpticalGroup(opticalGroupId)->getHybrid(hybridId); }

    const HybridDataContainer* const& getHybrid(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId) const { return getBoard(boardId)->getOpticalGroup(opticalGroupId)->getHybrid(hybridId); }

    ChipDataContainer*& getChip(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId, uint16_t chipId)
    {
        return getBoard(boardId)->getOpticalGroup(opticalGroupId)->getHybrid(hybridId)->getChip(chipId);
    }

    const ChipDataContainer* const& getChip(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId, uint16_t chipId) const
    {
        return getBoard(boardId)->getOpticalGroup(opticalGroupId)->getHybrid(hybridId)->getChip(chipId);
    }

  private:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive& boost::serialization::base_object<DataContainer<BoardDataContainer>>(*this);
    }
};

#endif
