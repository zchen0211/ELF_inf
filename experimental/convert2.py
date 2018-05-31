#!/usr/bin/env python

import sys
import torch


import torch._utils
try:
    torch._utils._rebuild_tensor_v2
except AttributeError:
    def _rebuild_tensor_v2(storage, storage_offset, size, stride, requires_grad, backward_hooks):
        tensor = torch._utils._rebuild_tensor(storage, storage_offset, size, stride)
        tensor.requires_grad = requires_grad
        tensor._backward_hooks = backward_hooks
        return tensor
    torch._utils._rebuild_tensor_v2 = _rebuild_tensor_v2


def main():
    f = sys.argv[1]
    model_data = torch.load(f)
    state_dict = model_data['state_dict']

    keys_to_delete = []
    for key in state_dict.keys():
        if key.endswith('.num_batches_tracked'):
            keys_to_delete.append(key)

    for key in keys_to_delete:
        del state_dict[key]

    torch.save(model_data, sys.stdout.buffer)
    sys.stdout.buffer.close()


if __name__ == '__main__':
    main()
