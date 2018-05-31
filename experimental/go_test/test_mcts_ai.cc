#include "../game_context.h"
#include "../mcts/mcts.h"
#include "elf/base/context.h"

using namespace std;

class Storage {
 public:
  elf::SharedMem& allocate(
      elf::Context* ctx,
      const elf::SharedMemOptions& smem_opts,
      const vector<string>& keys) {
    // Allocate memory here.
    auto& smem = ctx->allocateSharedMem(smem_opts, keys);

    for (const auto& key : keys) {
      auto& p = *smem[key];

      const int type_size = p.field().getSizeOfType();
      elf::Size stride = p.field().getSize().getContinuousStrides(type_size);
      storage_.emplace_back();

      auto& st = storage_.back();
      st.resize(p.field().getSize().nelement() * type_size);
      p.setAddress(reinterpret_cast<uint64_t>(&st[0]), stride.vec());
    }
    return smem;
  }

 private:
  std::vector<std::vector<unsigned char>> storage_;
};

MCTSGoAI* init_ai(elf::GameClient* client, int i) {
  mcts::TSOptions mcts_options;
  // mcts_options.alg_opt.c_puct = true;

  MCTSActorParams params;
  params.actor_name = "actor_black";
  params.seed = i;
  params.ply_pass_enabled = 0;
  params.komi = 7.5;
  params.required_version = -1;

  return new MCTSGoAI(
      mcts_options, [&](int) { return new MCTSActor(client, params); });
}

int main() {
  const int batchsize = 8;
  const int num_threads = 1;

  GameOptions options;
  options.use_mcts = true;
  options.use_mcts_ai2 = true;

  unique_ptr<elf::Context> ctx(new elf::Context);
  GoFeature go_feature(options);

  // extractor is an unordered_map, mapping srting to integers
  // classes and functions
  elf::Extractor& e = ctx->getExtractor();
  // register function extractAGZ()
  // register "s", "a", "rv", "offline_a", "V"
  // "pi, "move_idx", "black_ver", "white_ver"
  // "GoReply" ... in extractor e
  go_feature.registerExtractor(batchsize, e);

  elf::SharedMemOptions smem_options("actor_black", batchsize);
  smem_options.setTimeout(10);

  Storage storage;
  auto& smem = storage.allocate(
      ctx.get(),
      smem_options,
      {"s", "offline_a", "winner", "mcts_scores", "move_idx", "selfplay_ver"});
  cout << "Allocated shared mem: " << smem.info() << endl;

  // reset ai as a new MCTSGoAI by init_ai()
  // one-step act() by running MCTS, forward action c
  // finally reset ai and s
  auto f = [](int i, elf::GameClient* client) {
    unique_ptr<MCTSGoAI> ai;
    GoState s;
    int j = 0;

    while (!client->DoStopGames()) {
      // cout << "[" << i << "] Init mcts [" << j << "]" << endl;
      ai.reset(init_ai(client, i + j * 8));

      if (j % 2 == 1) {
        Coord c;
        // function in MCTSAI<>, the base class of MCTSGoAI
        // align state s,  then ts_->run(s);
        ai->act(s, &c);
        s.forward(c);
      }

      // this_thread::sleep_for(1s);

      // cout << "[" << i << "] Delete mcts [" << j << "]" << endl;
      ai.reset(nullptr);
      s.reset();
      // cout << "[" << i << "] Delete complete [" << j << "]" << endl;
      j++;
    }
  };

  // num_games = num_threads (8 in our case)
  // ctx.game_cb_ set as f()
  ctx->setStartCallback(num_threads, f);

  // start collectors_;
  // register server
  // clear game threads, get client
  // reset all 8 threads as:
  //  client->start()
  //  game_cb_(i, client)
  //  client->end()
  ctx->start();

  // Main loop
  for (int i = 0; i < 10; ++i) {
    // wait batch, smem = smem_batch_[0].data
    auto& smem = *ctx->wait();
    // batch_server_ release Batch
    ctx->step();
  }

  cout << "Done with all mcts " << endl;
  cout << "Done with Recv, quitting" << endl;
  ctx->stop();
}
