// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/u32/U32String.hpp"
#include "../../../unit/CpIndex.hpp"
#include "../../Input32.hpp"

namespace erbsland::re::impl {

/// An input that reads from an owning UTF-32 read-only string.
class U32StringInput final : public Input32 {
public:
    /// Create an input that reads the given UTF-32 string.
    [[nodiscard]] static auto create(const text::U32String &text) noexcept -> Input32Ptr {
        return std::make_shared<U32StringInput>(text);
    }
    /// Create an input that retains a copy of `text`.
    explicit U32StringInput(const text::U32String &text) noexcept : _text{text} {}

public: // implement InputBase
    [[nodiscard]] auto read() -> CharAndPosition override;
    [[nodiscard]] auto peek() -> CharAndPosition override;
    void skip(unit::CpLength characterCount) override;

public: // implement Input32
    [[nodiscard]] auto createMatch(CaptureGroupList captureGroupList) -> Match32Ptr override;

private:
    text::U32String _text;
    unit::CpIndex _position;
};

}
