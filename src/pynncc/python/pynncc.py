from typing import Union

import redis
import torch


def get_tensor_shm_handle(name: str, tensor: torch.Tensor) -> str:
    if not tensor.is_shared():
        raise ValueError("Supplied tensor is not in shared memory.")

    try:
        manager_handle, filename, _ = tensor.storage()._share_filename_cpu_()
    except AttributeError:
        manager_handle, filename, _ = tensor.storage()._share_filename_()

    dtype = None
    if tensor.dtype == torch.uint8:
        dtype = "uint8"
    elif tensor.dtype == torch.float32:
        dtype = "float32"
    elif tensor.dtype == torch.int32:
        dtype = "int32"

    dims = ",".join([str(dim) for dim in tensor.shape])

    return f"{manager_handle.decode('utf-8')}::{filename.decode('utf-8')}::{dtype}::{dims}"


class NNCCStorage:
    def __init__(self):
        self.redis = redis.Redis(host='localhost', port=6379, db=0)
        self.storage = dict()
        self.vars = dict()
        self.handles = dict()

    def submit_tensor(self, name: str, tensor: torch.Tensor, overwrite: bool = False):
        if (
                name in self.storage
                and not overwrite
        ):
            if self.storage[name].shape != tensor.shape:
                raise ValueError(
                    f"Shape of given tensor {tensor.shape} does not correspond "
                    f"to shared memory storage `{name}`: {self.storage[name].shape}."
                )

            if self.storage[name].dtype != tensor.dtype:
                raise ValueError(
                    f"Dtype of given tensor {tensor.dtype} does not correspond "
                    f"to shared memory storage `{name}`: {self.storage[name].dtype}."
                )

        if name not in self.storage or overwrite:
            self.storage[name] = tensor
            handle = get_tensor_shm_handle(name, self.storage[name].share_memory_())
            self.handles[name] = handle
        else:
            self.storage[name].copy_(tensor)
            handle = self.handles[name]

        self.redis.lpush("__nncc_queue_0", f"share_tensor::{name}::{handle}")

        return handle

    def publish_variable(self, name: str, value: Union[str, float, int]):
        _type = ""

        if isinstance(value, int):
            _type = "int"
        elif isinstance(value, float):
            _type = "float"
        else:
            _type = "str"

        message = f"const::{name}::{_type}::{str(value)}::1"
        queue = "__nncc_queue_0"

        self.vars[name] = value
        self.redis.lpush(queue, message)

    def get(self, name: str):
        return self.vars.get(name, None)


def busywaiting_nncc_request_listener(storage):
    redis_client = redis.Redis(host="localhost", port=6379, db=0)

    while True:
        _, value = redis_client.blpop(["__nncc_queue_1"], timeout=0)
        value = value.decode("utf-8")

        if value.startswith("const"):
            parts = value.split("::")
            name = parts[1]
            _type = parts[2]
            value = parts[3]
            receiver_id = int(parts[4])

            if _type == "int":
                value = int(value)
            elif _type == "float":
                value = float(value)

            storage.vars[name] = value

        if value == "stop::":
            break
        # fn_handles[value]()
        # redis_client.set(f"nncc_{value}", "true")
