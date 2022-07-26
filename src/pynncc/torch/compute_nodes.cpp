#include "compute_nodes.h"

int main() {
    using namespace nncc::nodes;

    auto float_value = FloatValue(1.);
    auto tensor_value = TensorValue(torch::zeros({1, 3, 64, 64}));

    tensor_value = tensor_value.Add(float_value);
    auto sum = torch::sum(As<torch::Tensor>(tensor_value.Get())).item<float>();
    std::cout << sum << std::endl;

    return 0;
}
