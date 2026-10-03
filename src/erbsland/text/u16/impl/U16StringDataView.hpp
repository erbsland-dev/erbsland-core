// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringDataView_fwd.hpp"

#include "../../../unit/U16Count.hpp"
#include "../../../unit/U16Index.hpp"
#include "../../../unit/U16Range.hpp"

#include <algorithm>
#include <span>

namespace erbsland::text::impl {

/// The view to the string data.
/// @tested{U16StringTest}
class U16StringDataView final {
public:
    /// Create an empty view.
    constexpr U16StringDataView() noexcept = default;
    /// Create a view to a selected range in a UTF-16 code-unit span.
    constexpr U16StringDataView(std::span<const char16_t> data, unit::U16Range range) noexcept :
        _data{data}, _range{range} {}

public: // tests
    /// Test if the selected range is a slice of the backing string data.
    [[nodiscard]] constexpr auto isSlice() const noexcept -> bool {
        if (_data.empty() || !_range.isValid() || _range.isEmpty()) {
            return false;
        }
        const auto backingLength = _data.back() == u'\0' ? _data.size() - 1U : _data.size();
        return !_range.beginIndex().isZero() || _range.length().toSizeT() != backingLength;
    }

public: // accessors
    /// Access the complete backing data span.
    [[nodiscard]] constexpr auto data() const noexcept -> std::span<const char16_t> { return _data; }
    /// Access the selected UTF-16 code-unit range in the backing data.
    [[nodiscard]] constexpr auto range() const noexcept -> unit::U16Range { return _range; }
    /// Create a bounded span to the selected byte range.
    [[nodiscard]] constexpr auto dataSpan() const noexcept -> std::span<const char16_t> {
        if (_data.empty() || !_range.isValid() || _range.isEmpty()) {
            return {};
        }
        const auto start = _range.beginIndex().toSizeT();
        if (start >= _data.size()) {
            return {};
        }
        const auto availableLength = _data.size() - start;
        const auto length = std::min(_range.length().toSizeT(), availableLength);
        return _data.subspan(start, length);
    }
    /// Convert an absolute UTF-16 code-unit range in this data view into a relative range in `dataSpan()`.
    [[nodiscard]] constexpr auto relativeRangeForAbsolute(unit::U16Range range) const noexcept -> unit::U16Range {
        const auto span = dataSpan();
        if (span.empty() || !range.isValid()) {
            return unit::U16Range::empty();
        }
        const auto origin = _range.beginIndex().toSizeT();
        if (range.beginIndex().toSizeT() < origin) {
            return unit::U16Range::empty();
        }
        const auto relativeStart = range.beginIndex().toSizeT() - origin;
        return unit::U16Range{unit::U16Index::fromSizeT(relativeStart), range.length()}.clampedTo(
            unit::U16Count::fromSizeT(span.size()));
    }

private:
    std::span<const char16_t> _data;                ///< The span to the character storage.
    unit::U16Range _range{unit::U16Range::empty()}; ///< The range the string covers.
};

}
