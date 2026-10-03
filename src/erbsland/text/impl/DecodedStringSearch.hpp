// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Char.hpp"
#include "../CharCompareFn.hpp"

#include "../../unit/ItemCount.hpp"

#include <cstddef>
#include <exception>
#include <optional>
#include <utility>
#include <vector>

namespace erbsland::text::impl {

/// A preprocessed decoded-character pattern for linear-time string searches.
/// @tested{U8StringReadToolsTest U16StringTest U32StringTest StringSeparatorSplitTest}
class DecodedStringSearch final {
private:
    /// One decoded pattern character and its longest matching prefix length.
    struct PatternElement {
        Char character;             ///< The decoded pattern character.
        std::size_t prefixLength{}; ///< Length of the longest matching proper prefix at this position.
    };

public:
    /// Decode and preprocess a search pattern.
    /// @tparam tIndex The index type used in native text indices.
    /// @tparam tReadNext The type for the decode function.
    /// @param maximumLength Upper bound for the number of decoded characters.
    /// @param start The native index at which the pattern starts.
    /// @param readNext A function that decodes one character and advances its native index, or returns end-of-data.
    /// @param compareFn An optional comparison function for decoded characters (defaults to equality).
    template <typename tIndex, typename tReadNext>
    DecodedStringSearch(
        const std::size_t maximumLength, tIndex start, tReadNext readNext, const CharCompareFn compareFn) :
        _compareFn{compareFn} {
        _pattern.reserve(maximumLength);
        while (true) {
            const auto character = readNext(start);
            if (character.isEndOfData()) {
                break;
            }
            auto prefixLength = _pattern.empty() ? 0U : _pattern.back().prefixLength;
            while (prefixLength > 0U && !charactersEqual(character, _pattern[prefixLength].character)) {
                prefixLength = _pattern[prefixLength - 1U].prefixLength;
            }
            if (!_pattern.empty() && charactersEqual(character, _pattern[prefixLength].character)) {
                ++prefixLength;
            }
            _pattern.push_back(PatternElement{character, prefixLength});
        }
    }

public:
    /// Test whether preprocessing is worthwhile for the given native pattern length.
    [[nodiscard]] static constexpr auto isUsefulFor(const std::size_t nativeLength) noexcept -> bool {
        return nativeLength >= 64U;
    }

    /// Find the first pattern occurrence in a decoded character stream.
    /// @param start The native stream index at which to start searching.
    /// @param readNext A function that decodes one character and advances its native index, or returns end-of-data.
    /// @return The native index of the first match, or the no-index value.
    template <typename tIndex, typename tReadNext>
    [[nodiscard]] auto find(const tIndex start, tReadNext readNext) const -> tIndex {
        if (_pattern.empty()) {
            return start;
        }

        auto position = start;
        auto processedLength = std::size_t{0};
        auto matchedLength = std::size_t{0};
        while (true) {
            const auto character = readNext(position);
            if (character.isEndOfData()) {
                return tIndex::noIndex();
            }
            ++processedLength;
            while (matchedLength > 0U && !charactersEqual(character, _pattern[matchedLength].character)) {
                matchedLength = _pattern[matchedLength - 1U].prefixLength;
            }
            if (charactersEqual(character, _pattern[matchedLength].character)) {
                ++matchedLength;
            }
            if (matchedLength == _pattern.size()) {
                auto result = start;
                const auto skippedLength = processedLength - matchedLength;
                for (auto i = std::size_t{0}; i < skippedLength; ++i) {
                    if (readNext(result).isEndOfData()) {
                        std::terminate();
                    }
                }
                return result;
            }
        }
    }

    /// Find both native boundaries without decoding the source prefix again.
    /// The reader may advance or retreat; returned boundaries follow that traversal order.
    /// @tparam tIndex The native index type.
    /// @tparam tReadNext The decoded reader function type.
    /// @param start The initial stream boundary.
    /// @param readNext Read one character and move its index, or return end-of-data.
    /// @return The entry and exit boundaries of the first match, or no value.
    template <typename tIndex, typename tReadNext>
    [[nodiscard]] auto findRange(const tIndex start, tReadNext readNext) const
        -> std::optional<std::pair<tIndex, tIndex>> {
        if (_pattern.empty()) {
            return std::pair{start, start};
        }
        auto boundaries = std::vector<tIndex>(_pattern.size());
        auto nextBoundary = std::size_t{};
        auto matchedLength = std::size_t{};
        auto position = start;
        while (true) {
            boundaries[nextBoundary] = position;
            const auto character = readNext(position);
            if (character.isEndOfData()) {
                return std::nullopt;
            }
            if (++nextBoundary == boundaries.size()) {
                nextBoundary = 0U;
            }
            while (matchedLength > 0U && !charactersEqual(character, _pattern[matchedLength].character)) {
                matchedLength = _pattern[matchedLength - 1U].prefixLength;
            }
            if (charactersEqual(character, _pattern[matchedLength].character)) {
                ++matchedLength;
            }
            if (matchedLength == _pattern.size()) {
                return std::pair{boundaries[nextBoundary], position};
            }
        }
    }

    /// Count non-overlapping pattern occurrences in a decoded character stream.
    /// @param start The native stream index at which to start searching.
    /// @param readNext A function that decodes one character and advances its native index, or returns end-of-data.
    /// @return The number of non-overlapping matches.
    template <typename tIndex, typename tReadNext>
    [[nodiscard]] auto count(const tIndex start, tReadNext readNext) const -> unit::ItemCount {
        if (_pattern.empty()) {
            return {};
        }

        auto result = unit::ItemCount{};
        auto position = start;
        auto matchedLength = std::size_t{0};
        while (true) {
            const auto character = readNext(position);
            if (character.isEndOfData()) {
                return result;
            }
            while (matchedLength > 0U && !charactersEqual(character, _pattern[matchedLength].character)) {
                matchedLength = _pattern[matchedLength - 1U].prefixLength;
            }
            if (charactersEqual(character, _pattern[matchedLength].character)) {
                ++matchedLength;
            }
            if (matchedLength == _pattern.size()) {
                ++result;
                matchedLength = 0U;
            }
        }
    }

private:
    /// Test if two characters compare as equal.
    [[nodiscard]] auto charactersEqual(const Char left, const Char right) const noexcept -> bool {
        return _compareFn == nullptr ? left == right : _compareFn(left, right) == std::strong_ordering::equal;
    }

private:
    std::vector<PatternElement> _pattern; ///< The decoded pattern and its prefix table.
    CharCompareFn _compareFn{};           ///< Optional decoded-character comparison function.
};

}
