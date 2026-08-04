// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteWriteTools.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace erbsland::mem::impl {

auto ByteWriteTools::span(const unit::ByteRange range) const noexcept -> ByteSpan {
    if (!range.isValid() || _data.empty()) {
        return {};
    }
    const auto clamped = range.clampedTo(unit::ByteLength::fromSizeT(_data.size()));
    if (clamped.isEmpty()) {
        return {};
    }
    return _data.subspan(clamped.index().toSizeT(), clamped.length().toSizeT());
}

void ByteWriteTools::fill(const unit::ByteRange range, const Byte value) noexcept {
    for (auto &byte : span(range)) {
        byte = value;
    }
}

void ByteWriteTools::overwrite(const unit::ByteRange range, const ByteDataView source) noexcept {
    auto destination = span(range);
    const auto bytes = source.dataSpan();
    const auto copyLength = std::min(destination.size(), bytes.size());
    if (copyLength == 0U) {
        return;
    }
    std::memmove(destination.data(), bytes.data(), copyLength * sizeof(Byte));
}

void ByteWriteTools::xorWith(const unit::ByteRange range, const ByteDataView source) noexcept {
    auto destination = span(range);
    auto bytes = source.dataSpan();
    const auto operationLength = std::min(destination.size(), bytes.size());
    if (operationLength == 0U) {
        return;
    }
    destination = destination.first(operationLength);
    bytes = bytes.first(operationLength);
    const auto destinationBegin = reinterpret_cast<std::uintptr_t>(destination.data());
    const auto destinationEnd = destinationBegin + destination.size_bytes();
    const auto sourceBegin = reinterpret_cast<std::uintptr_t>(bytes.data());
    const auto sourceEnd = sourceBegin + bytes.size_bytes();
    if (sourceBegin < destinationEnd && sourceEnd > destinationBegin && destinationBegin > sourceBegin) {
        for (auto index = operationLength; index > 0U; --index) {
            destination[index - 1U] ^= bytes[index - 1U];
        }
    } else {
        for (auto index = std::size_t{}; index < operationLength; ++index) {
            destination[index] ^= bytes[index];
        }
    }
}

}
