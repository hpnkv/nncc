#pragma once

#include <entt/entt.hpp>

#include <nncc/context/context.h>

namespace nncc::gui {

// TODO: uniform handling of events for widgets

class Widget {
public:
    Widget() : context(*context::Context::Get()) {
    }

    context::Context& context;

    entt::delegate<void(Widget*)> init;
    entt::delegate<void(Widget*)> destroy;
    entt::delegate<void(Widget*)> update;

    std::shared_ptr<void> state;

private:
};

}