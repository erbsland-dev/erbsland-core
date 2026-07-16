// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringBuilder_fwd.hpp"

#include "../U8String.hpp"

#include "../../impl/StringBuilderBase.hpp"

namespace erbsland::text::impl {

/// A string builder backend for UTF-8 strings.
/// @tested{StringBuilderTest}
class U8StringBuilder final : public StringBuilderBase {
public:
    U8StringBuilder() = default;
    explicit U8StringBuilder(unit::ByteLength capacity);
    U8StringBuilder(const U8StringBuilder &) = default;
    U8StringBuilder(U8StringBuilder &&) = default;
    auto operator=(const U8StringBuilder &) -> U8StringBuilder & = default;
    auto operator=(U8StringBuilder &&) -> U8StringBuilder & = default;
    ~U8StringBuilder() override = default;

public:
    [[nodiscard]] auto clone() const -> U8StringBuilder * override;
    [[nodiscard]] auto kind() const noexcept -> StringKind override;
    [[nodiscard]] auto length() const noexcept -> unit::CpLength override;
    [[nodiscard]] auto isEmpty() const noexcept -> bool override;
    void clear() noexcept override;
    void append(Char character) override;
    void append(Char character, unit::CpLength count) override;
    void append(const U8StringView &text) override;
    void append(const U8StringView &text, unit::ElementCount count) override;
    void append(const U16StringView &text) override;
    void append(const U16StringView &text, unit::ElementCount count) override;
    void append(const U32StringView &text) override;
    void append(const U32StringView &text, unit::ElementCount count) override;
    [[nodiscard]] auto toU8String() const -> U8String override;
    [[nodiscard]] auto toU16String() const -> U16String override;
    [[nodiscard]] auto toU32String() const -> U32String override;
    [[nodiscard]] auto takeU8String() -> U8String override;
    [[nodiscard]] auto takeU16String() -> U16String override;
    [[nodiscard]] auto takeU32String() -> U32String override;
    [[nodiscard]] auto toAnyString() const -> AnyString override;
    [[nodiscard]] auto takeAnyString() -> AnyString override;

private:
    U8String _text;                                 ///< The built string.
    unit::CpLength _length{unit::CpLength::zero()}; ///< The cached decoded code-point length.
};

}
