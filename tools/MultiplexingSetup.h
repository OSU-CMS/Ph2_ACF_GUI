/*!
 *
 * \file MultiplexingSetup.h
 * \brief MultiplexingSetup class, MultiplexingSetup of the hardware
 * \author Georg AUZINGER
 * \date 13 / 11 / 15
 *
 * \Support : inna.makarenko@cern.ch
 *
 */

#ifndef MultiplexingSetup_h__
#define MultiplexingSetup_h__

#include "tools/Tool.h"

#include <map>

class MultiplexingSetup : public Tool
{
  public:
    MultiplexingSetup();
    ~MultiplexingSetup();

    void Initialise();
    void Disconnect();
    void ConfigureSingleCard(uint8_t pBackPlaneId, uint8_t pCardId);
    void ConfigureAll();
    void Scan();
    void Power(bool pEnable = true);
    // void writeObjects();

    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;

    /*! \brief printout which boards have cards connected
     * \param availBPCards map of boards and cards (see std::map<int, std::vector<int>>
     * parseAvailableBackplanesCards(int availBPCards))
     */
    std::map<int, std::vector<int>> getAvailableCards(bool filterBoardsWithoutCards = true);
    void                            printAvailableCards();

  private:
    std::map<int, std::vector<int>> fAvailable;
    uint32_t                        fAvailableCards;
    bool                            fInterlockEnabled;

    void parseAvailable(bool filterBoardsWithoutCards = true)
    {
        fAvailable.clear();
        // copy "numbits" from "buf" starting at position "at"
        auto copybits = [](int buf, int at, int numbits)
        {
            int mask = ((~0u) >> (sizeof(int) * 8 - numbits)) << at; // 2nd aproach
            return ((buf & mask) >> at);
        };

        // split original integer into chunks of 5 bits
        std::vector<int> list_of_bp_and_cards;
        list_of_bp_and_cards.push_back(copybits(fAvailableCards, 15, 5)); // 0th board
        list_of_bp_and_cards.push_back(copybits(fAvailableCards, 10, 5)); // 1st board
        list_of_bp_and_cards.push_back(copybits(fAvailableCards, 5, 5));  // 2nd board
        list_of_bp_and_cards.push_back(copybits(fAvailableCards, 0, 5));  // 3rd board

        // iterate over
        for(unsigned int iBP = 0; iBP < list_of_bp_and_cards.size(); ++iBP)
        {
            // test leftmost bit corresponding to availability of the board
            if(list_of_bp_and_cards[iBP] & (1 << 4))
            {
                // if board is available
                fAvailable[iBP] = std::vector<int>{};
                // test cards state for given board
                for(unsigned int iCard = 0; iCard <= 3; iCard++)
                {
                    if(list_of_bp_and_cards[iBP] & (1 << (3 - iCard)))
                    { // if card is ON
                        fAvailable.at(iBP).push_back(iCard);
                    }
                }
            }
        }

        if(filterBoardsWithoutCards)
        {
            for(auto itBPCard = fAvailable.cbegin(); itBPCard != fAvailable.cend();)
            {
                if(itBPCard->second.empty())
                    fAvailable.erase(itBPCard++);
                else
                    ++itBPCard;
            }
        }
    }
};

#endif
