// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32StringDataView_fwd.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpLength.hpp"
#include "../../../unit/CpRange.hpp"

#include <algorithm>
#include <span>

namespace erbsland::text::impl {

/// The view to the string data.
/// @tested{U32StringTest}
class U32StringDataView final {
public:
    /// Create an empty view.
    constexpr U32StringDataView() noexcept = default;
    /// Create a view to a selected range in a UTF-32 code-unit span.
    constexpr U32StringDataView(std::span<const char32_t> data, unit::CpRange range) noexcept :
        _data{data}, _range{range} {}

public: // accessors
    /// Access the complete backing data span.
    [[nodiscard]] constexpr auto data() const noexcept -> std::span<const char32_t> { return _data; }
    /// Access the selected UTF-32 code-unit range in the backing data.
    [[nodiscard]] constexpr auto range() const noexcept -> unit::CpRange { return _range; }
    /// Create a bounded span to the selected byte range.
    [[nodiscard]] constexpr auto dataSpan() const noexcept -> std::span<const char32_t> {
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
    /// Convert an absolute UTF-32 code-unit range in this data view into a relative range in `dataSpan()`.
    [[nodiscard]] constexpr auto relativeRangeForAbsolute(unit::CpRange range) const noexcept -> unit::CpRange {
        const auto span = dataSpan();
        if (span.empty() || !range.isValid()) {
            return unit::CpRange::empty();
        }
        const auto origin = _range.index().toSizeT();
        if (range.index().toSizeT() < origin) {
            return unit::CpRange::empty();
        }
        const auto relativeStart = range.index().toSizeT() - origin;
        return unit::CpRange{unit::CpIndex::fromSizeT(relativeStart), range.length()}.clampedTo(
            unit::CpLength::fromSizeT(span.size()));
    }

private:
    std::span<const char32_t> _data;              ///< The span to the character storage.
    unit::CpRange _range{unit::CpRange::empty()}; ///< The range the string covers.
};

}
