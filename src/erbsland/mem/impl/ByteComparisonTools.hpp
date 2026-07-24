// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteDataView.hpp"

#include "../../unit/ByteIndex.hpp"

#include <algorithm>
#include <compare>
#include <cstring>

namespace erbsland::mem::impl {

/// Implements comparisons and searches on a byte-data view.
/// @tested{ByteDataViewTest ByteBlockTest}
class ByteComparisonTools final {
public:
    /// Create tools for the given borrowed data view.
    explicit constexpr ByteComparisonTools(ByteDataView data) noexcept : _data{data} {}

public:
    /// Compare the selected bytes lexicographically.
    [[nodiscard]] auto compare(ByteDataView other) const noexcept -> std::strong_ordering {
        const auto left = _data.dataSpan();
        const auto right = other.dataSpan();
        const auto size = std::min(left.size(), right.size());
        if (size != 0U) {
            const auto result = std::memcmp(left.data(), right.data(), size * sizeof(Byte));
            if (result != 0) {
                return result < 0 ? std::strong_ordering::less : std::strong_ordering::greater;
            }
        }
        return left.size() <=> right.size();
    }
    /// Test if the selected bytes start with a sequence.
    [[nodiscard]] auto startsWith(ByteDataView prefix) const noexcept -> bool {
        const auto data = _data.dataSpan();
        const auto bytes = prefix.dataSpan();
        return data.size() >= bytes.size() && std::ranges::equal(bytes, data.first(bytes.size()));
    }
    /// Test if the selected bytes end with a sequence.
    [[nodiscard]] auto endsWith(ByteDataView suffix) const noexcept -> bool {
        const auto data = _data.dataSpan();
        const auto bytes = suffix.dataSpan();
        return data.size() >= bytes.size() && std::ranges::equal(bytes, data.last(bytes.size()));
    }
    /// Test if the selected bytes contain a sequence.
    [[nodiscard]] auto contains(ByteDataView sequence) const noexcept -> bool { return !find(sequence).isNoIndex(); }
    /// Find the first sequence occurrence.
    [[nodiscard]] auto find(ByteDataView sequence, unit::ByteIndex start = unit::ByteIndex::zero()) const noexcept
        -> unit::ByteIndex {
        if (!start.isValid()) {
            return unit::ByteIndex::noIndex();
        }
        const auto data = _data.dataSpan();
        const auto needle = sequence.dataSpan();
        if (start.toSizeT() > data.size()) {
            return unit::ByteIndex::noIndex();
        }
        if (needle.empty()) {
            return start;
        }
        if (needle.size() > data.size() - start.toSizeT()) {
            return unit::ByteIndex::noIndex();
        }
        const auto searchRange = data.subspan(start.toSizeT());
        const auto result = std::search(searchRange.begin(), searchRange.end(), needle.begin(), needle.end());
        if (result == searchRange.end()) {
            return unit::ByteIndex::noIndex();
        }
        return unit::ByteIndex::fromSizeT(start.toSizeT() + static_cast<std::size_t>(result - searchRange.begin()));
    }
    /// Find the final sequence occurrence.
    [[nodiscard]] auto findLast(ByteDataView sequence) const noexcept -> unit::ByteIndex {
        const auto data = _data.dataSpan();
        const auto needle = sequence.dataSpan();
        if (needle.empty()) {
            return unit::ByteIndex::end(_data.length());
        }
        if (needle.size() > data.size()) {
            return unit::ByteIndex::noIndex();
        }
        const auto result = std::find_end(data.begin(), data.end(), needle.begin(), needle.end());
        if (result == data.end()) {
            return unit::ByteIndex::noIndex();
        }
        return unit::ByteIndex::fromSizeT(static_cast<std::size_t>(result - data.begin()));
    }

private:
    ByteDataView _data; ///< The borrowed data operated on by these tools.
};

}
