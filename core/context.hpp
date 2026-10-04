#pragma once
#include "device.hpp"
#include "instance.hpp"
#include "allocator.hpp"
/*
    Structures to bundle objects that often go together
*/

namespace core
{
    struct context
    {
        context(const core::device& aDevice,
        const core::allocator& aAlloc,
        const core::instance& aInstance)
        : device(aDevice), alloc(aAlloc), instance(aInstance)
        {};

        const core::device& device;
        const core::allocator& alloc;
        const core::instance& instance;
    };
}