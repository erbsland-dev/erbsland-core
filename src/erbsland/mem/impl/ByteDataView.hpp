// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteDataView_fwd.hpp"

#include "../ByteSpan.hpp"

#include "../../unit/ByteLength.hpp"
#include "../../unit/ByteRange.hpp"

#include <algorithm>

namespace erbsland::mem::impl {

/// A borrowed view of a selected range in byte storage.
/// The backing span is never owned and must remain valid for the complete lifetime of this view.
/// @tested{ByteDataViewTest}
class ByteDataView final {
public:
    /// Create an empty view.
    constexpr ByteDataView() noexcept = default;
    /// Create a view to a selected range in byte storage.
    constexpr ByteDataView(ConstByteSpan data, unit::ByteRange range) noexcept : _data{data}, _range{range} {}
    /// Create a view selecting a complete byte span.
    constexpr explicit ByteDataView(ConstByteSpan data) noexcept :
        _data{data}, _range{unit::ByteRange::fromSizeT(data.size())} {}

public: // accessors
    /// Access the complete borrowed backing span.
    [[nodiscard]] constexpr auto data() const noexcept -> ConstByteSpan { return _data; }
    /// Access the selected range in the backing span.
    [[nodiscard]] constexpr auto range() const noexcept -> unit::ByteRange { return _range; }
    /// Get the bounded selected byte length.
    [[nodiscard]] constexpr auto length() const noexcept -> unit::ByteLength {
        return unit::ByteLength::fromSizeT(dataSpan().size());
    }
    /// Create a bounded span for the selected range.
    [[nodiscard]] constexpr auto dataSpan() const noexcept -> ConstByteSpan {
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
    /// Convert a relative range into a bounded absolute range in the backing span.
    [[nodiscard]] constexpr auto absoluteRange(unit::ByteRange relativeRange) const noexcept -> unit::ByteRange {
        if (!_range.isValid() || !relativeRange.isValid()) {
            return unit::ByteRange::empty();
        }
        const auto clamped = relativeRange.clampedTo(length());
        if (clamped.isEmpty()) {
            return unit::ByteRange::empty();
        }
        return clamped.withOrigin(_range.index());
    }

private:
    ConstByteSpan _data;                              ///< The complete borrowed backing storage.
    unit::ByteRange _range{unit::ByteRange::empty()}; ///< The selected range in `_data`.
};

}
