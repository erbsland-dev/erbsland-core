// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../support/TestHelper.hpp"

#include <memory>

/// Test double recording readable-buffer dispatch calls.
/// @notest{Test-only probe.}
class ReadableBufferDispatchProbe final : public ReadableBuffer {
public:
    [[nodiscard]] auto size() const noexcept -> bgeo::BlockSize override { return _buffer.size(); }

    [[nodiscard]] auto rect() const noexcept -> bgeo::BlockRectangle override { return _buffer.rect(); }

    [[nodiscard]] auto get(const bgeo::BlockPosition pos) const noexcept -> const Block & override {
        return _buffer.get(pos);
    }

    [[nodiscard]] auto clone() const -> WritableBufferPtr override { return std::make_shared<Buffer>(_buffer); }

public:
    mutable erbsland::text::CharSet _characters;
    mutable bool _invert = false;

protected:
    [[nodiscard]] auto toMaskImpl(const erbsland::text::CharSet &characters, const bool invert) -> Bitmap override {
        _characters = characters;
        _invert = invert;
        return Bitmap{_buffer.size()};
    }

private:
    Buffer _buffer{bgeo::BlockSize{2, 1}};
};
