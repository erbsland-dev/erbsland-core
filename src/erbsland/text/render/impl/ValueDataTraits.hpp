#pragma once

#include "../Value.hpp"

#include "../../../util/List.hpp"
#include "../../String.hpp"
#include "../../StringMap.hpp"

#include <functional>
#include <variant>

namespace erbsland::text::render::impl {

/// The variant that holds the data for a value.
using ValueDataVariant = std::variant<bool, String, int64_t, double, ValueList, ValueMap, ValueCallbackFn>;

}
