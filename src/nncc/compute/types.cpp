#include <nncc/compute/types.h>

namespace nncc::compute {

static nncc::compute::DataTypeRegistry data_type_registry;

DataTypeRegistry* DataTypeRegistry::Get() {
    return &data_type_registry;
}

}