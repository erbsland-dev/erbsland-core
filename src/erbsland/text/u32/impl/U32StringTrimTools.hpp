// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32Encoding.hpp"
#include "U32StringDataView.hpp"

#include "../../../unit/CpIndex.hpp"
#include "../../../unit/CpRange.hpp"
#include "../../CharSet.hpp"
#include "../../StringSide.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>

namespace erbsland::text::impl {

/// Trimming algorithms on UTF-32 string data.
/// @tested{U32StringTest}
class U32StringTrimTools final {
public:
    /// The string side from where matching characters are removed.
    enum class Side : uint8_t {
        Both,
        Begin,
        End,
    };

public:
    /// Convert the public side selector into the trimming direction.
    [[nodiscard]] static auto sideFrom(const std::optional<StringSide> side) noexcept -> Side {
        if (!side.has_value()) {
            return Side::Both;
        }
        return *side == StringSide::Front ? Side::Begin : Side::End;
    }

    explicit constexpr U32StringTrimTools(const U32StringDataView &data) noexcept : _data{data} {}

public: // trim
    /// Return the absolute byte range with ASCII whitespace removed from the selected side.
    [[nodiscard]] auto trimmedRange(Side side) const noexcept -> unit::CpRange;
    /// Return the absolute byte range with characters from the set removed from the selected side.
    [[nodiscard]] auto trimmedRange(const CharSet &characters, Side side) const noexcept -> unit::CpRange;

private:
    /// Compute the trimmed byte range using the given decoded-character predicate.
    template <typename Predicate>
    [[nodiscard]] auto trimmedRangeFor(Side side, Predicate predicate) const noexcept -> unit::CpRange {
        const auto data = _data.dataSpan();
        if (data.empty()) {
            return unit::CpRange::empty();
        }

        auto begin = unit::CpIndex::zero();
        auto end = unit::CpIndex::fromSizeT(data.size());

        if (side == Side::Both || side == Side::Begin) {
            while (begin < end) {
                auto readPosition = begin;
                const auto character = utf32::decodeCharOrReplace(data, readPosition);
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
            utf32::fastRetreatChar(data, characterStart);
            auto readPosition = characterStart;
            const auto character = utf32::decodeCharOrReplace(data, readPosition);
            if (!predicate(character)) {
                break;
            }
            end = characterStart;
        }
        return toAbsoluteRange(begin, end);
    }
    /// Convert a relative byte range in the data span into an absolute range in the backing storage.
    [[nodiscard]] auto toAbsoluteRange(unit::CpIndex begin, unit::CpIndex end) const noexcept -> unit::CpRange;

private:
    U32StringDataView _data;
};

}
