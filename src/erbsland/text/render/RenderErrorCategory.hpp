// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::render {

/// The category of a layout-rendering failure.
enum class RenderErrorCategory : uint8_t {
    InvalidLayoutName, ///< A logical layout name is invalid.
    LayoutNotFound,    ///< No loader found the requested layout.
    Load,              ///< Loading a layout failed.
    Syntax,            ///< Layout syntax is invalid or unsupported.
    Runtime,           ///< Program execution failed.
    Limit,             ///< A renderer safety limit was exceeded.
    Internal,          ///< Compiled bytecode or an internal invariant is invalid.
};

}
