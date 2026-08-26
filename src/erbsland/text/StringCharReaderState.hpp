// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/StringReaderBase_fwd.hpp"

#include "../unit/CpIndex.hpp"

#include <cstddef>

namespace erbsland::text {

/// A saved state for `StringReader`.
///
/// This value is intentionally opaque. It stores the reader backend cursor and decoded code-point position. Create it
/// via `StringCharReader::save()` and only pass it back to the same reader or a compatible copy over the same visible
/// text and encoding. Passing it to any other reader is undefined.
/// @seedoc{/reference/text/string_reader}
/// @tested{StringCharReaderTest}
class StringCharReaderState final {
    friend class StringCharReader;
    friend class impl::StringReaderBase;

public:
    // defaults
    StringCharReaderState() = default;
    ~StringCharReaderState() = default;
    StringCharReaderState(const StringCharReaderState &) = default;
    StringCharReaderState(StringCharReaderState &&) = default;
    auto operator=(const StringCharReaderState &) -> StringCharReaderState & = default;
    auto operator=(StringCharReaderState &&) -> StringCharReaderState & = default;

public:
    /// Test if two saved states are equal.
    [[nodiscard]] constexpr auto operator==(const StringCharReaderState &other) const noexcept -> bool {
        return _rawPosition == other._rawPosition && _cpPosition == other._cpPosition;
    }
    /// Test if two saved states differ.
    [[nodiscard]] constexpr auto operator!=(const StringCharReaderState &other) const noexcept -> bool {
        return !operator==(other);
    }

private:
    /// Create a state from the backend and decoded positions.
    constexpr StringCharReaderState(const std::size_t rawPosition, const unit::CpIndex cpPosition) noexcept :
        _rawPosition{rawPosition}, _cpPosition{cpPosition} {}

    /// Access the backend-native position.
    [[nodiscard]] constexpr auto rawPosition() const noexcept -> std::size_t { return _rawPosition; }
    /// Access the decoded code-point position.
    [[nodiscard]] constexpr auto cpPosition() const noexcept -> unit::CpIndex { return _cpPosition; }

private:
    std::size_t _rawPosition{0};                      ///< The raw backend position.
    unit::CpIndex _cpPosition{unit::CpIndex::zero()}; ///< The decoded code-point position.
};

}
