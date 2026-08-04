// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringBuilder_fwd.hpp"

#include "../U16StringEditor.hpp"

#include "../../impl/AnyStringBuilderBase.hpp"

namespace erbsland::text::impl {

/// A string builder backend for UTF-16 strings.
/// @tested{AnyStringBuilderTest}
class U16StringBuilder final : public AnyStringBuilderBase {
public:
    /// Create a builder with initial `capacity`.
    explicit U16StringBuilder(unit::U16DataLength capacity);

    // defaults
    U16StringBuilder() = default;
    U16StringBuilder(const U16StringBuilder &) = default;
    U16StringBuilder(U16StringBuilder &&) = default;
    auto operator=(const U16StringBuilder &) -> U16StringBuilder & = default;
    auto operator=(U16StringBuilder &&) -> U16StringBuilder & = default;
    ~U16StringBuilder() override = default;

public:
    [[nodiscard]] auto clone() const -> U16StringBuilder * override;
    [[nodiscard]] auto kind() const noexcept -> StringKind override;
    [[nodiscard]] auto length() const noexcept -> unit::CpLength override;
    [[nodiscard]] auto isEmpty() const noexcept -> bool override;
    void clear() noexcept override;
    auto append(Char character) -> unit::CpLength override;
    void append(Char character, unit::CpLength count) override;
    auto append(const U8String &text) -> unit::CpLength override;
    void append(const U8String &text, unit::ItemCount count) override;
    auto append(const U16String &text) -> unit::CpLength override;
    void append(const U16String &text, unit::ItemCount count) override;
    auto append(const U32String &text) -> unit::CpLength override;
    void append(const U32String &text, unit::ItemCount count) override;
    void appendByteBlock(const mem::ByteBlock &bytes, const ByteFormat &format) override;
    [[nodiscard]] auto toU8StringEditor() const -> U8StringEditor override;
    [[nodiscard]] auto toU16StringEditor() const -> U16StringEditor override;
    [[nodiscard]] auto toU32StringEditor() const -> U32StringEditor override;
    [[nodiscard]] auto takeU8StringEditor() -> U8StringEditor override;
    [[nodiscard]] auto takeU16StringEditor() -> U16StringEditor override;
    [[nodiscard]] auto takeU32StringEditor() -> U32StringEditor override;
    [[nodiscard]] auto toAnyStringEditor() const -> AnyStringEditor override;
    [[nodiscard]] auto takeAnyStringEditor() -> AnyStringEditor override;

private:
    U16StringEditor _text;                          ///< The built string.
    unit::CpLength _length{unit::CpLength::zero()}; ///< The cached decoded code-point length.
};

}
