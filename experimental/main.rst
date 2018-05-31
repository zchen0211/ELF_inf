.. footer::

    Copyright |copy| 2018-present, Facebook, Inc. |---|
    all rights reserved.

.. |copy| unicode:: 0xA9
.. |---| unicode:: U+02014

=====================
ELF2 Python Interface
=====================

bash scripts to run:
====================
In the folder experiment
 1. Training: 
agz_train.sh, calls train.py

 2. Selfplay: 
agz_train.sh, calls df_selfplay.py

 3. Debug mode: print value/policy and dump tree
agz_debug_224.sh, df_selfplay.py

 4. Gtp mode: to play on cgos and with human
agz_gtp.sh, df_console.py

Python C++ Binding in elfgames_go
=================================
Classes written in C++ with Python interface:
 1. ``GameContext``: with context, games (of type GameBase, can be either selfplay or train), and train/eval control, reader/writer, network-options (like server, address, ipv6, port, ...), go-feature, 
 2. ContextOptions
 3. GameOptions
 4. GameStats
 5. ``GoGameSelfPlay``

Python C++ Binding in elf
=========================
Classes written in C++ with Python interface:
 1. enum ReplyStatus {"SUCCESS", "FAILED", "UNKNOWN"}
 2. ``Context``, with wait(), step(), stop(), version(), allocateSharedMem() and createSharedMemOptions() available
 3. SharedMemOptions
 4. ``SharedMem``: __getitem__(), info(), ...
 5. ``AnyP``, with info(), field(), set() provided
 6. ``FuncMapBase``, with batchsize, name, ...
 7. SearchAlgoOptions
 8. TSOptions
 9. logging

2. ``Model``
============
class ``Model_PolicyValue`` in Model file

3. ``Model file``
=================
pytorch scripts in elfgames.go.df_model3

4. class ``Trainer`` and ``Evaluator``
======================================

5. Model loader
===============
will call env=load_env() and setup everything;
  a. ``game``: class Loader() defined in elfgames/go/game.py
  b. ``method``:
     i. class ``MCTSPrediction`` defined in elfgames/go/mcts_predction.py in case of testing/selfplay;
     ii. not used in training;
  c. ``module_loaders``:
     i. class ``Model_PolicyValue``;
  d. ``sampler``: class Sampler
     defined in rlpytorch/sampler/sampler.py
  e. ``mi``: model interface;

6. ``GameContext``
==================
GC = env["game"].initialize()
 a. Set co = go.ContextOptions()
 b. GC = go.GameContext(co, opt), a class defined in C++ with pybind binding; it will put either ``GoGameSelfPlay`` or ``GoGameTrain`` in vector according to running mode;
 c. class GCWrapper(GC) wraps on the GameContext class, augmented with batchdim, gpu, GC
     _cb (callbacks);
     
7.1 ``Self Play`` mode
======================
 1. Addition to load in env:
  1. "eval_actor_black": Evaluate()
  2. "eval_actor_white": Evaluate()
  3. "mi_actor_black": ModelInterface()
  4. "mi_actor_white": ModelInterface()
 2. **load_env** env will be a dictionary with "game", "method", "sampler", "mi", "module_loaders" as in section 5, plus the addition to load.
 3. GC = env["game"].initialize() as in section 6.
 4. GC Register in member _cb to run Neural-Network with Evaluator.actor(batch)
  1. "actor_black": 
  2. "actor_white":
  3. "game_start": try reload model env["mi_actor_black"] and env["mi_actor_white"]
  4. "game_end": print winrate

 5. **GC.start()** defined in utils_elf.py:
  1. check_callbacks()
  2. **GC.ctx().start()** C++ context starts, will start collector, server, client and emplace_back callbacks in threads
 6. while not loop_end, call GC.run()
  1. **smem = self.GC.ctx.wait()** to wait for a batch
  2. **self._call(smem)** run the batch
  3. **self.GC.ctx().step()** to release batch;
 7. **GC.stop()** to stop the run;
     message queue stop;
     compeleted switch reset;
     thread join;

7.2 ``Training`` mode
=====================
 1. Addition to load in env:
  1. "trainer": Trainer()
  2. "runner": SingleProcessRun()
 2. Register in GC()._cb to run Neural-Network with actor(batch, e)
  1. "actor":
  2. "train": trainer.train(batch)
  3. "train-ctrl": remove and reload "model" env["mi"]

 b. trainer.setup()

 c. runner.setup(GC)

 d. runner.run()
