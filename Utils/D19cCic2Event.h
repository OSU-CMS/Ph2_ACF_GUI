/*

        \file                          Event.h
        \brief                         Event handling from DAQ
        \author                        Nicolas PIERRE
        \version                       1.0
        \date                                  10/07/14
        Support :                      mail to : nicolas.pierre@icloud.com

 */

#ifndef __D19cCic2Event_H__
#define __D19cCic2Event_H__

#include "Event.h"
#include <array>
#include <iterator>
#include <numeric>

namespace Ph2_HwInterface
{

/*!
 * \class Cluster2S
 * \brief Cluster2S object for the Event
 */
struct Cluster2S
{
    Cluster2S() {};
    bool         parseData(uint32_t data);
    uint16_t     fFirstStrip;
    uint8_t      fClusterWidth;
    uint8_t      getSensor() const { return (fFirstStrip + 1) & 0x01; }
    inline float getBaricentre() const { return fFirstStrip + float(fClusterWidth) / 2. - 0.5; };
    inline bool  isChannelHit(uint8_t channel) const;
    void         print() const;
};

template <typename T, size_t N>
struct ClusterCollection
{
    ClusterCollection() {};
    uint8_t     fNumberOfClusters{0};
    void        addCluster(T theCluster) { theContainer[fNumberOfClusters++] = theCluster; }
    auto        begin() { return theContainer.begin(); }
    auto        begin() const { return theContainer.begin(); }
    auto        end() { return theContainer.begin() + fNumberOfClusters; }
    auto        end() const { return theContainer.begin() + fNumberOfClusters; }
    size_t      size() const { return fNumberOfClusters; }
    auto&       operator[](std::size_t i) { return theContainer[i]; }
    const auto& operator[](std::size_t i) const { return theContainer[i]; }

  private:
    std::array<T, N> theContainer;
};

struct PixelClusterPS
{
    PixelClusterPS() {};
    bool         parseData(uint32_t data);
    uint8_t      fAddress{0xFF};
    uint8_t      fWidth{0xFF};
    uint8_t      fZpos{0xFF};
    inline float getBaricentre() const;
    inline bool  isChannelHit(uint8_t row, uint8_t col) const;
    void         print() const;
};

struct StripClusterPS
{
    StripClusterPS() {};
    bool         parseData(uint32_t data);
    uint8_t      fAddress{0xFF};
    uint8_t      fMip{0xFF};
    uint8_t      fWidth{0xFF};
    inline float getBaricentre() const;
    inline bool  isChannelHit(uint8_t col) const;
    void         print() const;
};

struct EventStub
{
    EventStub() : fPosition(255u), fBend(255u), fRow(255u) {};
    bool           parseData(uint32_t data, bool is2S);
    inline uint8_t getPosition() const { return fPosition; }
    inline uint8_t getBend() const { return fBend; }
    inline uint8_t getRow() const { return fRow; }
    inline float   getCenter() const { return static_cast<float>((fPosition / 2.)); }

    uint8_t fPosition{0xFF};
    uint8_t fBend{0xFF};
    uint8_t fRow{0xFF};
    void    print() const;
};
/*!
 * \class Event
 * \brief Event container to manipulate event flux from the Cbc
 */

struct HybridL1EventInfo
{
    HybridL1EventInfo() {}
    void     parseData(std::vector<uint32_t>::const_iterator dataStart);
    uint8_t  fErrorCode{0};
    uint8_t  fHybridId{0};
    uint8_t  fChipId{0};
    uint8_t  fChipType{0};
    uint16_t fFrameDelay{0};
    uint16_t fStatusBits{0};
    uint16_t fL1counter{0};
    uint8_t  fNumberOfStripClusters{0};
    uint8_t  fNumberOfPixelClusters{0};
    void     print() const;
};

struct ChipL1EventInfo
{
    ChipL1EventInfo() {}
    void                    parseData(std::array<uint32_t, NUMBER_OF_CIC_PORTS * 9>::const_iterator dataStart, size_t bitStart = 0);
    uint8_t                 fErrorCode{0};
    uint16_t                fPipelineAddress{0};
    uint16_t                fL1id{0};
    std::array<uint32_t, 8> fRawData{0, 0, 0, 0, 0, 0, 0, 0};
    inline bool             isChannelHit(uint8_t channel) const;
    std::vector<uint8_t>    getChannelHitList() const;
    inline uint8_t          countNumberOfHits() const;
    void                    print() const;
};

struct HybridStubEventInfo
{
    HybridStubEventInfo() {}
    void     parseData(std::vector<uint32_t>::const_iterator dataStart);
    uint16_t fStubDataDelay;
    uint16_t fStatusBits{0};
    uint8_t  fNumberOfStubs{0};
    uint16_t fBunchCrossingId{0};
    void     print() const;
};

/*!
 * \class CicEvent
 * \brief Event container to manipulate event flux from the Cbc2
 */
class D19cCic2Event : public Event
{
  public:
    /*!
     * \brief Constructor of the Event Class
     * \param pBoard : Board to work with
     * \param pNbCbc
     * \param pEventBuf : the pointer to the raw Event buffer of this Event
     */
    D19cCic2Event(const Ph2_HwDescription::BeBoard* pBoard, std::vector<uint32_t>& list, bool pWithTLU = false);
    /*!
     * \brief Copy Constructor of the Event Class
     */
    // CicEvent ( const Event& pEvent );
    /*!
     * \brief Destructor of the Event Class
     */
    ~D19cCic2Event() {}

    /*!
     * \brief Set an Event to the Event map
     * \param pEvent : Event to set
     * \return Aknowledgement of the Event setting (1/0)
     */
    void decodeEvent();

    /*!
     * \brief Get the Cbc Event counter
     * \return Cbc Event counter
     */
    uint32_t GetEventCountCBC() const override { return fEventCountCBC; }

    /*!
     * \brief Function to get all Error bits
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return Error bit
     */
    uint32_t Error(uint8_t pHybridId, uint8_t pCbcId) override;
    /*!
     * \brief Function to get pipeline address
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return Pipeline address
     */
    uint32_t PipelineAddress(uint8_t pHybridId, uint8_t pCbcId) override;
    /*!
     * \brief Function to get a CBC pixel bit data
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \param i : pixel bit data number i
     * \return Data Bit
     */
    bool DataBit(uint8_t pHybridId, uint8_t pCbcId, uint8_t row, uint8_t col) override;
    /*!
     * \brief Function to get bit vector of CBC data
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return Data Bit vector
     */
    std::vector<bool> DataBitVector(uint8_t pHybridId, uint8_t pCbcId) override;

    /*!
     * \brief Function to get Stub bit
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return stub bit?
     */
    bool StubBit(uint8_t pHybridId, uint8_t pCbcId) override;
    /*!
     * \brief Get a vector of Stubs - will be empty for Cbc2
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     */
    ClusterCollection<EventStub, 5> StubVector(uint8_t pHybridId, uint8_t pCbcId);
    /*!
     * \brief Function to count the Hits in this event
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return number of hits
     */
    uint32_t GetNHits(uint8_t pHybridId, uint8_t pCbcId) override;
    /*!
     * \brief Function to get a sparsified hit vector
     * \param pHybridId : Hybrid Id
     * \param pCbcId : Cbc Id
     * \return vector with hit channels (row, col)
     */
    std::vector<std::pair<uint16_t, uint16_t>> GetHits(uint8_t pHybridId, uint8_t pCbcId) override;
    ClusterCollection<Cluster2S, 31>           getClusters(uint8_t pHybridId, uint8_t pCbcId);
    ClusterCollection<StripClusterPS, 32>      GetStripClusters(uint8_t pHybridId, uint8_t pMPAId);
    ClusterCollection<PixelClusterPS, 32>      GetPixelClusters(uint8_t pHybridId, uint8_t pMPAId);

    void fillChipDataContainer(ChipDataContainer* chipContainer, const std::shared_ptr<ChannelGroupBase> testChannelGroup, uint8_t hybridId) override;

    uint16_t L1Status(uint8_t pHybridId);
    uint32_t L1Id(uint8_t pHybridId, uint8_t pReadoutChipId);
    uint32_t BxId(uint8_t pHybridId) override;
    uint16_t StubStatus(uint8_t pHybridId);

  private:
    // figure out how to switch between various hybrid types here
    std::vector<uint8_t>        fChipToCicMapping2S{0, 1, 2, 3, 7, 6, 5, 4};  // Index Hybrid Hybrid Id , Value CIC Hybrid Id
    std::vector<uint8_t>        fChipToCicMappingPSR{6, 7, 3, 2, 1, 0, 4, 5}; //  Index Hybrid Hybrid Id , Value CIC Hybrid Id
    std::vector<uint8_t>        fChipToCicMappingPSL{1, 0, 4, 5, 6, 7, 3, 2}; // Index hybrid Hybrid Id , Value CIC Hybrid Id
    const std::vector<uint8_t>* getChipToCicMapping(uint16_t theHybridId) const;

    // Index chip ID for CIC, value chip ID for I2C
    std::vector<uint8_t>        fCicToChipMapping2S{0, 1, 2, 3, 7, 6, 5, 4};
    std::vector<uint8_t>        fCicToChipMappingPSR{5, 4, 3, 2, 6, 7, 0, 1};
    std::vector<uint8_t>        fCicToChipMappingPSL{1, 0, 7, 6, 2, 3, 4, 5}; // Index hybrid Hybrid Id , Value CIC Hybrid Id
    const std::vector<uint8_t>* getCicToChipMapping(uint16_t theHybridId) const;

    static BoardDataContainer                            fDecodedL1Event;
    static BoardDataContainer                            fDecodedStubEvent;
    static std::array<uint32_t, NUMBER_OF_CIC_PORTS * 9> fTheChipDataVector;
    static bool                                          ifAreDecodedEventContainersReady;
    static uintptr_t                                     fLastEventDecodedPointer;

    uint8_t     fTLUenabled = 0;
    static bool fIs2S;
    static bool fIsSparsified;

    std::vector<uint32_t>             fLocalData;
    const Ph2_HwDescription::BeBoard* fBoard;

    // mapped id
    // takes chip id on the hybrid
    // returns chip id in the CIC
    inline uint8_t getIdForCic(uint8_t pHybridId, uint8_t pReadoutChipId) const
    {
        auto theChipToCicMapping = getChipToCicMapping(pHybridId);
        return (*theChipToCicMapping)[pReadoutChipId % 8];
    }

    uint16_t decodeHybridL1Event(HybridDataContainer* theHybridL1EventContainer, std::vector<uint32_t>::const_iterator dataStartIterator);
    uint16_t decodeHybridStubEvent(HybridDataContainer* theHybridStubEventContainer, std::vector<uint32_t>::const_iterator dataStartIterator);

    void print();
};
} // namespace Ph2_HwInterface
#endif
