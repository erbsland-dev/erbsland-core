// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringComparisonTools.hpp"

#include "U16Encoding.hpp"
#include "U16StringReadTools.hpp"

#include "../../impl/DecodedStringSearch.hpp"

namespace erbsland::text::impl {

using unit::U16Index;
using unit::U16Range;

auto U16StringComparisonTools::findFirstRange(
    const U16StringDataView &text, const CharCompareFn compareFn) const noexcept -> U16Range {
    return findTextRange(text, StringSide::Front, compareFn);
}

auto U16StringComparisonTools::findLastRange(
    const U16StringDataView &text, const CharCompareFn compareFn) const noexcept -> U16Range {
    return findTextRange(text, StringSide::Back, compareFn);
}

auto U16StringComparisonTools::findTextRange(
    const U16StringDataView &text, const StringSide side, const CharCompareFn compareFn) const noexcept -> U16Range {
    const auto data = _data.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return U16Range::noRange();
    }

    if (DecodedStringSearch::isUsefulFor(needle.size())) {
        if (side == StringSide::Front) {
            const auto readNeedle = [&needle](U16Index &position) noexcept -> Char {
                if (position.toSizeT() >= needle.size()) {
                    return Char::endOfData();
                }
                return utf16::decodeCharOrReplace(needle, position);
            };
            const auto readData = [&data](U16Index &position) noexcept -> Char {
                if (position.toSizeT() >= data.size()) {
                    return Char::endOfData();
                }
                return utf16::decodeCharOrReplace(data, position);
            };
            const auto search = DecodedStringSearch{needle.size(), U16Index::zero(), readNeedle, compareFn};
            if (const auto match = search.findRange(U16Index::zero(), readData)) {
                return U16Range{match->first, match->second};
            }
        } else {
            const auto readNeedle = [&needle](U16Index &position) noexcept -> Char {
                if (position.isZero()) {
                    return Char::endOfData();
                }
                utf16::fastRetreatChar(needle, position);
                auto readPosition = position;
                return utf16::decodeCharOrReplace(needle, readPosition);
            };
            const auto readData = [&data](U16Index &position) noexcept -> Char {
                if (position.isZero()) {
                    return Char::endOfData();
                }
                utf16::fastRetreatChar(data, position);
                auto readPosition = position;
                return utf16::decodeCharOrReplace(data, readPosition);
            };
            const auto search =
                DecodedStringSearch{needle.size(), U16Index::fromSizeT(needle.size()), readNeedle, compareFn};
            if (const auto match = search.findRange(U16Index::fromSizeT(data.size()), readData)) {
                return U16Range{match->second, match->first};
            }
        }
        return U16Range::noRange();
    }

    if (side == StringSide::Front) {
        auto position = U16Index::zero();
        while (position.toSizeT() < data.size()) {
            const auto end = endOfTextMatch(data, position, needle, compareFn);
            if (!end.isNoIndex()) {
                return U16Range{position, end};
            }
            utf16::fastAdvanceChar(data, position);
        }
    } else {
        auto position = U16Index::fromSizeT(data.size());
        while (!position.isZero()) {
            utf16::fastRetreatChar(data, position);
            const auto end = endOfTextMatch(data, position, needle, compareFn);
            if (!end.isNoIndex()) {
                return U16Range{position, end};
            }
        }
    }
    return U16Range::noRange();
}

auto U16StringComparisonTools::endOfTextMatch(
    const std::span<const char16_t> data,
    const U16Index start,
    const std::span<const char16_t> text,
    const CharCompareFn compareFn) noexcept -> U16Index {
    auto dataPosition = start;
    auto textPosition = U16Index::zero();
    while (textPosition.toSizeT() < text.size()) {
        if (dataPosition.toSizeT() >= data.size()) {
            return U16Index::noIndex();
        }
        const auto dataCharacter = utf16::decodeCharOrReplace(data, dataPosition);
        const auto textCharacter = utf16::decodeCharOrReplace(text, textPosition);
        if (!charactersEqual(dataCharacter, textCharacter, compareFn)) {
            return U16Index::noIndex();
        }
    }
    return dataPosition;
}

}
