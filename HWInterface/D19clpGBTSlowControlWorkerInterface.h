#ifndef _D19clpGBTSlowControlWorkerInterface_H__
#define _D19clpGBTSlowControlWorkerInterface_H__

#include "HWInterface/D19cCommandProcessorInterface.h"

namespace LpGBTSlowControlWorker
{
const uint8_t BASE_ID              = 16;
const uint8_t READ_IC              = 2;
const uint8_t WRITE_IC             = 3;
const uint8_t SINGLE_BYTE_READ_I2C = 4;
const uint8_t MULTI_BYTE_WRITE_I2C = 5;
const uint8_t READ_FE              = 6;
const uint8_t WRITE_FE             = 7;
const int     BLOCK_SIZE           = 16000;

const std::map<uint8_t, std::string> I2C_STATUS_MAP = {{0, "InvalidTransactionCode"}, {4, "TransactionSucess"}, {8, "SDAPulledLow"}, {32, "InvalidCommand"}, {64, "NotACK"}};

const std::map<int, std::string> WORKER_FSM_STATE_MAP{{0, "UNDEFINED"},
                                                      {1, "IDLE"},
                                                      {2, "GET_COMMAND_HEADER"},
                                                      {3, "REQ_N_WORDS"},
                                                      {4, "GET_FUNCTION_HEADER"},
                                                      {5, "FORWARD_REPLY_HEADER"},
                                                      {6, "REQ_FUNCTION_ARG"},
                                                      {7, "GET_FUNCTION_ARG"},
                                                      {8, "PERFORM_TRANSACTION"},
                                                      {9, "FORWARD_REPLY_PAYLOADN"},
                                                      {11, "REQ_FUNCTION_ARG"}};

const std::map<int, std::string> IC_FSM_STATE_MAP{{0, "UNDEFINED"},
                                                  {1, "IDLE"},
                                                  {2, "SEND_RD_RQ"},
                                                  {3, "FINALIZE_RD"},
                                                  {4, "LOAD_WR_DATA"},
                                                  {5, "SEND_WR_REQ"},
                                                  {6, "FINALIZE_WR"},
                                                  {7, "WAIT_REPLY"},
                                                  {8, "DELAY_GET_REPLY_FRAME"},
                                                  {9, "GET_REPLY_FRAME"},
                                                  {10, "VERIFY_REPLY_FRAME"},
                                                  {11, "GET_REPLY_DATA"}};

const std::map<int, std::string> I2C_FSM_STATE_MAP{{0, "UNDEFINED"},
                                                   {1, "IDLE"},
                                                   {2, "WRITE_CONF_DATA"},
                                                   {3, "WRITE_CONF_CMD"},
                                                   {4, "WRITE_SLAVE_ADDR"},
                                                   {5, "WRITE_RD_CMD"},
                                                   {6, "GET_RD_VALUE"},
                                                   {7, "FINALIZE_RD"},
                                                   {8, "WRITE_SLAVE_DATA0"},
                                                   {9, "WRITE_SLAVE_DATA1"},
                                                   {10, "WRITE_SLAVE_DATA2"},
                                                   {11, "WRITE_SLAVE_DATA3"},
                                                   {12, "WRITE_WRITE_DATA_CMD"},
                                                   {13, "WRITE_WR_CMD"},
                                                   {14, "FINALIZE_WR"},
                                                   {15, "GET_STAT"},
                                                   {16, "CHECK_STAT"},
                                                   {17, "WAIT_IC_WR_DONE"},
                                                   {18, "WAIT_IC_RD_DONE"},
                                                   {19, "UNDEFINED"}};

const std::map<int, std::string> FE_FSM_STATE_MAP{{0, "UNDEFINED"},
                                                  {1, "IDLE"},
                                                  {2, "SEND_RD_REQ"},
                                                  {3, "GET_RD_VALUE"},
                                                  {4, "FINALIZE_RD"},
                                                  {5, "SEND_WR_REQ"},
                                                  {6, "VERIFY_WR"},
                                                  {7, "FINALIZE_WR"},
                                                  {8, "WAIT_I2C_WR_DONE"},
                                                  {9, "WAIT_I2C_WR_DONE"}};
} // namespace LpGBTSlowControlWorker

namespace Ph2_HwDescription
{
class Chip;
class ChipRegItem;
} // namespace Ph2_HwDescription
namespace Ph2_HwInterface
{
class RegManager;
class D19clpGBTSlowControlWorkerInterface : public D19cCommandProcessorInterface
{
  public:
    D19clpGBTSlowControlWorkerInterface(RegManager* theRegManager);
    ~D19clpGBTSlowControlWorkerInterface();

  public:
    void                  Reset();
    std::vector<uint32_t> EncodeCommand(uint8_t pFunctionId, Ph2_HwDescription::Chip* pChip, const std::vector<Ph2_HwDescription::ChipRegItem>& pRegisterItems, bool pVerify = false);
    std::vector<uint32_t> EncodeCommandI2C(uint8_t pFunctionId, Ph2_HwDescription::Chip* pChip, uint8_t pMasterId, uint8_t pMasterConfig, const std::vector<uint32_t>& pSlaveData);
    bool                  WaitDone(uint8_t pFunctionId);
    bool                  IsDone(uint8_t pFunctionId);
    uint8_t               GetTryCounter(uint8_t pFunctionId);
    void                  PrintState();
    void                  SelectLink(uint8_t pLinkId);
};
} // namespace Ph2_HwInterface
#endif
