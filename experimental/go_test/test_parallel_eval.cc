#include "elfgames/go/client_manager.h"
#include "elfgames/go/go_game_specific.h"
#include "elfgames/go/ctrl_eval.h"
#include "elf/concurrency/ConcurrentQueue.h"
#include "elf/ai/tree_search/tree_search_options.h"

#include <queue>

using namespace std;

struct TimedItem {
  uint64_t t;

  friend bool operator<(const unique_ptr<TimedItem> &pq1, const unique_ptr<TimedItem> &pq2) {
    return pq1->t > pq2->t;
  }

  virtual ~TimedItem() = default;
};

struct TimedRecords : public TimedItem {
  Records records;
};

struct TimedRequest : public TimedItem {
  string send_to;
  MsgRequest request;
};

using TSOptions = elf::ai::tree_search::TSOptions;
using PQItem = std::priority_queue<unique_ptr<TimedItem>>;

struct SimulatorInfo {
  uint64_t black_win_delay;
  uint64_t white_win_delay;
  float expected_black_winrate;

  mutable int n = 0;
  mutable int win = 0;

  pair<float, uint64_t> getTrial(mt19937 &rng) const {
    std::uniform_real_distribution<> dis(0.0, 1.0);

    n ++;
    if (dis(rng) <= expected_black_winrate) {
      win ++;
      return make_pair(1.0f, black_win_delay);
    } else {
      return make_pair(-1.0f, white_win_delay);
    }
  }

  string info() const {
    stringstream ss;
    ss << "b_win_dl: " << black_win_delay << ", w_win_dl: " << white_win_delay
       << ", wr: " << expected_black_winrate;
    if (n > 0) {
      ss << ", empirical: " << static_cast<float>(win) / n << " (" << win << "/" << n << ")";
    }
    return ss.str();
  }
};

class WinRateSimulator {
 public:
   void addRandom(const ModelPair &mp, mt19937 &rng) {
     auto it = simulators_.find(mp);
     if (it != simulators_.end()) return;

     // Put random entry here.
     simulators_[mp].reset(new SimulatorInfo);
     SimulatorInfo &info = *simulators_[mp];

     info.black_win_delay = rng() % 20;
     info.white_win_delay = rng() % 20 + 100;

     if (rng() % 100 > 50) swap(info.black_win_delay, info.white_win_delay);
     info.expected_black_winrate = static_cast<float>(rng() % 100) / 100.0;
     // info.expected_black_winrate = 0.68;
     // static_cast<float>(rng() % 100) / 100.0;

     pairs_.push_back(mp);
   }

   const SimulatorInfo &get(const ModelPair &mp) const {
     auto it = simulators_.find(mp);
     if (it == simulators_.end()) {
       cout << "ModelPair cannot be found: " << mp.info() << endl;
     }
     assert(it != simulators_.end());
     return *it->second;
   }

   const vector<ModelPair> &getAllPairs() const { return pairs_; }

   string info() const {
     stringstream ss;
     for (const auto &p : pairs_) {
       auto it = simulators_.find(p);
       assert(it != simulators_.end());

       const auto &r = *it->second;

       ss << "GT[b=" << p.black_ver << ",w=" << p.white_ver << "], " << r.info() << endl;
     }
     return ss.str();
   }

 private:
   vector<ModelPair> pairs_;
   unordered_map<ModelPair, unique_ptr<SimulatorInfo>> simulators_;
};

class Client {
 public:
   Client(const string &id, const WinRateSimulator &ws)
     : id_(id), ws_(ws), rng_(std::hash<string>{}(id) ^ time(NULL)) {
     state_.thread_id = 0;
     state_.seq = 0;
     state_.move_idx = 0;
   }

   TimedRecords getInit() {
     TimedRecords r;

     r.records.identity = id_;
     r.records.states[0] = state_;
     r.t = 0;

     return r;
   }

   TimedRecords Process(const TimedRequest &request) {
     // Only test eval.
     const ModelPair &mp = request.request.vers;
     const ClientCtrl &ctrl = request.request.client_ctrl;

     TimedRecords r;

     if (!is_stuck_) {
       if (request.request != record_to_send_.request) {
         // cout << "[" << id_ << "] old: " << currPair().info() << ", new: " << mp.info() << endl;
         updateRequest(request.request);
         genRecord(request.t);
       } else {
         // Send
         if (! sendRecord(request.t, &r)) {
           state_.move_idx ++;
         }
       }
     }

     r.records.identity = id_;
     r.records.states[0] = state_;
     r.t = request.t + rng_() % 20;

     // swap between stuck and unstuck..
     //if (rng_() % 100 < 5)
     //  is_stuck_ = ! is_stuck_;
     return r;
   }

   void SetStuck() { is_stuck_ = true; }

 private:
   string id_;
   const WinRateSimulator &ws_;
   mt19937 rng_;

   ThreadState state_;

   // Also simulate client stuck..
   bool is_stuck_ = false;

   Record record_to_send_;
   uint64_t send_ts_ = 0;

   const ModelPair &currPair() const { return record_to_send_.request.vers; }

   void updateRequest(const MsgRequest &request) {
     record_to_send_.request = request;
   }

   void genRecord(uint64_t start) {
     const ModelPair &mp = currPair();

     if (! mp.is_selfplay() && ! mp.wait()) {
       pair<float, uint64_t> timed_reward = ws_.get(mp).getTrial(rng_);
       if (record_to_send_.request.client_ctrl.player_swap) timed_reward.first = -timed_reward.first;

       record_to_send_.result.reward = timed_reward.first;
       send_ts_ = start + timed_reward.second;
       // cout << "[" << id_ << "] Set " << mp.info() << ", end time: " << send_ts_ << endl;

       state_.seq ++;
       state_.move_idx = 0;
     }
   }

   bool sendRecord(uint64_t t, TimedRecords *r) {
     if (t < send_ts_ || currPair().wait()) return false;

     // cout << "[" << id_ << "] Sending record " << currPair().info() << endl;
     r->records.records.push_back(record_to_send_);
     genRecord(t);
     return true;
   }
};

class Server {
 public:
  Server(ClientManager &mgr, const WinRateSimulator &ws, const GameOptions &options)
    : mgr_(mgr), ws_(ws),
      rng_(time(NULL)), eval_ctrl_(options, TSOptions()) {
    vector<ModelPair> pairs = ws.getAllPairs();
    eval_ctrl_.setInitBestModel(pairs[0].white_ver);
    for (const auto &p : pairs) {
      eval_ctrl_.addNewModelForEvaluation(p.white_ver, p.black_ver);
    }
  }

  TimedRequest Process(const TimedRecords &r) {
    ClientInfo &info = mgr_.getClient(r.records.identity);
    for (const auto &s : r.records.states) {
      info.stateUpdate(s.second);
    }

    // A client is considered dead after 20 min.
    mgr_.updateClients();

    for (const auto &rr : r.records.records) {
      if (! rr.request.vers.is_selfplay()) {
        eval_ctrl_.feed(info, rr);
      }
    }

    eval_ctrl_.updateState(mgr_);

    TimedRequest request;
    eval_ctrl_.fillInRequest(info, &request.request);
    request.send_to = info.id();
    request.t = r.t + rng_() % 20;
    return request;
  }

 private:
  ClientManager &mgr_;
  const WinRateSimulator &ws_;
  mt19937 rng_;

  EvalSubCtrl eval_ctrl_;
};

ModelPair createModelPair(int ver) {
  ModelPair mp;
  mp.black_ver = ver;
  mp.white_ver = 0;
  return mp;
}

string getId(int i) {
  return string("game-") + to_string(i);
}

unique_ptr<TimedItem> make_item(TimedRequest &&request) {
  return unique_ptr<TimedItem>(new TimedRequest(move(request)));
}

unique_ptr<TimedItem> make_item(TimedRecords &&records) {
  return unique_ptr<TimedItem>(new TimedRecords(move(records)));
}

int main() {
  const size_t num_client = 300;
  GameOptions options;
  options.eval_thres = 0.55;
  options.expected_num_clients = num_client;

  uint64_t timestamp = 0;
  ClientManager mgr(1, 100, options.expected_num_clients, 0.0, -1, [&]() -> uint64_t { return timestamp; });

  // Two threads for client and server
  // mt19937 rng(time(NULL));
  mt19937 rng(1);

  WinRateSimulator ws;
  ws.addRandom(createModelPair(1000), rng);
  // ws.addRandom(createModelPair(2000), rng);
  // ws.addRandom(createModelPair(3000), rng);
  std::cout << ws.info() << endl;

  unordered_map<string, unique_ptr<Client>> clients;
  PQItem q;

  Server server(mgr, ws, options);

  for (size_t i = 0; i < num_client; ++i) {
    string id = getId(i);
    clients[id].reset(new Client(id, ws));
    mgr.getClient(id);
    // cout << info.info() << endl;
    cout << mgr.info() << endl;
    q.push(make_item(clients[id]->getInit()));
  }

  const size_t total_num_stuck = num_client / 5;
  size_t num_stuck = 0;

  const size_t n = 200000;

  for (size_t i = 0; i < n;  ++i) {
    /*
    if (num_stuck * n < i * total_num_stuck) {
      size_t idx = rng() % num_client;
      clients[getId(idx)]->SetStuck();
      cout << "client " << getId(idx) << " is set to be stuck!" << endl;
      num_stuck ++;
    }
    */

    const unique_ptr<TimedItem>& item = q.top();

    timestamp = item->t;

    // cout << "[t=" << timestamp << "]" << endl;
    unique_ptr<TimedItem> next_item;
    //
    const TimedRequest *request = dynamic_cast<const TimedRequest *>(item.get());
    if (request != nullptr) {
      next_item = move(make_item(clients[request->send_to]->Process(*request)));
    } else {
      const TimedRecords *records = dynamic_cast<const TimedRecords *>(item.get());
      assert(records != nullptr);
      next_item = move(make_item(server.Process(*records)));
    }

    q.pop();
    q.push(move(next_item));
  }

  cout << ws.info() << endl;
  return 0;
}
