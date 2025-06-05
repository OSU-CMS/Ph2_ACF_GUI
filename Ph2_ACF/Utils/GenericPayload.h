#ifndef __GENERICPAYLOAD__
#define __GENERICPAYLOAD__

#include <bitset>
#include <iostream>
#include <vector>

#include "easylogging++.h"

#define WORDSIZE 64

template <typename T>
std::vector<T> split_vec64(std::vector<uint64_t> pVec)
{
    size_t aSize     = sizeof(T);
    size_t aSizeBits = 8 * aSize;
    size_t tSize     = sizeof(uint64_t);

    std::vector<T> cVec;

    for(auto pWord: pVec)
    {
        if(tSize > aSize)
        {
            for(size_t i = (tSize / aSize); i > 0; i--)
            {
                size_t   cShift = (i - 1) * aSizeBits;
                uint64_t cMask  = ((uint64_t)(1) << aSizeBits) - 1;
                T        cWord  = (pWord >> cShift) & cMask;
                cVec.push_back(cWord); // add a mask
            }
        }
        else if(aSize == tSize)
            cVec.push_back((T)pWord);
    }

    return cVec;
}

// template<typename A> class GenericPayload
class GenericPayload
{
    // class to hold bits in vector of 64 bits
    // if a new word is needed for an additional bit,
    // the underlying container will be expanded by a word

  private:
    std::vector<uint64_t> fData;
    size_t                fBitCount;
    size_t                fWordIndex;
    size_t                fWriteBitIndex;

  public:
    GenericPayload() : fData(1, 0), fBitCount(0), fWordIndex(0), fWriteBitIndex(0) {}
    ~GenericPayload() { fData.clear(); }

    size_t Words() const
    {
        if(fWordIndex + 1 != fData.size()) LOG(ERROR) << "Error, there is a problem with the size";

        return fWordIndex + 1;
    }

    size_t WordSize() const { return WORDSIZE; }

    size_t Bits() const { return fBitCount; }

    size_t WritePosition() const { return fWriteBitIndex; }

    template <typename T>
    void print() const
    {
        LOG(INFO) << "Word Size: " << WORDSIZE << " Container Size: " << Words();
        std::vector<T> cVec = split_vec64<T>(fData);

        for(auto cWord: cVec)
        {
            if(sizeof(T) == 1) std::cout << std::bitset<8>(cWord) << std::endl;

            if(sizeof(T) == 2) std::cout << std::bitset<16>(cWord) << std::endl;

            if(sizeof(T) == 4) std::cout << std::bitset<32>(cWord) << std::endl;

            if(sizeof(T) == 8) std::cout << std::bitset<64>(cWord) << std::endl;
        }
    }

    size_t get_current_write_position() const
    {
        LOG(DEBUG) << "Write pointer (for next bit) currently pointing to: Word " << fWordIndex << " Bit (in word) " << fWriteBitIndex << " Bit (global) " << +fBitCount;
        return fWriteBitIndex;
    }

    template <typename T>
    std::vector<T> Data()
    {
        return split_vec64<T>(fData);
    }

    // the next three methods allow to set, unset and test bits at any position
    // no counters need to be incremented
    void set(size_t pPosition)
    {
        size_t cWord = pPosition / WORDSIZE;
        pPosition    = WORDSIZE - 1 - pPosition;
        size_t cBit  = pPosition % WORDSIZE;

        if(cWord > fData.size())
        {
            throw std::out_of_range("Bit index out of range");
            return;
        }
        else
            fData.at(cWord) |= (uint64_t)1 << cBit;
    }

    void unset(size_t pPosition)
    {
        size_t cWord = pPosition / WORDSIZE;
        pPosition    = WORDSIZE - 1 - pPosition;
        size_t cBit  = pPosition % WORDSIZE;

        if(cWord > fData.size())
        {
            throw std::out_of_range("Bit index out of range");
            return;
        }
        else
            fData.at(cWord) &= ~((uint64_t)1 << cBit);
    }

    void toggle(size_t pPosition)
    {
        size_t cWord = pPosition / WORDSIZE;
        pPosition    = WORDSIZE - 1 - pPosition;
        size_t cBit  = pPosition % WORDSIZE;

        if(cWord > fData.size())
        {
            throw std::out_of_range("Bit index out of range");
            return;
        }
        else
            fData.at(cWord) ^= ((uint64_t)1 << cBit);
    }

    bool test(size_t pPosition) const
    {
        size_t cWord = pPosition / WORDSIZE;
        pPosition    = WORDSIZE - 1 - pPosition;
        size_t cBit  = pPosition % WORDSIZE;

        if(cWord > fData.size())
        {
            throw std::out_of_range("Bit index out of range");
            return false;
        }
        else
        {
            if((fData.at(cWord) >> cBit) & (uint64_t)1)
                return true;
            else
                return false;
        }
    }

    // inline bool& operator[] (uint32_t pIndex)
    //{
    // if (this->test (pPosition) ) ;

    //}

    const bool operator[](size_t pIndex) const { return this->test(pIndex); }

    template <typename T>
    void operator+(T pWord)
    {
        this->append(pWord);
    }

    void padZero(size_t pNZeros) { this->reserve(pNZeros); }

    template <typename T>
    void append(T pWord, int pNLSBs = -1)
    {
        size_t pWidth = sizeof(T) * 8;

        if(pNLSBs != -1)
        {
            pWidth = pNLSBs;
            pWord &= (T)pWord & (((T)1 << pNLSBs) - 1);
        }

        size_t cFreeBits = WORDSIZE - (fBitCount % WORDSIZE);
        // incremtn the write bit index
        fWriteBitIndex = (fWriteBitIndex + pWidth) % WORDSIZE;

        // if i am not in the first word bout would need to start a new word for the new data:
        if(cFreeBits == 64 && fBitCount != 0)
        {
            fWordIndex++;
            fData.push_back(0);
        }

        // on the other hand, if the bits i want to insert exceed the free bit count
        // I also need to expand
        else if(pWidth > cFreeBits)
        {
            fWordIndex++;
            fData.push_back(0);
            fData.at(fWordIndex - 1) |= (uint64_t)pWord >> fWriteBitIndex;
        }

        // ok, now have inserted enough words and pointed myself to the correct word
        // case1 i either fit in the new word or the present one
        fData.at(fWordIndex) |= (uint64_t)pWord << (WORDSIZE - fWriteBitIndex);

        // case 2: i need to span both words and it has happend above
        // incremtnt the total bitcount
        fBitCount += pWidth;
    }

    void append(bool pWord, int pNLSBs = -1)
    {
        size_t cFreeBits = WORDSIZE - (fBitCount % WORDSIZE);

        // if i am not in the first word bout would need to start a new word for the new data:
        if(cFreeBits == 64 && fBitCount != 0)
        {
            fWordIndex++;
            fData.push_back(0);
        }

        // on the other hand, if the bits i want to insert exceed the free bit count
        // I also need to expand
        else if(1 > cFreeBits)
        {
            fWordIndex++;
            fData.push_back(0);
        }

        fWriteBitIndex = (fWriteBitIndex + 1) % WORDSIZE;

        if(pWord) fData.at(fWordIndex) |= (uint64_t)1 << (WORDSIZE - fWriteBitIndex);

        fBitCount++;
    }

    // void append (bool pWord, int pNLSBs = -1)
    //{
    //// only open a new word if the index points to  the MSB of a word
    //// and it is not the first word as that already exists from construction
    // if (fWriteBitIndex == 0 && fWordIndex != 0)
    //{
    // fWordIndex++;
    // fData.push_back (0);
    // fWriteBitIndex = (fWriteBitIndex + 1) % WORDSIZE;

    // if (pWord) fData.at (fWordIndex) |= (uint64_t) 1 << (WORDSIZE - fWriteBitIndex);

    //}
    // else
    //{
    // fWriteBitIndex = (fWriteBitIndex + 1) % WORDSIZE;

    // if (pWord) fData.at (fWordIndex) |= (uint64_t) 1 << (WORDSIZE - fWriteBitIndex);
    //}

    // fBitCount++;
    //}

    template <typename T>
    void insert(T pWord, size_t pPosition, int pNLSBs = -1)
    {
        // first determine the number of bits I have to insert
        size_t cWidth = sizeof(T) * 8;

        if(pNLSBs != -1)
        {
            cWidth = pNLSBs;
            pWord &= (T)pWord & (((T)1 << pNLSBs) - 1);
        }

        // now determine the index from of the bit where I have to insert
        size_t cWord = pPosition / WORDSIZE;
        // pPosition = WORDSIZE - 1 - pPosition;
        size_t cBit = (WORDSIZE - 1 - pPosition) % WORDSIZE;

        // now figure out how many bits  I have left at the end of the last word
        // this determines if I need to add a word to the vector
        size_t cFreeBits = WORDSIZE - (fBitCount % WORDSIZE);

        // if the bitcount is a multipele of 64, that means 64 free bits but in the next word
        // this i not the case if the bit count is 0 cause then the 0th word is already there and unused
        // so I need to add a word to hold my new data
        // all of this is true if I insert at least one bit which is always the case
        if(cFreeBits == 64 && fBitCount != 0)
        {
            fWordIndex++;
            fData.push_back(0);
        }

        // on the other hand, if the bits i want to insert exceed the free bit count
        // I also need to expand
        else if(cWidth > cFreeBits)
        {
            fWordIndex++;
            fData.push_back(0);
        }

        // now make some space for my new bits
        // to do this, first mask out the part that remains
        // in a temporary word
        // then mask out the part that moves and shift it right by the width of the new data
        uint64_t cShift = cWidth; // + (WORDSIZE - cBit - 1);
        // uint64_t cMask = (uint64_t (1) << (cBit + 1) ) - 1;
        uint64_t cMask           = (cBit == 63) ? 0xFFFFFFFFFFFFFFFF : (uint64_t(1) << (cBit + 1)) - 1;
        uint64_t cComplimentMask = ~cMask;

        // std::cout << "position: " << pPosition << " width: " << +cWidth << " free bits: " << +cFreeBits << " insert
        // in word: " << +cWord << " at bit(in word) " << +cBit << " shift right by " << +cShift << " and use a mask for
        // " << +cBit + 1 << " widht = " << std::hex << cMask << " and a complement  of " << cComplimentMask << std::dec
        // << std::endl; first, get the part of cWord that remains, we need it later
        uint64_t cTmpWord = fData.at(cWord) & cComplimentMask;
        // compute the part that falls out to the right
        // this is the default shift
        size_t   cFalloutShift = WORDSIZE - cShift;
        uint64_t cFalloutMask  = (uint64_t(1) << cShift) - 1;
        uint64_t cFallout      = (fData.at(cWord) & cFalloutMask) << cFalloutShift;

        // this is the famous wrap case
        // the bit position(0 at the left)+ the width have to be greater than the word size
        if(pPosition % 64 + cWidth > WORDSIZE)
        {
            size_t   cNBitsFirstWord  = cBit + 1;
            size_t   cNBitsSecondWord = cWidth - cNBitsFirstWord;
            uint64_t cSecondMask      = (uint64_t(1) << cNBitsSecondWord) - 1;
            uint64_t cFirstMask       = ~cSecondMask;
            uint64_t pFirstVal        = (pWord & cFirstMask) >> cNBitsSecondWord;
            uint64_t pSecondVal       = pWord & cSecondMask;
            // std::cout << "Mask 1/2: " << std::hex << cFirstMask << " " << cSecondMask << std::dec << std::endl;
            // std::cout << "Value 1/2: " << std::bitset<16> (pFirstVal) << " " << std::bitset<16> (pSecondVal) <<
            // std::dec << std::endl; std::cout << "1/2: " << + cNBitsFirstWord << " " << +cNBitsSecondWord << std::endl;
            uint64_t cTmpFallout = ((fData.at(cWord)) & cMask) << (WORDSIZE - cBit - 1);
            // dont forget to modify the original word
            fData.at(cWord) = cTmpWord | (fData.at(cWord) & cMask) >> cShift;
            // or the new data word with the MSBs of the new data
            fData.at(cWord) |= pFirstVal; // << (cBit - cWidth + 1);
            // before I change the new word, I still need to get the fallout, this time with the original formula
            cFallout = (fData.at(cWord + 1) & cFalloutMask) << cFalloutShift;
            // now shift the next word right by the above fallout and mask all but the fallout width beginning from the
            // MSB uint64_t cTmpMask = (uint64_t (1) << (WORDSIZE - (cNBitsSecondWord ) ) ) - 1; std::cout << "shift for
            // next word: " << (cWidth - (cBit + 1) ) << " mask for shifiting " << std::hex << cTmpMask << std::dec <<
            // std::endl;
            fData.at(cWord + 1) = (cTmpFallout | ((fData.at(cWord + 1)) >> (cNBitsFirstWord))) >> (cNBitsSecondWord);
            // finally also slide the LSBs of pWord into this word
            fData.at(cWord + 1) |= pSecondVal << (WORDSIZE - cNBitsSecondWord);
            // increment the cWord counter so the fallout handling loop can do it's dirty business
            // starting at the right location
            cWord += 1;
        }
        else
        {
            // rightshift only the part that has to move by the widht of the new data
            fData.at(cWord) = cTmpWord | (fData.at(cWord) & cMask) >> cShift;
            // now shift the data into position and or
            fData.at(cWord) |= (uint64_t)pWord << (cBit - cWidth + 1);
        }

        // std::cout << "Word " << +cWord << "Fallout " << std::bitset<64> (cFallout) << std::endl;

        // now, rightshift every word starting from cWord+1 by the space needed for my new data
        for(size_t word = cWord + 1; word < fData.size(); word++)
        {
            // assign the fallout of the previous word to cTmpWord
            cTmpWord = cFallout;
            // now compute the fallout of this word before any modification
            cFallout = (fData.at(word) & ((uint64_t(1) << cShift) - 1)) << (WORDSIZE - cShift);
            // now right shift this word by cShift to generate space for the fallout from the previous line and or it
            // with cTmpWord which is the fallout from the previous line
            fData.at(word) = cTmpWord | fData.at(word) >> cShift;
            // std::cout << "Word " << +word << "Fallout " << std::bitset<64> (cFallout) << std::endl;
        }
    }

    template <typename T>
    void prepend(T pWord, int pNLSBs = -1)
    {
        this->insert(pWord, 0, pNLSBs);
    }

    void appendD19CData(uint32_t pEvenWord, uint32_t pOddWord, int pNLSBs = -1)
    {
        // this method is specific to the d19c Firmware as it spits out the data
        // sensor by sensor, so even channels first and then odd channels
        // the process is called morton encoding and the code comes from here:
        // https://stackoverflow.com/questions/22101273/how-to-combine-two-16bit-words-into-one-32bit-word-bit-by-bit-efficiently
        uint64_t A, B;
        A = ((pOddWord & 0x00000000FFFF0000ull) << 16) | (pOddWord & 0x000000000000FFFFull);
        A = ((A & 0x0000FF000000FF00ull) << 8) | (A & 0x000000FF000000FFull);
        A = ((A & 0xF0F0F0F0F0F0F0F0ull) << 4) | (A & 0x0F0F0F0F0F0F0F0Full);
        A = ((A & 0xCCCCCCCCCCCCCCCCull) << 2) | (A & 0x0333333333333333ull);
        A = ((A & 0xAAAAAAAAAAAAAAAAull) << 1) | (A & 0x5555555555555555ull);

        B = ((pEvenWord & 0x00000000FFFF0000ull) << 16) | (pEvenWord & 0x000000000000FFFFull);
        B = ((B & 0x0000FF000000FF00ull) << 8) | (B & 0x000000FF000000FFull);
        B = ((B & 0xF0F0F0F0F0F0F0F0ull) << 4) | (B & 0x0F0F0F0F0F0F0F0Full);
        B = ((B & 0xCCCCCCCCCCCCCCCCull) << 2) | (B & 0x0333333333333333ull);
        B = ((B & 0xAAAAAAAAAAAAAAAAull) << 1) | (B & 0x5555555555555555ull);

        uint64_t cResultWord = A | (B << 1);
        this->append(cResultWord, pNLSBs);
    }

  private:
    void reserve(size_t pNBits)
    {
        size_t cFreeBits = WORDSIZE - (fBitCount % WORDSIZE);

        // if i am not in the first word bout would need to start a new word for the new data:
        if(cFreeBits == 64 && fBitCount != 0)
        {
            fWordIndex++;
            fData.push_back(0);
        }

        // on the other hand, if the bits i want to insert exceed the free bit count
        // I also need to expand
        else if(pNBits > cFreeBits)
        {
            fWordIndex++;
            fData.push_back(0);
        }

        fWriteBitIndex = (fWriteBitIndex + pNBits) % WORDSIZE;
        fBitCount += pNBits;
    }

    template <typename T>
    void insert_reserved(T pWord, size_t pPosition, int pNLSBs = -1)
    {
        // inserts  MSB of pWord at position pPosition. pPosition is starting at word 0, MSB with 0
        // it's an absolute bit position
        size_t cWidth = sizeof(T) * 8;

        if(pNLSBs != -1)
        {
            cWidth = pNLSBs;
            pWord &= pWord & (((T)1 << pNLSBs) - 1);
        }

        size_t cWord = pPosition / WORDSIZE;
        size_t cBit  = (WORDSIZE - pPosition) % WORDSIZE;

        if(cWord > fData.size())
        {
            throw std::out_of_range("Bit index out of range");
            // std::cout << "Error, this bit does not exist!" << std::endl;
            return;
        }

        if(pPosition + cWidth > WORDSIZE)
        {
            if(cWord + 1 > fData.size())
            {
                throw std::out_of_range("Bit index out of range");
                // std::cout << "Error, this bit does not exist!" << std::endl;
                return;
            }

            cBit = (cBit + (WORDSIZE - cWidth)) % WORDSIZE;

            if(cWord + 1 == fData.size())
            {
                fWordIndex++;
                fData.push_back(0);
            }

            fData.at(cWord) |= (uint64_t)pWord >> (WORDSIZE - cBit);
            fData.at(cWord + 1) |= (uint64_t)pWord << cBit;
        }
        else
        {
            cBit -= cWidth;
            fData.at(cWord) |= (uint64_t)pWord << cBit;
        }
    }
};

#endif
