// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringComparisonTools.hpp"

#include "U32Encoding.hpp"
#include "U32StringReadTools.hpp"

#include "../../impl/DecodedStringSearch.hpp"

namespace erbsland::text::impl {

using unit::CpIndex;
using unit::CpRange;

auto U32StringComparisonTools::findFirstRange(
    const U32StringDataView &text, const CharCompareFn compareFn) const noexcept -> CpRange {
    return findTextRange(text, StringSide::Front, compareFn);
}

auto U32StringComparisonTools::findLastRange(
    const U32StringDataView &text, const CharCompareFn compareFn) const noexcept -> CpRange {
    return findTextRange(text, StringSide::Back, compareFn);
}

auto U32StringComparisonTools::findTextRange(
    const U32StringDataView &text, const StringSide side, const CharCompareFn compareFn) const noexcept -> CpRange {
    const auto data = _data.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return CpRange::noRange();
    }

    if (DecodedStringSearch::isUsefulFor(needle.size())) {
        if (side == StringSide::Front) {
            const auto readNeedle = [&needle](CpIndex &position) noexcept -> Char {
                if (position.toSizeT() >= needle.size()) {
                    return Char::endOfData();
                }
                return utf32::decodeCharOrReplace(needle, position);
            };
            const auto readData = [&data](CpIndex &position) noexcept -> Char {
                if (position.toSizeT() >= data.size()) {
                    return Char::endOfData();
                }
                return utf32::decodeCharOrReplace(data, position);
            };
            const auto search = DecodedStringSearch{needle.size(), CpIndex::zero(), readNeedle, compareFn};
            if (const auto match = search.findRange(CpIndex::zero(), readData)) {
                return CpRange{match->first, match->second};
            }
        } else {
            const auto readNeedle = [&needle](CpIndex &position) noexcept -> Char {
                if (position.isZero()) {
                    return Char::endOfData();
                }
                utf32::fastRetreatChar(needle, position);
                auto readPosition = position;
                return utf32::decodeCharOrReplace(needle, readPosition);
            };
            const auto readData = [&data](CpIndex &position) noexcept -> Char {
                if (position.isZero()) {
                    return Char::endOfData();
                }
                utf32::fastRetreatChar(data, position);
                auto readPosition = position;
                return utf32::decodeCharOrReplace(data, readPosition);
            };
            const auto search =
                DecodedStringSearch{needle.size(), CpIndex::fromSizeT(needle.size()), readNeedle, compareFn};
            if (const auto match = search.findRange(CpIndex::fromSizeT(data.size()), readData)) {
                return CpRange{match->second, match->first};
            }
        }
        return CpRange::noRange();
    }

    if (side == StringSide::Front) {
        auto position = CpIndex::zero();
        while (position.toSizeT() < data.size()) {
            const auto end = endOfTextMatch(data, position, needle, compareFn);
            if (!end.isNoIndex()) {
                return CpRange{position, end};
            }
            utf32::fastAdvanceChar(data, position);
        }
    } else {
        auto position = CpIndex::fromSizeT(data.size());
        while (!position.isZero()) {
            utf32::fastRetreatChar(data, position);
            const auto end = endOfTextMatch(data, position, needle, compareFn);
            if (!end.isNoIndex()) {
                return CpRange{position, end};
            }
        }
    }
    return CpRange::noRange();
}

auto U32StringComparisonTools::endOfTextMatch(
    const std::span<const char32_t> data,
    const CpIndex start,
    const std::span<const char32_t> text,
    const CharCompareFn compareFn) noexcept -> CpIndex {
    auto dataPosition = start;
    auto textPosition = CpIndex::zero();
    while (textPosition.toSizeT() < text.size()) {
        if (dataPosition.toSizeT() >= data.size()) {
            return CpIndex::noIndex();
        }
        const auto dataCharacter = utf32::decodeCharOrReplace(data, dataPosition);
        const auto textCharacter = utf32::decodeCharOrReplace(text, textPosition);
        if (!charactersEqual(dataCharacter, textCharacter, compareFn)) {
            return CpIndex::noIndex();
        }
    }
    return dataPosition;
}

}
