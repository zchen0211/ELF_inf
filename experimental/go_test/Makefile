GCC = ${CXX}

GO = ../../src_cpp/elfgames/go

test_sgf:
	${GCC} -std=c++14 -lstdc++ -I../ test_sgf.cc ../base/go_state.cc ../sgf/sgf.cc ../base/board.cc ../base/board_feature.cc -o test_sgf
test_parallel_eval:
	${GCC} -std=c++14 -lstdc++ -I../../third_party/json/include -I../../third_party/concurrentqueue -I../../src_cpp ../../src_cpp/elfgames/go/client_manager.cc test_parallel_eval.cc -o test_parallel_eval
test_parallel_eval_debug:
	${GCC} -g -std=c++14 -lstdc++ -I../../third_party/json/include -I../../third_party/concurrentqueue -I../../src_cpp ../../src_cpp/elfgames/go/client_manager.cc -fsanitize=address,undefined test_parallel_eval.cc -o test_parallel_eval_debug
test_record:
	${GCC} -std=c++14 -lstdc++ -I../ -I../../ test_recorded_games.cc ${GO}/base/go_state.cc ${GO}/base/board.cc ${GO}/base/board_feature.cc -o test_recorded_games
test_resign_thres:
	${GCC} -std=c++14 -lstdc++ -I../ -I../../src_cpp -I../../third_party/json/single_include test_resign_thres.cc ${GO}/base/go_state.cc ${GO}/base/board.cc ${GO}/base/board_feature.cc -o test_resign_thres
test_mcts_ai:
	${GCC} -std=c++14 -lstdc++ -ltbb -lpthread -I../ -I../../ test_mcts_ai.cc ../base/go_state.cc ../base/board.cc ../base/board_feature.cc -o test_mcts_ai
