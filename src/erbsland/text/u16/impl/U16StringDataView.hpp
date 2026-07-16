// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringDataView_fwd.hpp"

#include "../../../unit/U16DataIndex.hpp"
#include "../../../unit/U16DataLength.hpp"
#include "../../../unit/U16DataRange.hpp"

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
    constexpr U16StringDataView(std::span<const char16_t> data, unit::U16DataRange range) noexcept :
        _data{data}, _range{range} {}

public: // accessors
    /// Access the complete backing data span.
    [[nodiscard]] constexpr auto data() const noexcept -> std::span<const char16_t> { return _data; }
    /// Access the selected UTF-16 code-unit range in the backing data.
    [[nodiscard]] constexpr auto range() const noexcept -> unit::U16DataRange { return _range; }
    /// Create a bounded span to the selected byte range.
    [[nodiscard]] constexpr auto dataSpan() const noexcept -> std::span<const char16_t> {
        if (_data.empty() || !_range.isValid() || _range.isEmpty()) {
            return {};
        }
        const auto start = _range.index().toSizeT();
        if (start >= _data.size()) {
            return {};
        }
        const auto availableLength = _data.size() - start;
        const auto length = std::min(_range.length().toSizeT(), availableLength);
        return _data.subspan(start, length);
    }
    /// Convert an absolute UTF-16 code-unit range in this data view into a relative range in `dataSpan()`.
    [[nodiscard]] constexpr auto relativeRangeForAbsolute(unit::U16DataRange range) const noexcept
        -> unit::U16DataRange {
        const auto span = dataSpan();
        if (span.empty() || !range.isValid()) {
            return unit::U16DataRange::empty();
        }
        const auto origin = _range.index().toSizeT();
        if (range.index().toSizeT() < origin) {
            return unit::U16DataRange::empty();
        }
        const auto relativeStart = range.index().toSizeT() - origin;
        return unit::U16DataRange{unit::U16DataIndex::fromSizeT(relativeStart), range.length()}.clampedTo(
            unit::U16DataLength::fromSizeT(span.size()));
    }

private:
    std::span<const char16_t> _data;                        ///< The span to the character storage.
    unit::U16DataRange _range{unit::U16DataRange::empty()}; ///< The range the string covers.
};

}
