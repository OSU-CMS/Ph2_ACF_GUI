/*!
  \file                  FileHandler.cc
  \brief                 Binary file handler
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinard@cern.ch
*/

#include "FileHandler.h"

FileHandler::FileHandler(const std::string& pBinaryFileName, char pOption) : fHeader(), fHeaderPresent(false), fOption(pOption), fBinaryFileName(pBinaryFileName), fFileIsOpened(false)
{
    FileHandler::openFile();

    if(fOption == 'w') fThread = std::thread(&FileHandler::writeFile, this);
}

FileHandler::FileHandler(const std::string& pBinaryFileName, char pOption, FileHeader pHeader)
    : fHeader(pHeader), fHeaderPresent(true), fOption(pOption), fBinaryFileName(pBinaryFileName), fFileIsOpened(false)
{
    FileHandler::openFile();

    if(fOption == 'w') fThread = std::thread(&FileHandler::writeFile, this);
}

FileHandler::~FileHandler()
{
    while(fQueue.empty() == false) std::this_thread::sleep_for(std::chrono::microseconds(DESTROYSLEEP));
    FileHandler::closeFile();
}

bool FileHandler::getHeader(FileHeader& theHeader) const
{
    if(fHeaderPresent == true)
    {
        theHeader = fHeader;
        return true;
    }

    FileHeader tmpHeader;
    theHeader = tmpHeader;
    return false;
}

void FileHandler::setData(std::vector<uint32_t>& pVector)
{
    std::lock_guard<std::mutex> cLock(fMutex);
    fQueue.push(pVector);
}

bool FileHandler::isFileOpen()
{
    std::lock_guard<std::mutex> cLock(fMemberMutex);
    return fFileIsOpened;
}

void FileHandler::rewind()
{
    std::lock_guard<std::mutex> cLock(fMemberMutex);

    if((fOption == 'r') && (FileHandler::isFileOpen() == true))
    {
        if(fHeader.fValid == true)
            fBinaryFile.seekg(48, std::ios::beg);
        else
            fBinaryFile.seekg(0, std::ios::beg);
    }
    else
        LOG(ERROR) << BOLDRED << "You should not try to rewind a file opened in write mode (or file not open)" << RESET;
}

bool FileHandler::openFile()
{
    if(FileHandler::isFileOpen() == false)
    {
        std::lock_guard<std::mutex> cLock(fMemberMutex);

        if(fOption == 'w')
        {
            fBinaryFile.open((getFilename()).c_str(), std::fstream::trunc | std::fstream::out | std::fstream::binary);

            if(fHeader.fValid == false)
            {
                LOG(WARNING) << GREEN << "Invalid file header provided, writing file without it ..." << RESET;
                fHeaderPresent = false;
            }
            else if(fHeader.fValid)
            {
                std::vector<uint32_t> cHeaderVec = fHeader.encodeHeader();
                fBinaryFile.write((char*)&cHeaderVec.at(0), cHeaderVec.size() * sizeof(uint32_t));
                LOG(INFO) << GREEN << "Valid file header provided, writing it into the file (" << BOLDYELLOW << cHeaderVec.size() * sizeof(uint32_t) << RESET << GREEN << " 32-bit words)" << RESET;
                fHeaderPresent = true;
            }
        }
        else if(fOption == 'r')
        {
            fBinaryFile.open(getFilename().c_str(), std::fstream::in | std::fstream::binary);
            fHeader.decodeHeader(FileHandler::readFileChunks(fHeader.fHeaderSize));

            if(fHeader.fValid == false)
            {
                fHeaderPresent = false;
                LOG(WARNING) << BOLDRED << "No valid header found in binary file: " << BOLDYELLOW << fBinaryFileName << BOLDRED << " --> resetting to begin of file and treating as normal data"
                             << RESET;
                fBinaryFile.clear();
                fBinaryFile.seekg(0, std::ios::beg);
            }
            else if(fHeader.fValid == true)
            {
                LOG(INFO) << GREEN << "Found a valid header in binary file: " << BOLDYELLOW << fBinaryFileName << RESET;
                fHeaderPresent = true;
            }
        }

        fFileIsOpened = true;
    }

    return fFileIsOpened;
}

void FileHandler::closeFile()
{
    if(fFileIsOpened == true)
    {
        fFileIsOpened = false;
        if((fOption == 'w') && (fThread.joinable() == true)) fThread.join();
        fBinaryFile.close();
        LOG(INFO) << GREEN << "Closed binary file: " << BOLDYELLOW << fBinaryFileName << RESET;
    }
}

std::vector<uint32_t> FileHandler::readFile()
{
    std::vector<uint32_t> cVector;
    uint32_t              word;

    fBinaryFile.seekg(0, std::ios::end);
    double fileSize = fBinaryFile.tellg();
    cVector.reserve(fileSize * sizeof(char) / sizeof(uint32_t));
    fBinaryFile.seekg(0, std::ios::beg);

    while(true)
    {
        fBinaryFile.read((char*)&word, sizeof(uint32_t));
        if(fBinaryFile.eof() == true) break;
        cVector.emplace_back(word);
    }

    closeFile();
    return cVector;
}

std::vector<uint32_t> FileHandler::readFileChunks(uint32_t pNWords)
{
    std::vector<uint32_t> cVector;
    uint32_t              word;

    for(auto i = 0u; i < pNWords; i++)
    {
        fBinaryFile.read((char*)&word, sizeof(uint32_t));

        if(fBinaryFile.eof() == true)
        {
            LOG(WARNING) << BOLDRED << "Attention, input file " << BOLDYELLOW << fBinaryFileName << BOLDRED << " ended before reading " << BOLDYELLOW << pNWords << " 32-bit words" << RESET;
            closeFile();
            break;
        }

        cVector.emplace_back(word);
    }

    return cVector;
}

void FileHandler::writeFile()
{
    while(fFileIsOpened == true)
    {
        std::vector<uint32_t> cData;
        bool                  cDataPresent = FileHandler::dequeue(cData);

        if((cDataPresent == true) && (cData.size() != 0))
        {
            std::lock_guard<std::mutex> cLock(fMemberMutex);
            fBinaryFile.write((char*)&cData.at(0), cData.size() * sizeof(uint32_t));
            fBinaryFile.flush();
        }
    }
}

bool FileHandler::dequeue(std::vector<uint32_t>& pData)
{
    std::lock_guard<std::mutex> cLock(fMutex);

    if(fQueue.empty() == false)
    {
        pData = fQueue.front();
        fQueue.pop();
        return true;
    }

    return false;
}
