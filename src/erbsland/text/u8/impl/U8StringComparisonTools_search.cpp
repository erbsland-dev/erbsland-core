// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8StringComparisonTools.hpp"

#include "U8Encoding.hpp"
#include "U8StringReadTools.hpp"

#include "../../impl/DecodedStringSearch.hpp"

namespace erbsland::text::impl {

using unit::ByteIndex;
using unit::ByteRange;

auto U8StringComparisonTools::findFirstRange(const U8StringDataView &text, const CharCompareFn compareFn) const noexcept
    -> ByteRange {
    return findTextRange(text, StringSide::Front, compareFn);
}

auto U8StringComparisonTools::findLastRange(const U8StringDataView &text, const CharCompareFn compareFn) const noexcept
    -> ByteRange {
    return findTextRange(text, StringSide::Back, compareFn);
}

auto U8StringComparisonTools::findTextRange(
    const U8StringDataView &text, const StringSide side, const CharCompareFn compareFn) const noexcept -> ByteRange {
    const auto data = _data.dataSpan();
    const auto needle = text.dataSpan();
    if (data.empty() || needle.empty()) {
        return ByteRange::noRange();
    }
    if (compareFn == nullptr && needle.size() == 1U && static_cast<unsigned char>(needle.front()) <= 0x7FU) {
        const auto separators = CharSet{Char{static_cast<char32_t>(needle.front())}};
        if (side == StringSide::Front) {
            return U8StringReadTools{_data}.findFirstRange(separators);
        }
        return U8StringReadTools{_data}.findLastRange(separators);
    }

    if (DecodedStringSearch::isUsefulFor(needle.size())) {
        if (side == StringSide::Front) {
            const auto readNeedle = [&needle](ByteIndex &position) noexcept -> Char {
                if (position.toSizeT() >= needle.size()) {
                    return Char::endOfData();
                }
                return utf8::decodeCharOrReplace(needle, position);
            };
            const auto readData = [&data](ByteIndex &position) noexcept -> Char {
                if (position.toSizeT() >= data.size()) {
                    return Char::endOfData();
                }
                return utf8::decodeCharOrReplace(data, position);
            };
            const auto search = DecodedStringSearch{needle.size(), ByteIndex::zero(), readNeedle, compareFn};
            if (const auto match = search.findRange(ByteIndex::zero(), readData)) {
                return ByteRange{match->first, match->second};
            }
        } else {
            const auto readNeedle = [&needle](ByteIndex &position) noexcept -> Char {
                if (position.isZero()) {
                    return Char::endOfData();
                }
                utf8::fastRetreatChar(needle, position);
                auto readPosition = position;
                return utf8::decodeCharOrReplace(needle, readPosition);
            };
            const auto readData = [&data](ByteIndex &position) noexcept -> Char {
                if (position.isZero()) {
                    return Char::endOfData();
                }
                utf8::fastRetreatChar(data, position);
                auto readPosition = position;
                return utf8::decodeCharOrReplace(data, readPosition);
            };
            const auto search =
                DecodedStringSearch{needle.size(), ByteIndex::fromSizeT(needle.size()), readNeedle, compareFn};
            if (const auto match = search.findRange(ByteIndex::fromSizeT(data.size()), readData)) {
                return ByteRange{match->second, match->first};
            }
        }
        return ByteRange::noRange();
    }

    if (side == StringSide::Front) {
        auto position = ByteIndex::zero();
        while (position.toSizeT() < data.size()) {
            const auto end = endOfTextMatch(data, position, needle, compareFn);
            if (!end.isNoIndex()) {
                return ByteRange{position, end};
            }
            utf8::fastAdvanceChar(data, position);
        }
    } else {
        auto position = ByteIndex::fromSizeT(data.size());
        while (!position.isZero()) {
            utf8::fastRetreatChar(data, position);
            const auto end = endOfTextMatch(data, position, needle, compareFn);
            if (!end.isNoIndex()) {
                return ByteRange{position, end};
            }
        }
    }
    return ByteRange::noRange();
}

auto U8StringComparisonTools::endOfTextMatch(
    const std::span<const char> data,
    const ByteIndex start,
    const std::span<const char> text,
    const CharCompareFn compareFn) noexcept -> ByteIndex {
    auto dataPosition = start;
    auto textPosition = ByteIndex::zero();
    while (textPosition.toSizeT() < text.size()) {
        if (dataPosition.toSizeT() >= data.size()) {
            return ByteIndex::noIndex();
        }
        const auto dataCharacter = utf8::decodeCharOrReplace(data, dataPosition);
        const auto textCharacter = utf8::decodeCharOrReplace(text, textPosition);
        if (!charactersEqual(dataCharacter, textCharacter, compareFn)) {
            return ByteIndex::noIndex();
        }
    }
    return dataPosition;
}

}
