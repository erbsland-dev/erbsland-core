// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../support/TestHelper.hpp"

class BitmapAccessor final : public Bitmap {
public:
    using Bitmap::Bitmap;

    auto readPixelRef(const bgeo::BlockPosition pos) -> bool { return pixelRef(pos); }

    void writePixelRef(const bgeo::BlockPosition pos, const bool value) { pixelRef(pos) = value; }
};
