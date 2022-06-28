#pragma once

#include <cstdint>
#include <memory>
#include <utility>

#include "world_math.h"

namespace nncc::engine {

class WorldObject : public std::enable_shared_from_this<WorldObject> {
public:
    virtual void Init() = 0;

    virtual void Update() = 0;

    virtual void Destroy() = 0;

    void Unlink() {

    }

    Vec3 transform{0, 0, 0};

protected:
    int64_t id_;
};

class ObjectContainer {
public:
private:

};

}