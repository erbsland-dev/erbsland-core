// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8Encoding.hpp"
#include "U8StringDataView.hpp"

#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ByteRange.hpp"
#include "../../CharSet.hpp"

#include <algorithm>
#include <cstdint>
#include <span>

namespace erbsland::text::impl {

/// Trimming algorithms on UTF-8 string data.
/// @tested{U8StringTrimTest}
class U8StringTrimTools final {
public:
    /// The string side from where matching characters are removed.
    enum class Side : uint8_t {
        Both,
        Begin,
        End,
    };

public:
    explicit constexpr U8StringTrimTools(const U8StringDataView &data) noexcept : _data{data} {}

public: // trim
    /// Return the absolute byte range with ASCII whitespace removed from the selected side.
    [[nodiscard]] auto trimmedRange(Side side) const noexcept -> unit::ByteRange;
    /// Return the absolute byte range with characters from the set removed from the selected side.
    [[nodiscard]] auto trimmedRange(const CharSet &characters, Side side) const noexcept -> unit::ByteRange;

private:
    /// Compute the trimmed byte range using the given decoded-character predicate.
    template <typename Predicate>
    [[nodiscard]] auto trimmedRangeFor(Side side, Predicate predicate) const noexcept -> unit::ByteRange {
        const auto data = _data.dataSpan();
        if (data.empty()) {
            return unit::ByteRange::empty();
        }

        auto begin = unit::ByteIndex::zero();
        auto end = unit::ByteIndex::fromSizeT(data.size());

        if (side == Side::Both || side == Side::Begin) {
            while (begin < end) {
                auto readPosition = begin;
                const auto character = utf8::decodeCharOrReplace(data, readPosition);
                if (!predicate(character)) {
                    break;
                }
                begin = readPosition;
            }
        }
        if (begin == end || side == Side::Begin) {
            return toAbsoluteRange(begin, end);
        }

        while (end > begin) {
            auto characterStart = end;
            utf8::fastRetreatChar(data, characterStart);
            auto readPosition = characterStart;
            const auto character = utf8::decodeCharOrReplace(data, readPosition);
            if (!predicate(character)) {
                break;
            }
            end = characterStart;
        }
        return toAbsoluteRange(begin, end);
    }
    /// Convert a relative byte range in the data span into an absolute range in the backing storage.
    [[nodiscard]] auto toAbsoluteRange(unit::ByteIndex begin, unit::ByteIndex end) const noexcept -> unit::ByteRange;

private:
    U8StringDataView _data;
};

}
