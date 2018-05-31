#include <functional>
#include <queue>
#include <vector>
#include <sstream>
#include <iostream>
#include <random>

using namespace std;

struct Item {
  int from = 0;
  int span = 0;
  int start_time = 0;

  float r = 0.0;

  Item(int from, mt19937 &rng)
    : from(from) {
    init(rng);
  }

  Item follow_job(mt19937 &rng) const {
    Item item(from, rng);
    item.start_time = start_time + span;
    return item;
  }

  void init(mt19937 &rng) {
    // Random span and reward.
    r = rng() % 2 == 0 ? 1.0 : 0.0;
    if (r > 0.5) {
      span = 30;
    } else {
      span = 10;
    }
  }

  string info() const {
    stringstream ss;
    ss << "[from=" << from << "][stime=" << start_time << "][span=" << span << "][etime=" << start_time + span << "][r=" << r << "]";
    return ss.str();
  }

  friend bool operator<(const Item &item1, const Item &item2) {
    return item1.start_time + item1.span > item2.start_time + item2.span;
  }
};

// Simulated scheduling
class Generator {
 public:
  Generator(int num_machine)
    : rng_(time(NULL)) {
    for (int i = 0; i < num_machine; ++i) {
      q_.push(Item(i, rng_));
    }
  }

  Item Proceed() {
    // Find the next finished task and generate.
    Item item = q_.top();
    q_.pop();

    // Add another item.
    q_.push(item.follow_job(rng_));
    return item;
  }

 private:
  // n queues
  priority_queue<Item> q_;
  mt19937 rng_;
};

// Straetgy 1, pick first k
struct PickFirst {
 public:
  PickFirst() {}

  void feed(const Item &item) {
    if (item.r > 0.5) n_win ++;
    n ++;
  }

  string info() const {
    stringstream ss;
    ss << "Estimate: " << static_cast<float>(n_win) / n
       << " (" << n_win << "/" << n << ")";
    return ss.str();
  }

 private:
  int n_win = 0;
  int n = 0;
};

struct PickLayer {
 public:
   struct Record {
     vector<bool> win_seq;
   };

   PickLayer(int n_machine)
     : records_(n_machine) {
   }

   void feed(const Item &item) {
     records_[item.from].win_seq.push_back(item.r > 0.5);
   }

   string info() const {
     // Find the smallest n
     int k = 10000;
     for (const auto &p : records_) {
       k = min(k, (int)p.win_seq.size());
     }

     int n_win = 0;
     int n = 0;

     for (const auto &p : records_) {
       for (size_t i = 0; i < k; ++i) {
         if (p.win_seq[i]) n_win ++;
         n ++;
       }
     }

     stringstream ss;
     ss << "Estimate: K = " << k << ", "
        << static_cast<float>(n_win) / n
        << " (" << n_win << "/" << n << ")";
     return ss.str();
   }

 private:
   vector<Record> records_;
};

int main() {
  Generator gen(200);
  PickFirst pick1;
  PickLayer pick2(200);

  for (int i = 0; i < 300; ++i) {
    Item item = gen.Proceed();
    // cout << i << ": " << item.info() << endl;
    pick1.feed(item);
    pick2.feed(item);
  }

  cout << pick1.info() << endl;
  cout << pick2.info() << endl;
  return 0;
}
