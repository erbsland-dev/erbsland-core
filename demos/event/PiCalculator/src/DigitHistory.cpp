// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DigitHistory.hpp"

#include <erbsland/AsciiCategory.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteRange.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/CpIndex.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/OverflowError.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/U8StringConstIterator.hpp>

#include <algorithm>
#include <limits>
#include <utility>

namespace demo {

using namespace el::text::literals;

void DigitHistory::append(el::String digits) {
    if (digits.isEmpty()) { // nothing to append.
        return;
    }
    // only accept ASCII digits.
    const static auto digitChars = el::CharSet::from(el::AsciiCategory::Digit);
    if (!digits.containsOnly(digitChars)) {
        throw el::ParameterFailure{"History accepts decimal digits only."_el, "digits"_el};
    }
    // test the unrealistic case of an overflow. ;-)
    const auto cpLength = digits.characterLength();
    if (_count.isInfinite() || cpLength.isInfinite() || cpLength > el::CpCount::maximum() - _count) {
        throw el::OverflowError{"Digit history is too large."_el};
    }
    _chunks.append(Chunk{el::CpIndex::end(_count), std::move(digits)});
    _count += cpLength;
}

auto DigitHistory::findChunk(const el::CpIndex begin) const -> el::ItemIndex {
    auto low = el::ItemIndex{};
    auto high = el::ItemIndex::end(_chunks.count());
    // Find the last starting offset not greater than begin, without scanning the full history on every redraw.
    while (low < high) {
        const auto middle = low + high.absoluteDistanceTo(low) / 2U;
        if (_chunks.getRefOrThrow(middle).begin <= begin) {
            low = middle + el::ItemCount::one();
        } else {
            high = middle;
        }
    }
    return low - el::ItemCount::one();
}

auto DigitHistory::range(const el::CpRange range) const -> el::String {
    // Clamp the given range to the entire history.
    auto clampedRange = range.clampedTo(_count);
    // return an empty string on invalid ranges.
    if (clampedRange.isEmpty()) {
        return {};
    }
    // Get the first chunk where our digit sequence starts.
    auto index = findChunk(range.beginIndex());
    // Start with an offset and remaining digits to be returned.
    auto remaining = clampedRange.length();
    auto offset = range.beginIndex();
    // Prepare a string editor and reserve enough memory for all digits. We know ASCII digits 1 CP = 1 byte.
    auto result = el::StringEditor{};
    result.reserve(el::ByteCount::fromInteger(remaining.toRawValue()));
    // Iterate over the chunks, until all remaining digits are retrieved.
    while (!remaining.isZero()) {
        const auto &chunk = _chunks.getRefOrThrow(index);
        // Extract only the portion of this chunk that covers the slice we need.
        const auto local = offset - chunk.begin.distanceFromZero();
        const auto digitSlice = chunk.digits.slice(el::CpRange{local, remaining});
        const auto digitSliceLength = digitSlice.characterLength();
        result.append(digitSlice);
        // advance to next chunk and update the offset and remaining digits count.
        index.increment();
        remaining -= digitSliceLength;
        offset += digitSliceLength;
    }
    return result;
}

}
