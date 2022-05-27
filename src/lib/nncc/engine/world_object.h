#pragma once

namespace nncc::engine {

class WorldObject {
public:
    virtual void Init() = 0;

    virtual void Update() = 0;
    
private:
};

}