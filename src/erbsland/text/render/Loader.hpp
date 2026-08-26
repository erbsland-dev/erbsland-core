// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LayoutSource.hpp"
#include "Loader_fwd.hpp"

namespace erbsland::text::render {

/// Loads layout sources by logical name.
/// All methods must be thread-safe.
/// @seedoc{/reference/text/render}
/// @notest{Abstract interface; environment and concrete-loader tests cover behavior.}
class Loader {
public:
    // defaults
    virtual ~Loader() = default;

public:
    /// Load a layout.
    /// @param layout The validated logical layout name.
    /// @return The loaded source, or no value when this loader has no matching layout.
    [[nodiscard]] virtual auto load(const String &layout) -> std::optional<LayoutSource> = 0;
};

}
