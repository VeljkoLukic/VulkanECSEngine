#pragma once
#include <cstdint>

namespace ecs {

using EntityID = uint32_t;

static constexpr EntityID NullEntity = ~EntityID(0);

} // namespace ecs
