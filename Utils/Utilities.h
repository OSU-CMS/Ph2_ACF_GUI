/*
    \file                          Utilities.h
    \brief                         Some objects that might come in handy
    \author                        Nicolas PIERRE
    \version                       1.0
    \date                          10/06/14
    Support :                      mail to : nicolas.pierre@icloud.com
*/

#ifndef UTILITIES_H
#define UTILITIES_H

#include "HWDescription/Definition.h"
#include "Utils/StartInfo.h"

#include <algorithm>
#include <bitset>
#include <cstdio>
#include <ios>
#include <iostream>
#include <istream>
#include <math.h>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

struct Cluster
{
    Cluster(uint8_t theRow, uint8_t theFirstCol, uint8_t theColWidth) : fRow(theRow), fFirstCol(theFirstCol), fColWidth(theColWidth) {}

    Cluster() : fRow(0), fFirstCol(0), fColWidth(0) {}

    uint8_t fRow;
    uint8_t fFirstCol;
    uint8_t fColWidth;
};

struct Stub
{
    Stub(uint8_t theSeed, uint8_t theBend, uint8_t theZ) : fSeed(theSeed), fBend(theBend), fZ(theZ) {}

    Stub() : fSeed(0), fBend(0), fZ(0) {}

    uint8_t fSeed;
    uint8_t fBend;
    uint8_t fZ;
};

template <typename... Args>
std::string string_format(const std::string& format, Args... args)
{
    size_t                  size = snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

template <class T, size_t N>
auto parseString(const std::string& data)
{
    std::array<T, N> result;
    std::transform(data.begin(), data.end(), result.begin(), [](char c) { return (c == 'N' ? 0xFF : c - '0'); });
    return result;
}

std::string getResultDirectoryName(const StartInfo& theStartInfo);

std::string getResultDirectoryName(const int runNumber);

int returnPreviousRunNumber(std::string cFileName);

int returnAndIncreaseRunNumber(std::string cFileName);

/*!
 * \brief get Current Time & Date
 */
const std::string currentDateTime();

/*!
 * \brief Error Function for SCurve Fit
 * \param x: array of values
 * \param p: parameter array
 * \return function value
 */
double MyErf(double* x, double* par);

/*!
 * \brief Gamma peak with charge sharing
 * \param x: array of values
 * \param p: parameter array
 * \return function value
 */
double MyErfc(double* x, double* par);
/*!
 * \brief Gamma peak with charge sharing, compliment of MyErf
 * \param x: array of values
 * \param p: parameter array
 * \return function value
 */
double MyGammaSignal(double* x, double* par);

/*!
 * \brief converts any char array to int by automatically detecting if it is hex or dec
 * \param pRegValue: parsed xml parmaeter char*
 * \return converted integer
 */
uint32_t convertAnyInt(std::string pRegValue);

double convertAnyDouble(std::string pRegValue);

std::vector<float> convertStringToFloatList(std::string theListString);

// Tokenize string
void tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters);

/*! \brief Expand environment variables in string
 * \param s input string
 * \return Result with variables expanded */
std::string expandEnvironmentVariables(std::string s);

/*! \brief Convert value to a string AVOIDING THE USAGE OF std::to_string()
 * \param value input value
 * \return Result string */
template <typename T>
std::string convertToString(T value)
{
    std::stringstream ss;
    ss << +value;
    return ss.str();
}

// Get run number from file
void getRunNumber(const std::string& pPath, int& pRunNumber, bool pIncrement = true);

// CM Noise fitting functions
double hitProbability(double pThreshold);
double binomialPdf(uint32_t n, uint32_t k, double p);
double hitProbabilityFunction(double* pStrips, double* pPar);

template <typename T>
void addNoDuplicate(std::vector<T>& vector, const std::vector<T>& vector2add)
{
    for(auto element: vector2add)
    {
        if(std::find(vector.begin(), vector.end(), element) == vector.end()) vector.push_back(element);
    }
}

template <std::size_t NBITS>
uint8_t reverseBits(uint8_t cValue)
{
    std::bitset<NBITS> cBitset = cValue;
    std::string        cSelect = cBitset.to_string();
    std::reverse(cSelect.begin(), cSelect.end());
    std::bitset<NBITS> cReverseBiset(cSelect);
    // std::cout << std::bitset<8>(cValue) << " reversed " << std::bitset<NBITS>(cReverseBiset) << "\n";
    // cValue = (cValue & 0xF0) >> 4 | (cValue & 0x0F) << 4;
    // cValue = (cValue & 0xCC) >> 2 | (cValue & 0x33) << 2;
    // cValue = (cValue & 0xAA) >> 1 | (cValue & 0x55) << 1;
    return cReverseBiset.to_ulong();
}

// Credit to A.Rossi
template <typename T>
T getLeastSquareSlope(std::vector<T>& x, const std::vector<T>& y)
{
    std::vector<float> cCross(x.size(), 0.);
    std::transform(x.begin(), x.end(), y.begin(), cCross.begin(), std::multiplies<float>{}); // sum(xy)
    auto               cSumCross = std::accumulate(cCross.begin(), cCross.end(), 0.);
    std::vector<float> cSq(x.size(), 0.);
    std::transform(x.begin(), x.end(), x.begin(), cSq.begin(), std::multiplies<float>{}); // sum(x2)
    auto cSumSq = std::accumulate(cSq.begin(), cSq.end(), 0.);
    auto cSumX  = std::accumulate(x.begin(), x.end(), 0.);
    auto cSumY  = std::accumulate(y.begin(), y.end(), 0.);

    float cLSQN = cCross.size() * cSumCross - cSumX * cSumY;
    float cLSQD = cSq.size() * cSumSq - cSumX * cSumX;
    return static_cast<T>(cLSQN / cLSQD);
}

// Template to return a vector of all mismatched elements in two vectors using std::mismatch for readback value comparison
template <typename T, class BinaryPredicate>
std::vector<typename std::iterator_traits<T>::value_type> get_mismatches(T pWriteVector_begin, T pWriteVector_end, T pReadVector_begin, BinaryPredicate p)
{
    std::vector<typename std::iterator_traits<T>::value_type> pMismatchedWriteVector;

    for(std::pair<T, T> cPair = std::make_pair(pWriteVector_begin, pReadVector_begin); (cPair = std::mismatch(cPair.first, pWriteVector_end, cPair.second, p)).first != pWriteVector_end;
        ++cPair.first, ++cPair.second)
        pMismatchedWriteVector.push_back(*cPair.first);

    return pMismatchedWriteVector;
}

std::string getBoardString(uint16_t boardId);

std::string getOpticalGroupString(uint16_t boardId, uint16_t opticalGroupId);

std::string getHybridString(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId);

std::string getReadoutChipString(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId, uint16_t readoutChipId);

time_t getTimeStamp();

std::string getTimeStampString();

template <size_t N>
std::bitset<N> reorderBytes(const std::vector<uint32_t> theWordVector, uint8_t wordSize = 1)
{
    if(wordSize != 1 && wordSize != 2)
    {
        std::cerr << "getPatternPrintout wordSize can be only 1 or 2" << std::endl;
        abort();
    }
    std::bitset<N> reorderedByteVector;
    uint16_t       mask = 0xFF;
    if(wordSize == 2) mask = 0xFFFF;

    size_t numberOfTotalBytes = theWordVector.size() * sizeof(uint32_t) - wordSize;

    for(size_t theWordIndex = 0; theWordIndex < theWordVector.size(); ++theWordIndex)
    {
        for(uint8_t theByteShift = 0; theByteShift < sizeof(uint32_t); theByteShift += wordSize)
        {
            std::bitset<N> byteValue{((theWordVector[theWordIndex] >> (theByteShift * 8)) & mask)};
            byteValue = byteValue << numberOfTotalBytes * 8;
            numberOfTotalBytes -= wordSize;
            reorderedByteVector |= byteValue;
        }
    }

    return reorderedByteVector;
}

std::vector<uint32_t> reorderPattern(const std::vector<uint32_t>& theWordVector, uint8_t wordSize);

std::string getPatternPrintout(const std::vector<uint32_t>& theWordVector, uint8_t wordSize, bool reorderWords = false);

std::vector<uint32_t> applyByteShift(const std::vector<uint32_t>& theWordVector, uint8_t numberOfBytesInSinglePacket, uint8_t numberOfPacketsToSkip);

std::pair<bool, size_t> matchPattern(const std::vector<uint32_t>& theWordVector, uint8_t numberOfBytesInSinglePacket, uint32_t pattern, uint32_t patternMask);

uint16_t linearizeRowAndCols(uint16_t row, uint16_t col, uint16_t numberOfCols);

float                 countMatchingBits(const std::vector<uint32_t>& incomingData, const std::vector<uint32_t>& possiblePatternList);
std::vector<uint32_t> getPossiblePatterns(uint8_t injectedPattern, bool is10Gmodule);

std::vector<std::vector<uint32_t>> splitBits(const std::vector<uint32_t>& input, uint32_t numberOfSplits);

template <size_t Nbits, size_t Mask, typename T>
inline uint32_t getWord(T dataStart, size_t bitStart)
{
    auto   firstWordPointer = dataStart + bitStart / 32;
    size_t firstWordMSB     = 31 - bitStart % 32;
    size_t secondWordLSB    = (32 - (Nbits + bitStart) % 32) % 32;
    if(firstWordMSB >= Nbits - 1) { return (*firstWordPointer >> secondWordLSB) & Mask; }
    else
    {
        auto     secondWordPointer = dataStart + (bitStart + Nbits) / 32;
        uint64_t longerWord        = *firstWordPointer;
        longerWord                 = longerWord << 32;
        longerWord |= *secondWordPointer;
        return (longerWord >> secondWordLSB) & Mask;
    }
}
std::string runCommand(const std::string& cmd);

#endif
