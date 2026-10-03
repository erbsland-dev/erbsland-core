// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16Encoding.hpp"
#include "U16StringDataView.hpp"

#include "../../../unit/U16Index.hpp"
#include "../../../unit/U16Range.hpp"
#include "../../CharSet.hpp"
#include "../../StringSide.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>

namespace erbsland::text::impl {

/// Trimming algorithms on UTF-16 string data.
/// @tested{U16StringTest}
class U16StringTrimTools final {
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

    /// Create trim tools for `data`.
    explicit constexpr U16StringTrimTools(const U16StringDataView &data) noexcept : _data{data} {}

public: // trim
    /// Return the absolute byte range with ASCII whitespace removed from the selected side.
    [[nodiscard]] auto trimmedRange(Side side) const noexcept -> unit::U16Range;
    /// Return the absolute byte range with characters from the set removed from the selected side.
    [[nodiscard]] auto trimmedRange(const CharSet &characters, Side side) const noexcept -> unit::U16Range;

private:
    /// Compute the trimmed byte range using the given decoded-character predicate.
    template <typename Predicate>
    [[nodiscard]] auto trimmedRangeFor(Side side, Predicate predicate) const noexcept -> unit::U16Range {
        const auto data = _data.dataSpan();
        if (data.empty()) {
            return unit::U16Range::empty();
        }

        auto begin = unit::U16Index::zero();
        auto end = unit::U16Index::fromSizeT(data.size());

        if (side == Side::Both || side == Side::Begin) {
            while (begin < end) {
                auto readPosition = begin;
                const auto character = utf16::decodeCharOrReplace(data, readPosition);
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
            utf16::fastRetreatChar(data, characterStart);
            auto readPosition = characterStart;
            const auto character = utf16::decodeCharOrReplace(data, readPosition);
            if (!predicate(character)) {
                break;
            }
            end = characterStart;
        }
        return toAbsoluteRange(begin, end);
    }
    /// Convert a relative byte range in the data span into an absolute range in the backing storage.
    [[nodiscard]] auto toAbsoluteRange(unit::U16Index begin, unit::U16Index end) const noexcept -> unit::U16Range;

private:
    U16StringDataView _data;
};

}
