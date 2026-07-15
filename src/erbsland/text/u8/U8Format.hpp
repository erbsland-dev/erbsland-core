// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String.hpp"
#include "U8StringView.hpp"

#include "../impl/FormatData.hpp"
#include "../StringBuilder.hpp"

#include <cstddef>
#include <string_view>

namespace erbsland::text {

/// A reusable validated UTF-8 format.
/// @seedoc{/reference/text/string_formatter}
/// @tested{U8FormatTest}
class U8Format final {
public:
    /// Parse and validate a UTF-8 format pattern.
    /// @throws text::FormatError If the pattern is invalid or exceeds format limits.
    explicit U8Format(std::string_view pattern);
    /// Parse and validate a UTF-8 format pattern.
    /// @throws text::FormatError If the pattern is invalid or exceeds format limits.
    explicit U8Format(const U8StringView &pattern);

    // defaults
    ~U8Format() = default;
    U8Format(const U8Format &) = default;
    U8Format(U8Format &&) = default;
    auto operator=(const U8Format &) -> U8Format & = default;
    auto operator=(U8Format &&) -> U8Format & = default;

public: // accessors
    /// Get the number of argument fields in the pattern.
    [[nodiscard]] auto fieldCount() const noexcept -> unit::ArgumentCount;

public: // formatting
    /// Build a UTF-8 string from the arguments.
    /// @throws text::FormatError If arguments do not match the pattern or output exceeds limits.
    template <typename... Args>
    [[nodiscard]] auto build(Args &&...args) const -> U8String;
    /// Append formatted arguments to a string builder.
    /// @throws text::FormatError If arguments do not match the pattern or output exceeds limits.
    template <typename... Args>
    auto appendTo(StringBuilder &builder, Args &&...args) const -> StringBuilder &;

private:
    using DataPtr = impl::FormatDataPtr;

private:
    DataPtr _data; ///< The compiled shared format data.
};

}

#include "U8Format.tpp"
