/*!
  \file                  FileHandler.h
  \brief                 Binary file handler
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinard@cern.ch
*/

#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include "FileHeader.h"
#include "easylogging++.h"

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <unistd.h>
#include <vector>

// #############
// # CONSTANTS #
// #############
#define DESTROYSLEEP 1000 // [microseconds]

class FileHandler
{
  public:
    FileHandler(const std::string& pBinaryFileName, char pOption);
    FileHandler(const std::string& pBinaryFileName, char pOption, FileHeader pHeader);
    ~FileHandler();

    std::string getFilename() const { return fBinaryFileName; }

    bool                  getHeader(FileHeader& theHeader) const;
    void                  setData(std::vector<uint32_t>& pVector);
    bool                  isFileOpen();
    void                  rewind();
    bool                  openFile();
    void                  closeFile();
    void                  writeFile();
    std::vector<uint32_t> readFile();
    std::vector<uint32_t> readFileChunks(uint32_t pNWords);

  private:
    bool dequeue(std::vector<uint32_t>& pData);

    std::fstream                      fBinaryFile;
    FileHeader                        fHeader;
    bool                              fHeaderPresent;
    char                              fOption;
    std::string                       fBinaryFileName;
    std::thread                       fThread;
    mutable std::mutex                fMutex;
    mutable std::mutex                fMemberMutex;
    std::queue<std::vector<uint32_t>> fQueue;
    std::atomic<bool>                 fFileIsOpened;
};

#endif
