#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import os
import re

from rlpytorch import load_env, SingleProcessRun, Trainer, ModelInterface

matcher = re.compile(r"save-(\d+).bin")

if __name__ == '__main__':
    additional_to_load = {
        'trainer0': (
            Trainer.get_option_spec(),
            lambda option_map: Trainer(option_map)),
        'trainer1': (
            Trainer.get_option_spec(),
            lambda option_map: Trainer(option_map)),
        'mi0': (
            ModelInterface.get_option_spec(), ModelInterface),
        'mi1': (
            ModelInterface.get_option_spec(), ModelInterface),
        'runner': (
            SingleProcessRun.get_option_spec(),
            lambda option_map: SingleProcessRun(option_map)),
    }

    env = load_env(os.environ, num_models=2,
                   additional_to_load=additional_to_load,
                   overrides=dict(backprop0=False,
                                  backprop1=False, mode="offline_train"))

    trainer0 = env['trainer0']
    trainer1 = env['trainer1']
    runner = env['runner']

    GC = env["game"].initialize()

    for i in range(2):
        model_loader = env["model_loaders"][i]
        model = model_loader.load_model(GC.params)
        env["mi%d" % i].add_model("model", model)
        env["mi%d" % i]["model"].eval()

    model_ver = 0
    model_filename = model_loader.options.load
    if isinstance(model_filename, str) and model_filename != "":
        realpath = os.path.realpath(model_filename)
        m = matcher.match(os.path.basename(realpath))
        if m:
            model_ver = int(m.group(1))

    eval_old_model = env["game"].options.eval_old_model

    if eval_old_model >= 0:
        GC.GC.setEvalMode(model_ver, eval_old_model)
    else:
        GC.GC.setInitialVersion(model_ver)

    selfplay_ver = model_ver
    root = os.environ["save"]
    print("Root: " + root)

    def pseudo_train(batch, *args, **kwargs):
        global trainer0, trainer1
        # Check whether the version match.
        trainer0.train(batch, *args, **kwargs)
        trainer1.train(batch, *args, **kwargs)

    GC.reg_callback("train", pseudo_train)
    GC.reg_callback("train_ctrl", lambda: None)

    trainer0.setup(
        sampler=env["sampler"],
        mi=env["mi0"],
        rl_method=env["method"])

    trainer1.setup(
        sampler=env["sampler"],
        mi=env["mi1"],
        rl_method=env["method"])

    def episode_start(i):
        trainer0.episode_start(i)
        trainer1.episode_start(i)

    def episode_summary(i):
        print('Summary for trainer0')
        trainer0.episode_summary(i, save=False)
        print('Summary for trainer1')
        trainer1.episode_summary(i, save=False)

    runner.setup(GC, episode_summary=episode_summary,
                 episode_start=episode_start)

    runner.run()
