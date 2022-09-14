import redis
import torch


def get_tensor_shm_handle(tensor: torch.Tensor) -> str:
    if not tensor.is_shared():
        raise ValueError("Supplied tensor is not in shared memory.")
        
    manager_handle, filename, _ = tensor.storage()._share_filename_cpu_()

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
            handle = get_tensor_shm_handle(self.storage[name].share_memory_())
            self.handles[name] = handle
        else:
            self.storage[name].copy_(tensor)
            handle = self.handles[name]
        
        self.redis.lpush("nncc_tensors", f"{name}::{handle}")
        
        return handle
    
    
def busywaiting_nncc_request_listener(fn_handles):
    redis_client = redis.Redis(host='localhost', port=6379, db=0)
    
    while True:
        _, value = redis_client.blpop(["nncc_requests"], timeout=0)
            
        value = value.decode("utf-8")
        if value == "::done::":
            break
        fn_handles[value]()
        redis_client.set(f"nncc_{value}", "true")
