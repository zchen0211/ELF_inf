#pragma once

#include <string>
#include <thread>
#include <vector>

#include "elf/base/context.h"
#include "elf/base/hist.h"
#include "elf/utils/cmd_util.h"

using namespace std;

using AnyP = elf::AnyP;
using Context = elf::Context;
using Client = elf::GameClient;
using Extractor = elf::Extractor;

struct State {
  int id;
  int seq;
  int reply;

  // This save history values.
  elf::HistT<int> hist_values;

  State(int hist_len) : hist_values(hist_len, 1, elf::HistT<int>::BATCH_HIST) {}

  State(const State&) = delete;

  void dumpState(AnyP& anyp, int batch_idx) const {
    // Dump the last n state.
    // cout << "Dump state for id=" << id << ", seq=" << seq << endl;
    hist_values.extract(
        anyp.getAddress<int>({0}), anyp.field().getBatchSize(), batch_idx);
  }

  void loadReply(const int* a) {
    // cout << "[" << hex << this << dec << "] load reply for id=" << id << ",
    // seq=" << seq << ", a=" << *a << endl;
    reply = *a;
  }
};

class Game {
 public:
  Game(
      const elf::OptionMap& options,
      int idx,
      const std::string& batch_target,
      Client* client)
      : options_(options),
        idx_(idx),
        batch_target_(batch_target),
        client_(client),
        s_(options_.get<int>("input_T")) {}

  void mainLoop() {
    // client_->oo() << "Starting sending thread " << idx_;

    // mt19937 rng(idx_);
    for (int j = 0; !client_->DoStopGames(); ++j) {
      s_.id = idx_;
      s_.seq = j;

      int* value_slot = s_.hist_values.prepare();
      *value_slot = idx_ + j; // rng();

      // client_->oo() << "client " << idx_ << " sends #" << j << "...";

      elf::FuncsWithState funcs =
          client_->BindStateToFunctions({batch_target_}, &s_);

      if (client_->sendWait({batch_target_}, &funcs) == comm::FAILED) {
        if ((j == 0 && s_.reply != idx_ + j + 1) ||
            (j > 0 && s_.reply != (idx_ + j) * 2)) {
          client_->oo() << "Error: [" << hex << &s_ << dec << "] client "
                        << idx_ << " return from #" << j
                        << ", last_value_slot: " << *value_slot
                        << ", reply = " << s_.reply;
        }
      } else {
        // client_->oo() << "client " << idx_ << " return from #" << j << "
        // failed.";
      }
    }
  }

 private:
  const elf::OptionMap& options_;

  int idx_;
  std::string batch_target_;
  Client* client_;

  State s_;
};

inline elf::OptionSpec getOptionSpec() {
  elf::OptionSpec optionSpec;
  optionSpec.addOption<int>("batchsize", "batch size");
  optionSpec.addOption<int>("input_T", "input t");
  optionSpec.addOption<int>("num_games", "number of games");
  optionSpec.addOption<std::string>("run_name", "name of run", "name");
  optionSpec.addOption<double>("learning_rate", "learning rate", 1e-4);
  optionSpec.addOption<bool>("some_bool", "some bool");
  optionSpec.addOption<bool>("some_bool_2", "some bool", false);
  optionSpec.addOption<bool>("some_bool_3", "some bool", true);
  optionSpec.addOption<std::vector<int>>(
      "list_of_ints", "a list of ints", {1, 2, 3});
  return optionSpec;
}

class GameContext {
 public:
  GameContext(const elf::OptionMap& options) : options_(options) {
    // Specify keys for exchange.
    int batchsize = options_.get<int>("batchsize");
    int input_T = options_.get<int>("input_T");
    int num_games = options_.get<int>("num_games");

    Extractor& e = ctx_.getExtractor();

    e.addField<int>("value")
        .addExtents(batchsize, {batchsize, input_T, 1})
        .addFunction<State>(&State::dumpState);
    e.addField<int>("reply")
        .addExtents(batchsize, {batchsize, 1})
        .addFunction<State>(&State::loadReply);

    batch_name_ = "test";

    auto f = [this](int i, elf::GameClient* client) {
      Game game(options_, i, batch_name_, client);
      game.mainLoop();
    };

    ctx_.setStartCallback(num_games, f);
  }

  void allocate() {
    std::vector<std::string> keys{"value", "reply"};

    // allocate memory here.
    elf::SharedMemOptions smem_opts(
        batch_name_, options_.get<int>("batchsize"));
    auto& smem = ctx_.allocateSharedMem(smem_opts, keys);

    for (const auto& key : keys) {
      auto& p = *smem[key];

      const int type_size = p.field().getSizeOfType();
      elf::Size stride = p.field().getSize().getContinuousStrides(type_size);
      storage_.emplace_back();

      auto& st = storage_.back();
      st.resize(p.field().getSize().nelement() * type_size);
      p.setAddress(reinterpret_cast<uint64_t>(&st[0]), stride.vec());
    }

    cout << "Allocated shared mem: " << smem.info() << endl;
  }

  Context* ctx() {
    return &ctx_;
  }

 private:
  Context ctx_;
  const elf::OptionMap& options_;

  std::vector<std::vector<unsigned char>> storage_;
  std::string batch_name_;
};
