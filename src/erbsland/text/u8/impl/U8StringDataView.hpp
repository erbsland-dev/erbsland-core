// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ByteLength.hpp"
#include "../../../unit/ByteRange.hpp"

#include <algorithm>
#include <span>

namespace erbsland::text::impl {

/// The view to the string data.
/// @tested{U8StringDataViewTest}
class U8StringDataView final {
public:
    /// Create an empty view.
    constexpr U8StringDataView() noexcept = default;
    /// Create a view to a selected range in a byte span.
    constexpr U8StringDataView(std::span<const char> data, unit::ByteRange range) noexcept :
        _data{data}, _range{range} {}

public: // accessors
    /// Access the complete backing data span.
    [[nodiscard]] constexpr auto data() const noexcept -> std::span<const char> { return _data; }
    /// Access the selected byte range in the backing data.
    [[nodiscard]] constexpr auto range() const noexcept -> unit::ByteRange { return _range; }
    /// Create a bounded span to the selected byte range.
    [[nodiscard]] constexpr auto dataSpan() const noexcept -> std::span<const char> {
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
    /// Convert an absolute byte range in this data view into a relative range in `dataSpan()`.
    [[nodiscard]] constexpr auto relativeRangeForAbsolute(unit::ByteRange range) const noexcept -> unit::ByteRange {
        const auto span = dataSpan();
        if (span.empty() || !range.isValid()) {
            return unit::ByteRange::empty();
        }
        const auto origin = _range.index().toSizeT();
        if (range.index().toSizeT() < origin) {
            return unit::ByteRange::empty();
        }
        const auto relativeStart = range.index().toSizeT() - origin;
        return unit::ByteRange{unit::ByteIndex::fromSizeT(relativeStart), range.length()}.clampedTo(
            unit::ByteLength::fromSizeT(span.size()));
    }

private:
    std::span<const char> _data;                      ///< The span to the character storage.
    unit::ByteRange _range{unit::ByteRange::empty()}; ///< The range the string covers.
};

}
