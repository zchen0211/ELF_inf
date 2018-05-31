#include <iostream>
#include <random>
#include "../base/go_state.h"
#include "../sgf/sgf.h"

using namespace std;

void plot_feature(const vector<float>& f, int k) {
  const float* plane = &f[k * BOARD_SIZE * BOARD_SIZE];
  for (int i = 0; i < BOARD_SIZE; ++i) {
    for (int j = 0; j < BOARD_SIZE; ++j) {
      cout << plane[j * BOARD_SIZE + BOARD_SIZE - 1 - i] << " ";
    }
    cout << endl;
  }
}

void plot_features(const vector<float>& f) {
  // Print that feature out.
  const int num_channels = f.size() / BOARD_SIZE / BOARD_SIZE;
  for (int k = 0; k < num_channels; ++k) {
    cout << "Plane " << k << ":" << endl;
    plot_feature(f, k);
    cout << endl;
  }
}

bool test_vector(const float* v1, const float* v2, int len) {
  for (int i = 0; i < len; ++i) {
    if (v1[i] != v2[i])
      return false;
  }
  return true;
}

bool test_agz_feature(
    const vector<float>& f_curr,
    const vector<float>& f_last) {
  const int d = BOARD_SIZE * BOARD_SIZE;
  const int len = f_curr.size() / d - 4;

  for (int i = 0; i < len / 2; ++i) {
    const float* curr_last = &f_curr[(2 * i + 2) * d];
    const float* last = &f_last[(2 * i + 1) * d];
    if (!test_vector(curr_last, last, d)) {
      return false;
    }

    const float* curr_last2 = &f_curr[(2 * i + 3) * d];
    const float* last2 = &f_last[(2 * i) * d];
    if (!test_vector(curr_last2, last2, d)) {
      return false;
    }

    /*

for (int i = 0; i < len; ++i) {
    if ( abs(curr_last[i] - last[i]) > 1e-10) {
        cout << "History check wrong at " << i << ": curr_last = " <<
curr_last[i] << ", last = " << last[i] << endl;

        int plane = i / (BOARD_SIZE * BOARD_SIZE);
        cout << "Curr feature (plane: " << plane + 2 << ")" << endl;
        plot_feature(f_curr, plane + 2);

        cout << "Last feature (plane: " << plane << ")" << endl;
        plot_feature(f_last, plane);
        return false;
    }
    */
  }

  // Check whether alternating.
  if (!test_vector(&f_curr[(len + 2) * d], &f_last[(len + 2 + 1) * d], d)) {
    cout << "Player check wrong " << endl;
    return false;
  }

  if (!test_vector(&f_curr[(len + 2 + 1) * d], &f_last[(len + 2) * d], d)) {
    cout << "Player check wrong " << endl;
    return false;
  }

  return true;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cout << "Usage [the program] sgf_file" << endl;
    return 1;
  }
  Sgf sgf;
  sgf.load(argv[1]);
  cout << sgf.printHeader() << endl;
  cout << sgf.printMainVariation() << endl;

  GoState b;

  vector<float> last_features;

  auto iter = sgf.begin();
  while (!iter.done()) {
    auto curr = iter.getCurrMove();

    cout << "Before Move: " << b.getPly() << endl;

    cout << b.showBoard() << endl;
    float score = b.evaluate(&cout);
    cout << "Score (no komi): " << score << endl;
    // Dump features.
    const BoardFeature& bf = b.extractor();
    vector<float> features;
    bf.extractAGZ(&features);

    if (last_features.size() == features.size()) {
      if (!test_agz_feature(features, last_features)) {
        return 1;
      }
    }

    last_features = features;

    GoState b2(b);
    const BoardFeature& bf2 = b2.extractor();
    vector<float> features2;
    bf2.extractAGZ(&features2);

    if (!test_vector(&features[0], &features2[0], features.size())) {
      return 1;
    }

    // Print that feature out.
    // plot_features(features);

    cout << "[" << iter.getCurrIdx() << "]: " << STR_STONE(curr.player) << "["
         << coord2str(curr.move) << "][" << coord2str2(curr.move) << "]"
         << endl;
    // ss << endl;
    if (!b.forward(curr.move)) {
      cout << "Invalid move! " << endl;
      break;
    }
    ++iter;
  }
  // cout << sgf.printMainVariation() << endl;
  //
  cout << b.showBoard() << endl;
  float score = b.evaluate(&cout);
  cout << "Final score (no komi): " << score << endl;
  return 0;
}
