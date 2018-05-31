#include "test_elf.h"

using namespace std;

/*
ReceiverCB SimpleCallback() {
    return [](Comm::Server *server, vector<Message> *tasks) {
        for (auto &task : *tasks) {
            task.m->reply = task.m->seq + task.m->id;
        }
    };
}
*/

int main() {
    int input_T = 2;

    elf::OptionMap optionMap(getOptionSpec());
    optionMap.loadJSON({
        {"batchsize", 8},
        {"input_T", 2},
        {"num_games", 32},
    });

    GameContext gc(optionMap);
    gc.allocate();
    gc.allocate();

    gc.ctx()->start();

    // Main loop
    for (int k = 0; k < 10; ++k) {
        auto smem = *gc.ctx()->wait();
        auto *value = smem["value"];
        auto *reply = smem["reply"];
        for (int i = 0; i < smem.getEffectiveBatchSize(); ++i) {
             std::cout << "value: ";
             for (int j = 0; j < input_T; ++j) {
                 std::cout << *value->getAddress<int>({i, j}) << " ";
             }
             std::cout << std::endl;
             *reply->getAddress<int>(i) = *value->getAddress<int>({i, 0}) + *value->getAddress<int>({i, 1}) + 1;
        }
        gc.ctx()->step();
    }

    cout << "Time to stop" << endl;
    gc.ctx()->stop();
}
