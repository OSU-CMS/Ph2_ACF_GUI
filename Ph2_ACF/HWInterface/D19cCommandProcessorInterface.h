#ifndef _D19cCommandProcessorInterface_H__
#define _D19cCommandProcessorInterface_H__

#include "HWInterface/CommandProcessorInterface.h"
#include <map>

namespace CommandProcessorArbitrators
{
const std::map<int, std::string> COMMAND_ARBITRATOR_FSM_STATE_MAP{{0, "UNDEFINED"},
                                                                  {1, "IDLE"},
                                                                  {2, "WAIT_FIFO_READ"},
                                                                  {3, "READ_FIRST_WORD"},
                                                                  {4, "HANDSHAKE_WORKER_INIT"},
                                                                  {5, "HANDSHAKE_WORKER_FIRSTARG"},
                                                                  {6, "HANDSHAKE_WORKER_RESOURCES"},
                                                                  {7, "FORWARD_WHILE_COUNTDOWN"},
                                                                  {8, "HANDSHAKE_REPLY"},
                                                                  {9, "SOFT_RESET_WORKER_STEP0"},
                                                                  {10, "SOFT_RESET_WORKER_STEP1"}};

const std::map<int, std::string> REPLY_ARBITRATOR_FSM_STATE_MAP{{0, "UNDEFINED"}, {1, "IDLE"}, {2, "GET_N_REPLIES"}, {3, "FORWARD_N_REPLIES"}, {4, "WAIT_FIFO"}};
} // namespace CommandProcessorArbitrators

namespace Ph2_HwInterface
{
class RegManager;
class D19cCommandProcessorInterface : public CommandProcessorInterface
{
  public:
    D19cCommandProcessorInterface(RegManager* theRegManager);
    ~D19cCommandProcessorInterface();

  public:
    // void                  Reset() override;
    void                  WriteCommand(const std::vector<uint32_t>& pCommand) override;
    std::vector<uint32_t> ReadReply(int pNWords) override;
};
} // namespace Ph2_HwInterface
#endif
