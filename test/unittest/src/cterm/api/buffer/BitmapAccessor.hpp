// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../support/TestHelper.hpp"

/// Test-only access to protected bitmap pixel references.
/// @notest{Used only by bitmap unit tests.}
class BitmapAccessor final : public Bitmap {
public:
    using Bitmap::Bitmap;

    /// Read a pixel through the protected reference accessor.
    auto readPixelRef(const bgeo::BlockPosition pos) -> bool { return pixelRef(pos); }

    /// Write a pixel through the protected reference accessor.
    void writePixelRef(const bgeo::BlockPosition pos, const bool value) { pixelRef(pos) = value; }
};
