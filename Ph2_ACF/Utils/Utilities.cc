/*
  FileName :                     Utilities.cc
  Content :                      Some objects that might come in handy
  Programmer :                   Nicolas PIERRE
  Version :                      1.0
  Date of creation :             10/06/14
  Support :                      mail to : nicolas.pierre@icloud.com
*/

#include "Utils/Utilities.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"

#include <boost/algorithm/string.hpp>
#include <boost/math/special_functions/binomial.hpp>

std::string getResultDirectoryName(const StartInfo& theStartInfo)
{
    std::string resultDirectory = getResultDirectoryName(theStartInfo.getRunNumber());
    std::string append          = theStartInfo.getAppendInformation();
    if(append != "") resultDirectory = resultDirectory + "_" + append;
    return resultDirectory;
}

std::string getResultDirectoryName(const int runNumber)
{
    std::string resultDirectory = "Results/Run_" + std::to_string(runNumber);
    return resultDirectory;
}

int returnPreviousRunNumber(std::string cFileName)
{
    std::string   cLine;
    int           cRunNumber = -1;
    std::ifstream cStream(cFileName);
    if(cStream.is_open())
    {
        while(std::getline(cStream, cLine))
        {
            std::istringstream cIStream(cLine);
            cIStream >> cRunNumber;
        }
    }

    return cRunNumber;
}

int returnAndIncreaseRunNumber(std::string cFileName)
{
    int           cRunNumber = returnPreviousRunNumber(cFileName) + 1;
    std::ofstream cRunLog;
    cRunLog.open(cFileName, std::fstream::app);
    cRunLog << cRunNumber << "\n";
    cRunLog.close();

    return cRunNumber;
}

const std::string currentDateTime()
{
    time_t    now = time(0);
    struct tm tstruct;
    char      buf[80];
    tstruct = *localtime(&now);

    strftime(buf, sizeof(buf), "_%d-%m-%y_%Hh%Mm%S", &tstruct);

    return buf;
}

double MyErf(double* x, double* par)
{
    double x0    = par[0];
    double width = par[1];
    double fitval(0);

    if(x[0] < x0)
        fitval = 0.5 * erfc((x0 - x[0]) / (sqrt(2.) * width));
    else
        fitval = 0.5 + 0.5 * erf((x[0] - x0) / (sqrt(2.) * width));

    return fitval;
}

double MyErfc(double* x, double* par) { return 1 - MyErf(x, par); }

double MyGammaSignal(double* x, double* par)
{
    double VCth = x[0];
    double P0   = par[0];           // plateau
    double P1   = par[1];           // width
    double P2   = par[2];           // signal
    double P3   = par[3] / sqrt(2); // noise?

    double fitval = P0 + (P0 * P3 / P1) * log((exp(VCth / P3) + exp(P2 / P3)) / (exp(VCth / P3) + exp((P2 + P1) / P3)));
    return fitval;
}

uint32_t convertAnyInt(std::string pRegValue)
{
    int baseType = 0;
    if(pRegValue.find("0x") != std::string::npos)
        baseType = 16;
    else if(pRegValue.find("0d") != std::string::npos)
        baseType = 10;
    else if(pRegValue.find("0b") != std::string::npos)
        baseType = 2;
    if(baseType != 0) pRegValue.erase(0, 2);
    return static_cast<uint32_t>(strtoul(pRegValue.c_str(), 0, (baseType != 0 ? baseType : 10)));
}

double convertAnyDouble(std::string pRegValue)
{
    int baseType = 0;
    if(pRegValue.find("0x") != std::string::npos)
    {
        baseType = 16;
        unsigned int      x;
        std::stringstream ss;
        ss << std::hex << pRegValue;
        ss >> x;
        return x;
    }
    else if(pRegValue.find("0d") != std::string::npos)
        baseType = 10;
    else if(pRegValue.find("0b") != std::string::npos)
        baseType = 2;
    if(baseType != 0) pRegValue.erase(0, 2);
    return strtod(pRegValue.c_str(), 0);
}

std::vector<float> convertStringToFloatList(std::string theListString)
{
    boost::erase_all(theListString, " ");
    std::vector<float> theListOfFloats;

    if(theListString.find('-') != std::string::npos)
    {
        size_t            dashPosition = theListString.find('-');
        std::stringstream startString(theListString.substr(0, dashPosition));
        std::stringstream endString(theListString.substr(dashPosition + 1));
        float             startFloat, endFloat;
        startString >> startFloat;
        endString >> endFloat;
        for(float i = startFloat; i <= endFloat; i++) theListOfFloats.push_back(i);
    }
    else
    {
        std::vector<std::string> subStringList;
        boost::algorithm::split(subStringList, theListString, boost::algorithm::is_any_of(","));
        for(auto subString: subStringList) theListOfFloats.push_back(strtof(subString.c_str(), nullptr));
    }
    return theListOfFloats;
}

void tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters)
{
    std::vector<std::string> cTokens;
    cTokens.clear();

    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);

    // Find first "non-delimiter".
    std::string::size_type pos = str.find_first_of(delimiters, lastPos);

    while(std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        cTokens.push_back(str.substr(lastPos, pos - lastPos));

        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);

        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }

    tokens = cTokens;
}

void getRunNumber(const std::string& pPath, int& pRunNumber, bool pIncrement)
{
    std::string  line;
    std::fstream cFile;
    std::string  filename = expandEnvironmentVariables(pPath) + "/.run_number.txt";

    struct stat buffer;

    if(stat(filename.c_str(), &buffer) == 0)
    {
        cFile.open(filename.c_str(), std::fstream::out | std::fstream::in);

        if(cFile.is_open())
        {
            cFile >> pRunNumber;

            if(pIncrement)
            {
                pRunNumber++;
                cFile.clear();
                cFile.seekp(0);
                cFile << pRunNumber;
            }

            cFile.close();
        }
    }
    else if(pRunNumber != -1)
    {
        pRunNumber = 1;
        cFile.open(filename, std::fstream::out);
        cFile << pRunNumber;
        cFile.close();
    }
}

std::string expandEnvironmentVariables(std::string s)
{
    if(s.find("${") == std::string::npos) return s;

    std::string pre  = s.substr(0, s.find("${"));
    std::string post = s.substr(s.find("${") + 2);

    if(post.find('}') == std::string::npos) return s;

    std::string variable = post.substr(0, post.find('}'));
    std::string value    = "";

    post = post.substr(post.find('}') + 1);

    if(getenv(variable.c_str()) != NULL) value = std::string(getenv(variable.c_str()));

    return expandEnvironmentVariables(pre + value + post);
}

double hitProbability(double pThreshold)
{
    return 0.5 - (erf(pThreshold / sqrt(2)) / 2);
    // area above threshold under the gaussian curve.
    // The Factors are to only treat the positive half
    // 1-erf(x/sqrt(2)/2 + .5)
}

double binomialPdf(uint32_t n, uint32_t k, double p)
{
    double value = 0;
    try
    {
        value = boost::math::binomial_coefficient<double>(n, k) * pow(p, k) * pow((1 - p), n - k);
    }
    catch(...)
    {
        std::cout << "binomial PDF failed with n=" << n << " k=" << k << " p=" << p << std::endl;
    }
    return value;
}

double hitProbabilityFunction(double* pStrips, double* pPar)
{
    double cNSamplingsCM = 100;
    double cSigmaRange   = 6;

    const double samplingHalfStep = cSigmaRange / static_cast<double>(cNSamplingsCM);
    double&      threshold        = pPar[0];
    double&      cmnFraction      = pPar[1];
    double&      nEvents          = pPar[2];
    double&      nActiveStrips    = pPar[3];

    double result = 0;
    double hitProb;
    double sampleProbability, x;
    double indFraction = 0;
    if(abs(cmnFraction) <= 1) indFraction = pow(1 - cmnFraction * cmnFraction, 0.5);

    int iStrips = int(ceil(pStrips[0] - 0.5));               // round to nearest integer
    if((iStrips < 0) || (iStrips > nActiveStrips)) return 0; // only defined in range

    for(uint32_t j = 0; j < cNSamplingsCM; ++j)
    {
        // loop over all x values
        x = -cSigmaRange + j * 2 * samplingHalfStep;

        // approximate probability at sampling point by interpolating
        sampleProbability = hitProbability(x - samplingHalfStep);
        sampleProbability -= hitProbability(x + samplingHalfStep);

        // probability of hit taking cmn into account
        hitProb = hitProbability((threshold + x * cmnFraction) * indFraction);
        // distribution function scaled to nevents
        result += binomialPdf(int(nActiveStrips), iStrips, hitProb) * sampleProbability * nEvents;
    }
    return result;
}

std::string getBoardString(uint16_t boardId) { return "Board_" + std::to_string(boardId); }

std::string getOpticalGroupString(uint16_t boardId, uint16_t opticalGroupId) { return getBoardString(boardId) + "_OpticalGroup_" + std::to_string(opticalGroupId); }

std::string getHybridString(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId) { return getOpticalGroupString(boardId, opticalGroupId) + "_Hybrid_" + std::to_string(hybridId); }

std::string getReadoutChipString(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId, uint16_t readoutChipId)
{
    return getHybridString(boardId, opticalGroupId, hybridId) + "_ReadoutChip_" + std::to_string(readoutChipId);
}

time_t getTimeStamp()
{
    time_t rawtime;
    time(&rawtime);
    return rawtime;
}

std::string getTimeStampString()
{
    time_t rawtime = getTimeStamp();
    char   buffer[80];
    std::strftime(buffer, sizeof(buffer), TIME_FORMAT, std::localtime(&rawtime));
    std::string time_str(buffer);

    return time_str;
}

std::vector<uint32_t> applyByteShift(const std::vector<uint32_t>& theWordVector, uint8_t numberOfBytesInSinglePacket, uint8_t numberOfPackets)
{
    uint16_t mask = 0xFF;
    if(numberOfBytesInSinglePacket == 2) mask = 0xFFFF;

    int                   maxWritePatternShift = sizeof(uint32_t) / numberOfBytesInSinglePacket - 1;
    std::vector<uint32_t> longIntWordVector;

    uint32_t longIntWord             = 0;
    int      writeSinglePatternShift = maxWritePatternShift;
    for(auto theWord: theWordVector)
    {
        uint32_t tmpLongIntWord = theWord; // otherwise bitshift will roll over
        for(int8_t readSinglePatterShift = (sizeof(uint32_t) / numberOfBytesInSinglePacket - 1); readSinglePatterShift >= 0; --readSinglePatterShift)
        {
            if(numberOfPackets > 0)
            {
                --numberOfPackets;
                continue;
            }
            longIntWord = longIntWord | (((tmpLongIntWord >> (readSinglePatterShift * 8 * numberOfBytesInSinglePacket)) & mask) << (writeSinglePatternShift * numberOfBytesInSinglePacket * 8));
            --writeSinglePatternShift;
            if(writeSinglePatternShift < 0)
            {
                longIntWordVector.push_back(longIntWord);
                longIntWord             = 0;
                writeSinglePatternShift = maxWritePatternShift;
            }
        }
    }

    return longIntWordVector;
}

std::vector<uint32_t> reorderPattern(const std::vector<uint32_t>& theWordVector, uint8_t wordSize)
{
    if(wordSize != 1 && wordSize != 2)
    {
        std::cerr << "reorderPattern wordSize can be only 1 or 2" << std::endl;
        abort();
    }

    uint16_t mask = 0xFF;
    if(wordSize == 2) mask = 0xFFFF;

    size_t                wordVectorSize = theWordVector.size();
    std::vector<uint32_t> theOrderedWordVector(wordVectorSize, 0);

    for(size_t wordIndex = 0; wordIndex < wordVectorSize; ++wordIndex)
    {
        for(uint8_t theByteShift = 0; theByteShift < sizeof(uint32_t); theByteShift += wordSize)
        {
            uint32_t byteValue = ((theWordVector[wordIndex] >> (theByteShift * 8)) & mask);
            // std::cout << std::hex << "full word " << theWordVector[wordIndex] << " bit shift " << (theByteShift*8) << " mask " << mask << " ouput byte " << byteValue << std::endl;
            theOrderedWordVector[wordIndex] |= byteValue << ((sizeof(uint32_t) - wordSize - theByteShift) * 8);
        }
    }

    return theOrderedWordVector;
}

std::string getPatternPrintout(const std::vector<uint32_t>& theWordVector, uint8_t wordSize, bool reorderWords)
{
    if(wordSize != 1 && wordSize != 2)
    {
        std::cerr << "getPatternPrintout wordSize can be only 1 or 2" << std::endl;
        abort();
    }
    std::vector<uint32_t> theLocalWordVector;
    if(reorderWords)
        theLocalWordVector = reorderPattern(theWordVector, wordSize);
    else
        theLocalWordVector = theWordVector;

    uint16_t mask = 0xFF;
    if(wordSize == 2) mask = 0xFFFF;

    std::stringstream thePattern;
    thePattern << std::hex;

    for(auto theWord: theLocalWordVector)
    {
        for(int8_t theByteShift = sizeof(uint32_t) - wordSize; theByteShift >= 0; theByteShift -= wordSize)
        {
            uint32_t byteValue = ((theWord >> (theByteShift * 8)) & mask);
            // std::cout << std::hex << "full word " << theWord << " bit shift " << (theByteShift*8) << " mask " << mask << " ouput byte " << byteValue << std::endl;
            if(byteValue <= 0xF) thePattern << "0";
            if(wordSize == 2)
            {
                if(byteValue <= 0xFF) thePattern << "0";
                if(byteValue <= 0xFFF) thePattern << "0";
            }
            thePattern << +byteValue;
        }
        thePattern << " ";
    }
    thePattern << std::dec;

    return thePattern.str();
}

std::pair<bool, size_t> matchPattern(const std::vector<uint32_t>& theWordVector, uint8_t numberOfBytesInSinglePacket, uint32_t pattern, uint32_t patternMask)
{
    uint8_t numberOfBytesInWord = sizeof(uint32_t);
    for(uint8_t numberOfBytesToSkip = 0; numberOfBytesToSkip < numberOfBytesInWord; ++numberOfBytesToSkip)
    {
        std::vector<uint32_t> longIntWordVector = applyByteShift(theWordVector, numberOfBytesInSinglePacket, numberOfBytesToSkip);
        for(size_t wordIndex = 0; wordIndex < longIntWordVector.size(); ++wordIndex)
        {
            if((longIntWordVector[wordIndex] & patternMask) == pattern) return {true, wordIndex * numberOfBytesInWord + numberOfBytesToSkip};
        }
    }

    return {false, 0}; // Pattern not found
}

uint16_t linearizeRowAndCols(uint16_t row, uint16_t col, uint16_t numberOfCols) { return col + row * numberOfCols; }

float countMatchingBits(const std::vector<uint32_t>& incomingData, const std::vector<uint32_t>& possiblePatternList)
{
    float maximumMatchingEfficiency = -1;
    for(auto possiblePattern: possiblePatternList)
    {
        float currentEfficiency = 0;
        for(auto word: incomingData)
        {
            auto            possiblePatternXOR = word ^ possiblePattern;
            std::bitset<32> possiblePatternXORbitset(possiblePatternXOR);
            possiblePatternXORbitset.flip();
            currentEfficiency += possiblePatternXORbitset.count();
        }
        if(currentEfficiency > maximumMatchingEfficiency) maximumMatchingEfficiency = currentEfficiency;
        if(maximumMatchingEfficiency == 32 * incomingData.size()) break;
    }

    return maximumMatchingEfficiency / (incomingData.size() * 32);
}

std::vector<uint32_t> getPossiblePatterns(uint8_t injectedPattern, bool is10Gmodule)
{
    uint64_t fullPattern = 0;
    if(is10Gmodule)
    {
        uint16_t doubleDigitShiftRegisterPattern = 0;
        for(uint8_t bit = 0; bit < 8; ++bit)
        {
            uint16_t singleBit = (injectedPattern >> bit) & 0x1;
            doubleDigitShiftRegisterPattern |= ((singleBit << (2 * bit)) | singleBit << (2 * bit + 1));
        }
        for(uint8_t bitShift = 0; bitShift < 4; ++bitShift) { fullPattern |= (uint64_t(doubleDigitShiftRegisterPattern) << (16 * bitShift)); }
    }
    else
    {
        for(uint8_t bitShift = 0; bitShift < 8; ++bitShift) { fullPattern |= (uint64_t(injectedPattern) << (8 * bitShift)); }
    }

    std::vector<uint32_t> possiblePatternList;
    for(uint8_t bitShift = 0; bitShift < 32; ++bitShift) { possiblePatternList.push_back((fullPattern >> bitShift) & 0xFFFFFFFF); }

    // Remove duplicates
    sort(possiblePatternList.begin(), possiblePatternList.end());
    possiblePatternList.erase(unique(possiblePatternList.begin(), possiblePatternList.end()), possiblePatternList.end());

    return possiblePatternList;
}

std::vector<std::vector<uint32_t>> splitBits(const std::vector<uint32_t>& input, uint32_t numberOfSplits)
{
    size_t size = input.size() / numberOfSplits;
    if(input.size() % numberOfSplits > 0) ++size;
    std::vector<std::vector<uint32_t>> outputVector(numberOfSplits, std::vector<uint32_t>(size, 0));

    uint32_t numberOfBitsInWord = 32;

    uint32_t totalNumberOfBits = input.size() * numberOfBitsInWord;

    for(uint32_t bitNumber = 0; bitNumber < totalNumberOfBits; ++bitNumber)
    {
        uint32_t bit          = (input[bitNumber / numberOfBitsInWord] >> (numberOfBitsInWord - 1 - bitNumber % numberOfBitsInWord)) & 0x1;
        auto&    wordToModify = outputVector[bitNumber % numberOfSplits][bitNumber / (numberOfSplits * numberOfBitsInWord)];
        wordToModify |= (bit << (numberOfBitsInWord - 1 - ((bitNumber / numberOfSplits) % numberOfBitsInWord)));
    }

    return outputVector;
}

std::string runCommand(const std::string& cmd)
{
    std::array<char, 128> buffer;
    std::string           result;

    // Open pipe to file
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if(!pipe) { throw std::runtime_error("popen() failed!"); }

    // Read until end of process
    while(fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) { result += buffer.data(); }

    return result;
}