/*!
 *
 * \file RegeristerTest.h
 * \brief Register Tester  class
 * \author Sarah SEIF EL NASR_STOREY
 * \date 19 / 10 / 16
 *
 * \Support : sarah.storey@cern.ch
 *
 */

#ifndef RegisterTester_h__
#define RegisterTester_h__

#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerRecycleBin.h"
#include "tools/Tool.h"

#ifdef __USE_ROOT__
#include "DQMUtils/DQMHistogramRegisterTest.h"
#include "TH1.h"
#endif

using namespace Ph2_System;

// Typedefs for Containers
typedef std::map<uint32_t, std::set<std::string>>              BadRegisters;
typedef std::pair<std::string, Ph2_HwDescription::ChipRegItem> Register;
typedef std::vector<Register>                                  Registers;

class RegisterTester : public Tool
{
  public:
    RegisterTester();

    // D'tor
    ~RegisterTester();

    void Initialise();
    // Test registers for hybrid test test
    void RegisterTest();

    // Reload CBC registers from file found in directory.
    // If no directory is given use the default files for the different operational modes found in Ph2_ACF/settings
    void ReconfigureRegisters(std::string pDirectoryName = "");
    //
    void TestRegisters();
    // Print test results to a text file in the results directory : registers_test.txt
    void PrintTestReport();

    // Get number of registers which failed the test
    int GetNumFails() { return fNBadRegisters; };

    // Return true if all the CBCs passed the register check.
    bool PassedTest();

    void print(std::vector<uint8_t> pChipIds);
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;
    void writeObjects();
    void Reset();
    void initializeRecycleBin() { fRecycleBin.setDetectorContainer(fDetectorContainer); }

    void CheckReadRegisters(uint8_t pPageToSelect = 0, uint8_t pNRegisters = 10);
    void CheckWriteRegisters(uint8_t pPageToSelect = 0, uint8_t pNRegisters = 10);
    void CheckPageSwitchRead(uint8_t pPageToSelect = 0, uint8_t pNRegisters = 1);
    void CheckPageSwitchWrite(uint8_t pPageToSelect = 0, uint8_t pNRegisters = 1);
    void SetSortOrder(uint8_t pSortOrder) { fSortOrder = pSortOrder; };
    void SetBitToFlip(uint8_t pBit) { fBitToFlip = pBit; };
    void SetReturnToDefPage(uint8_t pReturn) { fReturnToDefPage = pReturn; };

  private:
    // timing
    std::chrono::seconds::rep fStartTime;
    std::chrono::seconds::rep fStopTime;

    // Containers
    BadRegisters                 fBadRegisters;
    ContainerRecycleBin<uint8_t> fRecycleBin;
    DetectorDataContainer        fRegList;

    // Counters
    uint32_t fNBadRegisters;

    // sort order
    uint8_t fSortOrder{0};

    // bit to flip during register write test
    uint8_t fBitToFlip{0};

    // set page back to default after a write
    uint8_t fReturnToDefPage{0};

    // HardReset
    void SendHardReset(const Ph2_HwDescription::OpticalGroup* pOpticalGroup, const Ph2_HwDescription::Chip* pFrontEndChip);

    // functions/procedures
    void PrintTestResults(std::ostream& os = std::cout);

    struct
    {
        bool operator()(Register a, Register b) const { return a.second.fPage < b.second.fPage; }
    } customLessThanPage;
    struct
    {
        bool operator()(Register a, Register b) const { return a.second.fPage > b.second.fPage; }
    } customGreaterThanPage;
    struct
    {
        bool operator()(Register a, Register b) const { return a.second.fAddress > b.second.fAddress; }
    } customGreaterThanAddress;
    struct
    {
        bool operator()(Register a, Register b) const { return a.second.fAddress < b.second.fAddress; }
    } customLessThanAddress;

#ifdef __USE_ROOT__
    DQMHistogramRegisterTest fDQMHistogrammer;
#endif
    // void CheckPageSwitch();
    // void CheckPageSwitch();
};
#endif
