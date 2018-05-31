import sys

import elf
import test_elf as test
from elf.options import auto_import_options, PyOptionSpec

sys.path.insert(0, "../")

spec = {}

spec["test"] = dict(
    input=["value"],
    reply=["reply"],
    # timeout_usec = 10,
)

typename_to_type = {
    'str': str,
    'int': int,
    'float': float,
    'bool': bool,
}


class runGC(object):
    @classmethod
    def get_option_spec(cls):
        spec = test.getOptionSpec()
        return spec

    @auto_import_options
    def __init__(self, option_map):
        self.GC = test.GameContext(option_map)
        self.wrapper = elf.GCWrapper(
            self.GC, int(self.options.batchsize), spec, gpu=0, num_recv=2)
        self.wrapper.reg_callback("test", self.on_batch)

    def on_batch(self, batch):
        print("Receive batch: ", batch.smem.info(),
              ", curr_batchsize: ", str(batch.batchsize), sep='')
        reply = batch["value"][:, 0] + batch["value"][:, 1] + 1
        return dict(reply=reply)

    def start(self):
        self.wrapper.start()

    def run(self):
        self.wrapper.run()


if __name__ == '__main__':
    option_spec = PyOptionSpec(RunGC.get_option_spec())
    option_map = option_spec.parse()

    rungc = RunGC(option_map)
    rungc.start()
    for i in range(10):
        rungc.run()

    print("Stopping................")
    rungc.wrapper.stop()
