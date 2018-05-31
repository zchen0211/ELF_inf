#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

#include "elfgames/go/base/go_state.h"
#include "elfgames/go/record.h"
#include "elfgames/go/sgf/sgf.h"

using namespace std;

class Checker {
 public:
  bool check_record(const Record& r) {
    GoState b;
    vector<Coord> moves = sgfstr2coords(r.result.content);
    bool black_win = r.result.reward > 0;

    for (const Coord& m : moves) {
      // show_board(b);
      // ss << endl;
      if (!b.forward(m)) {
        cout << "Invalid move! " << endl;
        break;
      }
    }

    // In the end game, check whether there is any big groups with 1 liberty but
    // is not taken.
    for (int g_idx = 1; g_idx < b.board()._num_groups; ++g_idx) {
      const Group& g = b.board()._groups[g_idx];
      if (g.stones >= 10 && g.liberties == 1) {
        if ((g.color == S_BLACK && black_win &&
             r.result.reward < 2 * g.stones + 1) ||
            (g.color == S_WHITE && !black_win &&
             r.result.reward >= -2 * g.stones - 1)) {
          // show_board(b);
          return false;
        }
      }
    }
    return true;
  }

  /*
  string save_to_sgf(const Record &r) const {
      vector<Coord> moves = sgfstr2coords(r.result.content);
      const vector<float> &values = r.result.values;
  }
  */

 private:
  static constexpr float komi = 7.5;

  void show_board(const GoState& b) {
    cout << "Ply: " << b.getPly() << endl;
    cout << b.showBoard() << endl;
    float score = b.evaluate(komi, &cout);
    cout << "Score: " << score << endl;
  }
};

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cout << "Usage [the program] json_files" << endl;
    return 1;
  }

  Checker checker;

  mutex g_mutex;
  vector<string> chosen;

  vector<thread> threads;

  for (int k = 1; k < argc; ++k) {
    // cout << "Deal with " << argv[k] << endl;
    threads.emplace_back([&, k]() {
      vector<Record> records;
      bool success = Record::from_json_file(string(argv[k]), &records);

      if (!success) {
        lock_guard<mutex> lock(g_mutex);
        cout << argv[k] << " cannot be opened! " << endl;
        return;
      }

      for (size_t i = 0; i < records.size(); ++i) {
        const Record& r = records[i];
        if (!checker.check_record(r)) {
          // cout << "Found one! Filename: " <<  << ", idx = " << i << endl;
          lock_guard<mutex> lock(g_mutex);
          string item = "V: " + to_string(r.result.reward) + ", " +
              string(argv[k]) + " --idx " + to_string(i);
          cout << item << endl;
          chosen.push_back(item);
          // cout << r.result.content << endl;
        }
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  cout << "Total count: " << chosen.size() << endl;

  return 0;
}
