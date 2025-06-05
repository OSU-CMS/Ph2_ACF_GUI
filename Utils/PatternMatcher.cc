#include "Utils/PatternMatcher.h"
#include "Utils/Utilities.h"
#include <bitset>
#include <iostream>

PatternMatcher::PatternMatcher() {}

PatternMatcher::~PatternMatcher() {}

PatternMatcher::PatternMatcher(const PatternMatcher& thePatternMatcher)
    : fPatternNumberOfBits(thePatternMatcher.fPatternNumberOfBits)
    , fPatternNumberOfMaskedBits(thePatternMatcher.fPatternNumberOfMaskedBits)
    , fPatternAndMaskVector(thePatternMatcher.fPatternAndMaskVector)
{
}

PatternMatcher& PatternMatcher::operator=(const PatternMatcher& thePatternMatcher)
{
    fPatternNumberOfBits       = thePatternMatcher.fPatternNumberOfBits;
    fPatternNumberOfMaskedBits = thePatternMatcher.fPatternNumberOfMaskedBits;
    fPatternAndMaskVector.assign(thePatternMatcher.fPatternAndMaskVector.begin(), thePatternMatcher.fPatternAndMaskVector.end());
    return *this;
}

void PatternMatcher::addToPattern(uint32_t thePattern, uint32_t thePatternMask, uint8_t thePatternBitLenght)
{
    fPatternNumberOfMaskedBits = 0;

    uint8_t numberOfBitsInWord = (sizeof(uint32_t)) * 8;
    uint8_t availableBitNumber = numberOfBitsInWord - fPatternNumberOfBits % numberOfBitsInWord;
    if(availableBitNumber == numberOfBitsInWord) fPatternAndMaskVector.push_back({0, 0});
    auto& thePatternCurrentWord = fPatternAndMaskVector.back();

    if(availableBitNumber >= thePatternBitLenght)
    {
        uint8_t bitShift = availableBitNumber - thePatternBitLenght;
        thePatternCurrentWord.first |= (thePattern << bitShift);
        thePatternCurrentWord.second |= (thePatternMask << bitShift);
    }
    else
    {
        uint8_t numberOfRemainingBits = thePatternBitLenght - availableBitNumber;
        thePatternCurrentWord.first |= (thePattern >> numberOfRemainingBits);
        thePatternCurrentWord.second |= (thePatternMask >> numberOfRemainingBits);

        uint32_t newWord = thePattern << (numberOfBitsInWord - numberOfRemainingBits);
        uint32_t newMask = thePatternMask << (numberOfBitsInWord - numberOfRemainingBits);

        fPatternAndMaskVector.push_back({newWord, newMask});
    }
    fPatternNumberOfBits += thePatternBitLenght;

    // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Pattern  -> ";
    // for(const auto word : fPatternAndMaskVector) std::cout << std::hex << word.first << std::dec << " ";
    // std::cout << std::endl;
}

void PatternMatcher::updatePattern(uint32_t thePattern, uint32_t thePatternMask, uint8_t thePatternBitLenght, uint32_t firstBitPosition)
{
    if(firstBitPosition + firstBitPosition > fPatternNumberOfBits)
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] firstBitPosition + firstBitPosition (" << (firstBitPosition + firstBitPosition)
                  << ") cannot be greater then fPatternNumberOfMaskedBits (" << fPatternNumberOfMaskedBits << "). Aborting" << std::endl;
        abort();
    }

    size_t positionOfFirstWord  = (firstBitPosition - 1) / 32;
    size_t positionOfSecondWord = (firstBitPosition - 1 + thePatternBitLenght) / 32;
    if((firstBitPosition + thePatternBitLenght) % 32 == 0) --positionOfSecondWord;

    uint8_t  numberOfBitsToSkipFirstWord  = (firstBitPosition - 1) % 32;
    uint32_t bitToSkipMaskFirstWord       = ~0u << (32 - numberOfBitsToSkipFirstWord);
    uint8_t  numberOfBitsToSkipSecondWord = 32 - (firstBitPosition - 1 + thePatternBitLenght) % 32;
    uint32_t bitToSkipMaskSecondWord      = (~0u) >> (32 - numberOfBitsToSkipSecondWord);

    if(positionOfSecondWord > positionOfFirstWord)
    {
        uint32_t theFirstNewWord               = (thePattern)&bitToSkipMaskFirstWord;
        uint32_t theFirstNewMask               = (thePatternMask)&bitToSkipMaskFirstWord;
        auto&    theCurrentPatternAndMaskFirst = fPatternAndMaskVector.at(positionOfFirstWord);
        theCurrentPatternAndMaskFirst.first &= bitToSkipMaskFirstWord;
        theCurrentPatternAndMaskFirst.first |= theFirstNewWord;
        theCurrentPatternAndMaskFirst.second &= bitToSkipMaskFirstWord;
        theCurrentPatternAndMaskFirst.second |= theFirstNewMask;

        uint32_t theSecondNewWord               = (thePattern << numberOfBitsToSkipSecondWord) & bitToSkipMaskSecondWord;
        uint32_t theSecondNewMask               = (thePattern << numberOfBitsToSkipSecondWord) & bitToSkipMaskSecondWord;
        auto&    theCurrentPatternAndMaskSecond = fPatternAndMaskVector.at(positionOfSecondWord);
        theCurrentPatternAndMaskSecond.first &= bitToSkipMaskSecondWord;
        theCurrentPatternAndMaskSecond.first |= theSecondNewWord;
        theCurrentPatternAndMaskSecond.second &= bitToSkipMaskSecondWord;
        theCurrentPatternAndMaskSecond.second |= theSecondNewMask;
    }
    else
    {
        uint32_t theNewWord               = thePattern << numberOfBitsToSkipSecondWord;
        uint32_t theNewMask               = thePatternMask << numberOfBitsToSkipSecondWord;
        uint32_t theFullMask              = bitToSkipMaskFirstWord | bitToSkipMaskSecondWord;
        auto&    theCurrentPatternAndMask = fPatternAndMaskVector.at(positionOfFirstWord);
        theCurrentPatternAndMask.first &= theFullMask;
        theCurrentPatternAndMask.first |= theNewWord;
        theCurrentPatternAndMask.second &= theFullMask;
        theCurrentPatternAndMask.second |= theNewMask;
    }
}

bool PatternMatcher::isMatched(const std::vector<uint32_t>& theWordVector) const
{
    if(theWordVector.size() < fPatternAndMaskVector.size()) return false;
    // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Incoming -> ";
    // for(const auto& word : theWordVector) std::cout << std::hex << word << std::dec << " ";
    // std::cout << std::endl;

    // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Pattern  -> ";
    // for(const auto& word : fPatternAndMaskVector) std::cout << std::hex << word.first << std::dec << " ";
    // std::cout << std::endl;

    for(size_t patternIndex = 0; patternIndex < fPatternAndMaskVector.size(); ++patternIndex)
    {
        auto thePatternAndMaskWord = fPatternAndMaskVector[patternIndex];
        auto theIncomigMaskedWord  = theWordVector[patternIndex] & thePatternAndMaskWord.second;
        auto thePatternMaskedWord  = thePatternAndMaskWord.first & thePatternAndMaskWord.second;
        if(theIncomigMaskedWord != thePatternMaskedWord) return false;
    }

    return true;
}

uint32_t PatternMatcher::countMatchingBits(const std::vector<uint32_t>& theWordVector) const
{
    uint32_t numberOfMatchingBits = 0;

    for(size_t index = 0; index <= fPatternAndMaskVector.size(); ++index)
    {
        if(index >= theWordVector.size()) break;
        uint32_t        patternAndWordXOR = theWordVector[index] ^ fPatternAndMaskVector[index].first;
        std::bitset<32> patternAndWordXORBitset(patternAndWordXOR);
        std::bitset<32> maskBitset(fPatternAndMaskVector[index].second);
        patternAndWordXORBitset.flip();
        auto patternAndWordXORBitsetMasked = patternAndWordXORBitset & maskBitset;
        numberOfMatchingBits += patternAndWordXORBitsetMasked.count();
    }

    return numberOfMatchingBits;
}

std::vector<uint32_t> PatternMatcher::getPattern() const
{
    std::vector<uint32_t> thePatternVector;
    for(const auto& thePatternAndMaskWord: fPatternAndMaskVector) thePatternVector.push_back(thePatternAndMaskWord.first);
    return thePatternVector;
}

std::vector<uint32_t> PatternMatcher::getMask() const
{
    std::vector<uint32_t> theMaskVector;
    for(const auto& thePatternAndMaskWord: fPatternAndMaskVector) theMaskVector.push_back(thePatternAndMaskWord.second);
    return theMaskVector;
}

void PatternMatcher::maskStubFor2Skickoff()
{
    std::vector<uint32_t> theMaskVector(fPatternAndMaskVector.size(), 0);

    size_t lineCounter = 0;
    for(auto& theMask: theMaskVector)
    {
        for(int8_t bitCounter = sizeof(uint32_t) * 8 - 1; bitCounter >= 0; --bitCounter)
        {
            if(lineCounter == 4)
            {
                lineCounter = 0;
                continue;
            }
            theMask |= 0x1 << bitCounter;
            ++lineCounter;
        }
    }

    for(uint8_t patternIndex = 0; patternIndex < fPatternAndMaskVector.size(); ++patternIndex) { fPatternAndMaskVector[patternIndex].second &= theMaskVector[patternIndex]; }
}

uint32_t PatternMatcher::getNumberOfMaskedBits()
{
    if(fPatternNumberOfMaskedBits != 0) return fPatternNumberOfMaskedBits;

    for(auto thePatterAndMask: fPatternAndMaskVector)
    {
        std::bitset<32> theMaskBitset(thePatterAndMask.second);
        fPatternNumberOfMaskedBits += theMaskBitset.count();
    }

    return fPatternNumberOfMaskedBits;
}

void PatternMatcher::clear()
{
    fPatternNumberOfBits = 0;
    fPatternAndMaskVector.clear();
}

bool PatternMatcher::isSubsetMatched(const std::vector<uint32_t>& theWordVector, uint32_t firstBitPosition, uint32_t numberOfBitsToMatch) const
{
    PatternMatcher theSubsetPatternMatcher;
    size_t         positionOfFirstWord = (firstBitPosition - 1) / 32;
    size_t         positionOfLastWord  = (firstBitPosition - 1 + numberOfBitsToMatch) / 32;
    if((firstBitPosition + numberOfBitsToMatch) % 32 == 0) --positionOfLastWord;

    // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] positionOfFirstWord = " << positionOfFirstWord << std::endl;
    // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] positionOfLastWord = " << positionOfLastWord << std::endl;

    uint8_t  numberOfBitsToSkipFirstWord = (firstBitPosition - 1) % 32;
    uint32_t bitToSkipMaskFirstWord      = ((~0u) << numberOfBitsToSkipFirstWord) >> numberOfBitsToSkipFirstWord;
    uint8_t  numberOfBitsToSkipLastWord  = 32 - (firstBitPosition - 1 + numberOfBitsToMatch) % 32;
    uint32_t bitToSkipMaskLastWord       = (~0u) << numberOfBitsToSkipLastWord;

    // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] numberOfBitsToSkipFirstWord = " << +numberOfBitsToSkipFirstWord << std::endl;
    // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] numberOfBitsToSkipLastWord = " << +numberOfBitsToSkipLastWord << std::endl;
    // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] bitToSkipMaskFirstWord = " << std::hex << bitToSkipMaskFirstWord << std::dec << std::endl;
    // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] bitToSkipMaskLastWord = " << std::hex << bitToSkipMaskLastWord << std::dec << std::endl;

    if(positionOfLastWord > positionOfFirstWord)
    {
        theSubsetPatternMatcher.addToPattern(fPatternAndMaskVector[positionOfFirstWord].first, fPatternAndMaskVector[positionOfFirstWord].second & bitToSkipMaskFirstWord, 32);
        for(size_t wordIndex = positionOfFirstWord + 1; wordIndex < positionOfLastWord - 1; ++wordIndex)
        {
            theSubsetPatternMatcher.addToPattern(fPatternAndMaskVector[wordIndex].first, fPatternAndMaskVector[wordIndex].second, 32);
        }
        theSubsetPatternMatcher.addToPattern(fPatternAndMaskVector[positionOfLastWord].first, fPatternAndMaskVector[positionOfLastWord].second & bitToSkipMaskLastWord, 32);
    }
    else
    {
        theSubsetPatternMatcher.addToPattern(fPatternAndMaskVector[positionOfLastWord].first, fPatternAndMaskVector[positionOfLastWord].second & bitToSkipMaskFirstWord & bitToSkipMaskLastWord, 32);
    }

    std::vector<uint32_t> theSubsetWordVector(theWordVector.begin() + positionOfFirstWord, theWordVector.begin() + positionOfLastWord + 1);
    // std::cout << "Stub data received    " << getPatternPrintout(theSubsetWordVector, 2) << std::endl;
    // std::cout << "Stub pattern expected " << getPatternPrintout(theSubsetPatternMatcher.getPattern(), 2) << std::endl;
    // std::cout << "Stub pattern mask     " << getPatternPrintout(theSubsetPatternMatcher.getMask(), 2) << std::endl;

    return theSubsetPatternMatcher.isMatched(theSubsetWordVector);
}

void PatternMatcher::addTrailingZeros(size_t totalNumberOfBitNeeded)
{
    if(totalNumberOfBitNeeded < fPatternNumberOfBits)
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] totalNumberOfBitNeeded (" << totalNumberOfBitNeeded << ") is less then the number of bits already present in the pattern ("
                  << fPatternNumberOfBits << "). Aborting" << std::endl;
        abort();
    }
    size_t numberOfEmptyBits = totalNumberOfBitNeeded - fPatternNumberOfBits;

    if(numberOfEmptyBits > 0)
    {
        size_t numberOfPaddingBits = numberOfEmptyBits % 32;
        if(numberOfPaddingBits > 0) addToPattern(0x0, 0x0, numberOfPaddingBits);
        for(size_t trailingEmptyWordNumber = 0; trailingEmptyWordNumber < numberOfEmptyBits / 32; ++trailingEmptyWordNumber) { addToPattern(0x0, 0x0, 32); }
    }
}
