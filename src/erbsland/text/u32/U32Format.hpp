// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32String.hpp"
#include "U32StringView.hpp"

#include "../impl/FormatData.hpp"
#include "../StringBuilder.hpp"

#include <cstddef>
#include <string_view>

namespace erbsland::text {

/// A reusable validated UTF-32 format.
/// @seedoc{/reference/text/string_formatter}
/// @tested{U32FormatTest}
class U32Format final {
public:
    /// Parse and validate a UTF-32 format pattern.
    /// @throws text::FormatError If the pattern is invalid or exceeds format limits.
    explicit U32Format(std::u32string_view pattern);
    /// Parse and validate a UTF-32 format pattern.
    /// @throws text::FormatError If the pattern is invalid or exceeds format limits.
    explicit U32Format(const U32StringView &pattern);

    // defaults
    ~U32Format() = default;
    U32Format(const U32Format &) = default;
    U32Format(U32Format &&) = default;
    auto operator=(const U32Format &) -> U32Format & = default;
    auto operator=(U32Format &&) -> U32Format & = default;

public: // accessors
    /// Get the number of argument fields in the pattern.
    [[nodiscard]] auto fieldCount() const noexcept -> unit::ArgumentCount;

public: // formatting
    /// Build a UTF-32 string from the arguments.
    /// @throws text::FormatError If arguments do not match the pattern or output exceeds limits.
    template <typename... Args>
    [[nodiscard]] auto build(Args &&...args) const -> U32String;
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

#include "U32Format.tpp"
