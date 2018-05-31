#pragma once

#include "elf/base/ctrl.h"
#include "../common/record.h"

using ThreadedCtrlBase =
    elf::ThreadedCtrlBase<elf::concurrency::ConcurrentQueue>;
using Ctrl = elf::CtrlT<elf::concurrency::ConcurrentQueue>;
using Addr = elf::Addr;

class ThreadedDispatcher : public ThreadedCtrlBase {
 public:
  ThreadedDispatcher(elf::GameClient* client, Ctrl& ctrl, int num_games)
      : ThreadedCtrlBase(ctrl, 500), client_(client), num_games_(num_games) {
    start<std::pair<Addr, MsgRestart>, MsgRestart, MsgRequest>();
  }

  // Called by game threads
  void RegGame(int game_idx) {
    ctrl_.RegMailbox<MsgRequest, MsgRestart>(
        "game_" + std::to_string(game_idx));
    // cout << "Register game " << game_idx << endl;
    game_counter_.increment();
  }

  MsgRestart BroadcastReceiveIfDifferent(
      const MsgRequest& old_request,
      std::function<MsgRestart(MsgRequest&&)> on_receive) {
    MsgRequest request = old_request;
    MsgRestart msg;

    if (!request.vers.wait()) {
      if (!ctrl_.peekMail(&request, 0)) {
        return MsgRestart();
      }
    } else {
      ctrl_.waitMail(&request);
    }

    // Once you receive, you need to send a reply.
    msg = on_receive(std::move(request));
    ctrl_.sendMail(addr_, std::make_pair(ctrl_.getAddr(), msg));

    // Wait for confirm from the other side. If result is nontrivial.
    if (msg.result == RestartReply::UPDATE_MODEL) {
      // std::cout << "[" << msg.game_idx << "] On receive done. " << endl;
      MsgRestart msg2;
      ctrl_.waitMail(&msg2);
      // std::cout << "[" << msg.game_idx << "] Broadcast update complete. " <<
      // endl;
    }
    return msg;
  }

 protected:
  MsgRequest curr_request_;
  elf::concurrency::Counter<int> game_counter_;
  elf::GameClient* client_ = nullptr;
  const int num_games_;

  std::string start_target_ = "game_start";

  void before_loop() override {
    // Wait for all games + this processing thread.
    std::cout << "Wait all games[" << num_games_
              << "] to register their mailbox" << std::endl;
    game_counter_.waitUntilCount(num_games_);
    game_counter_.reset();
    std::cout << "All games [" << num_games_ << "] registered" << std::endl;
  }

  void on_thread() override {
    // cout << "Register Recv threads" << endl;
    MsgRequest msg;
    if (ctrl_.peekMail(&msg, 0)) {
      process_request(msg);
    }
  }

  bool process_request(const MsgRequest& request) {
    // Actionable request
    if (request == curr_request_) {
      return false;
    }

    std::cout << elf_utils::now()
              << ", EvalCtrl get new request: " << request.info() << std::endl;
    curr_request_ = request;

    MsgRequest wait_request;
    wait_request.vers.set_wait();

    std::vector<Addr> addrs = ctrl_.filterPrefix(std::string("game"));
    // std::cout << "EvalCtrl: #addrs: " << addrs.size() << std::endl;

    // Check request
    size_t n = curr_request_.client_ctrl.num_game_thread_used < 0
        ? addrs.size()
        : curr_request_.client_ctrl.num_game_thread_used;

    for (size_t i = 0; i < addrs.size(); ++i) {
      const size_t thread_idx = stoi(addrs[i].label.substr(5));
      if (thread_idx < n)
        ctrl_.sendMail(addrs[i], request);
      else
        ctrl_.sendMail(addrs[i], wait_request);
    }

    std::pair<Addr, MsgRestart> msg;
    std::vector<Addr> addrs_to_reply;
    bool update_model = false;

    // Wait until we get all confirmations.
    for (size_t i = 0; i < addrs.size(); ++i) {
      ctrl_.waitMail(&msg);
      // std::cout << "EvalCtrl: Get confirm from " << msg.second.result << ",
      // game_idx = " << msg.second.game_idx << std::endl;
      switch (msg.second.result) {
        case RestartReply::UPDATE_MODEL:
          addrs_to_reply.push_back(msg.first);
          update_model = true;
          break;
        case RestartReply::UPDATE_MODEL_ASYNC:
          update_model = true;
          break;
        default:
          break;
      }
    }

    if (update_model) {
      // Once it is done, send to Python side.
      std::cout << elf_utils::now() << " Get actionable request: black_ver = "
                << request.vers.black_ver
                << ", white_ver = " << request.vers.white_ver
                << ", #addrs_to_reply: " << addrs_to_reply.size() << std::endl;
      elf::FuncsWithState funcs =
          client_->BindStateToFunctions({start_target_}, &request.vers);
      client_->sendWait({start_target_}, &funcs);
    }

    for (const auto& addr : addrs_to_reply) {
      ctrl_.sendMail(addr, MsgRestart());
    }
    return true;
  }
};
