#ifndef __PATTERN_MATCHER_H__
#define __PATTERN_MATCHER_H__

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <vector>

/*!
 * \class PatternMatcher
 * \brief Class to create a pattern and check whether if it matches with data
 */
class PatternMatcher
{
  public:
    /*!
     * \brief Constructor of the PatternMatcher Class
     */
    PatternMatcher();
    /*!
     * \brief Destructor of the PatternMatcher Class
     */
    ~PatternMatcher();

    PatternMatcher(const PatternMatcher& thePatternMatcher);

    PatternMatcher& operator=(const PatternMatcher& thePatternMatcher);

    /*!
     * \brief Add more information in the pattern
     * \param thePattern: the new piece of pattern
     * \param thePatternMask: the mask for the new piece of pattern
     * \param thePatternBitLenght: the number of bits composing the new piece of pattern
     */
    void addToPattern(uint32_t thePattern, uint32_t thePatternMask, uint8_t thePatternBitLenght);

    /*!
     * \brief returns true if the pattern matches with the theWordVector vector provided
     * \param theWordVector: the data to match
     * \return is the pattern matched
     */
    bool isMatched(const std::vector<uint32_t>& theWordVector) const;

    /*!
     * \brief returns the numberOfBits of the pattern that matches with the theWordVector vector provided
     * \param theWordVector: the data to match
     * \return number of bits matchad by the pattern
     */
    uint32_t countMatchingBits(const std::vector<uint32_t>& theWordVector) const;

    /*!
     * \brief get the number of bits composing the pattern
     * \return number of bits in the pattern
     */
    size_t getNumberOfPatternBits() const { return fPatternNumberOfBits; }

    /*!
     * \brief get the pattern vector
     *  \returns pattern vector
     */
    std::vector<uint32_t> getPattern() const;

    /*!
     * \brief get the pattern mask
     * \returns pattern mask
     */
    std::vector<uint32_t> getMask() const;

    /*!
     * \brief temporary function to ignore bug on stub line 4 for 2S kickoff FEHR
     */
    void maskStubFor2Skickoff();

    /*!
     * \brief get number of unmasked bits
     * \returns number of unmasked bits
     */
    uint32_t getNumberOfMaskedBits();

    /*!
     * \brief returns true if the pattern matches with the theWordVector vector provided within the indicated subset
     * \param theWordVector the data to match
     * \param firstBitPosition position of first bit to match
     * \param numberOfBitsToMatch number of consecutive bits to match
     * \return is the pattern matched
     */
    bool isSubsetMatched(const std::vector<uint32_t>& theWordVector, uint32_t firstBitPosition, uint32_t numberOfBitsToMatch) const;

    /*!
     * @brief change bits in the original pattern
     * \param thePattern: the new piece of pattern
     * \param thePatternMask: the mask for the new piece of pattern
     * \param thePatternBitLenght: the number of bits composing the new piece of pattern
     * @param firstBitPosition: the first bit to edit
     */
    void updatePattern(uint32_t thePattern, uint32_t thePatternMask, uint8_t thePatternBitLenght, uint32_t firstBitPosition);

    /*!
     * @brief clear all data members
     */
    void clear();

    /*!
     * @brief get maximum number of bit matching the pattern by considering all possible bitshifts
     * @tparam N number of bits in the pattern
     * @param inputDataVector data to match
     * @return maximum number of matching bits
     */
    template <size_t N>
    uint32_t getNumberOfMatchingBitsForAllBitshifts(const std::vector<uint32_t>& inputDataVector)
    {
        std::bitset<N> maskBitset(0xFFFFFFFF);
        std::bitset<N> theInputDataBiset;
        for(size_t index = 0; index < inputDataVector.size(); ++index)
        {
            std::bitset<N> tmpDataset(inputDataVector[index]);
            theInputDataBiset |= (tmpDataset << (N - 32 * (index + 1)));
        }

        uint32_t maximumEfficiency = 0;

        for(size_t bitShift = 0; bitShift < N; ++bitShift)
        {
            std::vector<uint32_t> rolledInputDataVector(inputDataVector.size());
            for(size_t index = 0; index < rolledInputDataVector.size(); ++index) { rolledInputDataVector[index] = ((theInputDataBiset >> (N - 32 * (index + 1))) & maskBitset).to_ulong(); }
            uint32_t currentEfficiency = countMatchingBits(rolledInputDataVector);
            if(currentEfficiency > maximumEfficiency) maximumEfficiency = currentEfficiency;
            if(maximumEfficiency == getNumberOfMaskedBits()) break;
            int lowestBit        = theInputDataBiset[N - 1];
            theInputDataBiset    = (theInputDataBiset << 1);
            theInputDataBiset[0] = lowestBit;
        }
        return maximumEfficiency;
    }

    void addTrailingZeros(size_t totalNumberOfBitNeeded);

  private:
    size_t                                     fPatternNumberOfBits{0};
    size_t                                     fPatternNumberOfMaskedBits{0};
    std::vector<std::pair<uint32_t, uint32_t>> fPatternAndMaskVector;
};

#endif