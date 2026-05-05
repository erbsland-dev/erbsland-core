// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String.hpp"
#include "U16StringView.hpp"

#include "../impl/FormatData.hpp"
#include "../StringBuilder.hpp"

#include <cstddef>
#include <string_view>

namespace erbsland::text {

/// A reusable validated UTF-16 format.
/// @seedoc{/reference/text/string_formatter}
/// @tested{U16FormatTest}
class U16Format final {
public:
    /// Parse and validate a UTF-16 format pattern.
    /// @throws err::FormatError If the pattern is invalid or exceeds format limits.
    explicit U16Format(std::u16string_view pattern);
    /// Parse and validate a UTF-16 format pattern.
    /// @throws err::FormatError If the pattern is invalid or exceeds format limits.
    explicit U16Format(const U16StringView &pattern);

    // defaults
    ~U16Format() = default;
    U16Format(const U16Format &) = default;
    U16Format(U16Format &&) = default;
    auto operator=(const U16Format &) -> U16Format & = default;
    auto operator=(U16Format &&) -> U16Format & = default;

public: // accessors
    /// Get the number of argument fields in the pattern.
    [[nodiscard]] auto fieldCount() const noexcept -> unit::ArgumentCount;

public: // formatting
    /// Build a UTF-16 string from the arguments.
    /// @throws err::FormatError If arguments do not match the pattern or output exceeds limits.
    template <typename... Args>
    [[nodiscard]] auto build(Args &&...args) const -> U16String;
    /// Append formatted arguments to a string builder.
    /// @throws err::FormatError If arguments do not match the pattern or output exceeds limits.
    template <typename... Args>
    auto appendTo(StringBuilder &builder, Args &&...args) const -> StringBuilder &;

private:
    using DataPtr = impl::FormatDataPtr;

private:
    DataPtr _data; ///< The compiled shared format data.
};

}

#include "U16Format.tpp"
