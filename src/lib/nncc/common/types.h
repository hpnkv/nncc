#pragma once

#include <folly/FBString.h>
#include <folly/FBVector.h>

namespace nncc {

using string = folly::fbstring;

template<class T, class Allocator = std::allocator<T>>
using vector = folly::fbvector<T, Allocator>;

}