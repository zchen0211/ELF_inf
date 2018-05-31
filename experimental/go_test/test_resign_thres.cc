#include <iostream>
#include <random>
#include "elfgames/go/ctrl_selfplay.h"

using namespace std;

int main() {
  GameOptions options;
  elf::ai::tree_search::TSOptions mcts_options;
  SelfPlaySubCtrl selfplay(options, mcts_options);
  selfplay.setCurrModel(0);

  mt19937 rng(time(NULL));
  uniform_real_distribution<> dis(0.0, 1.0);

  const int n = 100000;
  for (int i = 0; i < n; ++i) {
    // Generate a record and feed that in.
    Record r;
    // Fake values
    float black_strength = dis(rng) * 0.5;
    float white_strength = dis(rng) * 0.5;

    normal_distribution<> black_dice(black_strength, 1);
    normal_distribution<> white_dice(white_strength, 1);

    r.result.reward = 0;
    r.request.vers.black_ver = 0;
    r.request.vers.white_ver = -1;

    float resign_thres = selfplay.getResignThreshold();
    if (i % 100 == 0) {
      cout << "Curr threshold: " << resign_thres << endl;
    }

    if (dis(rng) < 0.1) {
      r.result.black_never_resign = r.result.white_never_resign = true;
    }

    int j = 0;
    float odds = 0.0;

    while (j < 700) {
      float diff = 0.1 * (black_dice(rng) - white_dice(rng));
      odds += diff;

      float curr_value = 2.0 / (1.0 + exp(-odds)) - 1.0;

      r.result.values.push_back(curr_value);
      if (curr_value <= -1.0 + resign_thres) {
        if (! r.result.black_never_resign) {
          r.result.reward = -1.0;
          break;
        }
      } else if (curr_value >= 1.0 - resign_thres) {
        if (! r.result.white_never_resign) {
          r.result.reward = 1.0;
          break;
        }
      }
      j ++;
    }

    if (j == 700) {
      r.result.reward = (black_strength > white_strength) ? 1.0 : -1.0;
    }

    if (r.result.black_never_resign || r.result.white_never_resign) {
      cout << endl;
      for (const float v : r.result.values) {
        cout << v << ", ";
      }
      cout << "Finish: reward: " << r.result.reward << ", #move: " << j
           << ", black_never_resign: " << r.result.black_never_resign << ", white_never_resign: "
           << r.result.white_never_resign << ", black_strength: " << black_strength << ", white_strength: " << white_strength << endl;
    }

    selfplay.feed(r);
  }

  return 0;
}
