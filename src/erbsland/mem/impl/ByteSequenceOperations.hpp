// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ByteSpan.hpp"

#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ByteRange.hpp"
#include "../../util/impl/LoopControl.hpp"
#include "../../util/LoopResult.hpp"
#include "../../util/LoopStatus.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>

namespace erbsland::mem::impl {

/// Create a read-only span for a clamped byte range.
/// @tested{ByteArrayTest ByteBufferTest ByteBlockTest}
[[nodiscard]] constexpr auto clampedSpan(const ConstByteSpan bytes, const unit::ByteRange range) noexcept
    -> ConstByteSpan {
    if (!range.isValid() || bytes.empty()) {
        return {};
    }
    const auto clamped = range.clampedTo(unit::ByteLength::fromSizeT(bytes.size()));
    if (clamped.isEmpty()) {
        return {};
    }
    return bytes.subspan(clamped.index().toSizeT(), clamped.length().toSizeT());
}

/// Create a writable span for a clamped byte range.
/// @tested{ByteArrayTest ByteBufferTest ByteBlockTest}
[[nodiscard]] constexpr auto clampedSpan(const ByteSpan bytes, const unit::ByteRange range) noexcept -> ByteSpan {
    if (!range.isValid() || bytes.empty()) {
        return {};
    }
    const auto clamped = range.clampedTo(unit::ByteLength::fromSizeT(bytes.size()));
    if (clamped.isEmpty()) {
        return {};
    }
    return bytes.subspan(clamped.index().toSizeT(), clamped.length().toSizeT());
}

/// Overwrite the largest possible part of a clamped target range.
/// @tested{ByteArrayTest ByteBufferTest ByteBlockTest}
[[nodiscard]] inline auto overwrite(
    const ByteSpan target, const unit::ByteRange targetRange, const ConstByteSpan source) noexcept -> unit::ByteLength {
    auto destination = clampedSpan(target, targetRange);
    const auto copyLength = std::min(destination.size(), source.size());
    if (copyLength == 0U) {
        return unit::ByteLength::zero();
    }
    std::memmove(destination.data(), source.data(), copyLength * sizeof(Byte));
    return unit::ByteLength::fromSizeT(copyLength);
}

/// Fill a clamped target range.
/// @tested{ByteArrayTest ByteBufferTest ByteBlockTest}
inline void fill(const ByteSpan target, const unit::ByteRange targetRange, const Byte value) noexcept {
    for (auto &byte : clampedSpan(target, targetRange)) {
        byte = value;
    }
}

/// XOR the largest possible part of a clamped target range.
/// Overlapping source storage is interpreted as if the source were copied before the operation.
/// @tested{ByteArrayTest ByteBufferTest ByteBlockTest}
[[nodiscard]] inline auto xorWith(
    const ByteSpan target, const unit::ByteRange targetRange, ConstByteSpan source) noexcept -> unit::ByteLength {
    auto destination = clampedSpan(target, targetRange);
    const auto operationLength = std::min(destination.size(), source.size());
    if (operationLength == 0U) {
        return unit::ByteLength::zero();
    }
    destination = destination.first(operationLength);
    source = source.first(operationLength);
    const auto destinationBegin = reinterpret_cast<std::uintptr_t>(destination.data());
    const auto destinationEnd = destinationBegin + destination.size_bytes();
    const auto sourceBegin = reinterpret_cast<std::uintptr_t>(source.data());
    const auto sourceEnd = sourceBegin + source.size_bytes();
    if (sourceBegin < destinationEnd && sourceEnd > destinationBegin && destinationBegin > sourceBegin) {
        for (auto index = operationLength; index > 0U; --index) {
            destination[index - 1U] ^= source[index - 1U];
        }
    } else {
        for (auto index = std::size_t{}; index < operationLength; ++index) {
            destination[index] ^= source[index];
        }
    }
    return unit::ByteLength::fromSizeT(operationLength);
}

/// Invoke a callback for every byte and its optional index.
/// @tested{ByteArrayTest ByteBufferTest ByteBlockTest}
template <typename Function>
auto forEachByte(const ConstByteSpan bytes, Function function) -> util::LoopResult {
    auto rawIndex = std::size_t{};
    for (const auto byte : bytes) {
        const auto status = [&]() -> util::LoopStatus {
            if constexpr (std::invocable<Function &, Byte, unit::ByteIndex>) {
                return util::impl::invokeLoopFunction(function, byte, unit::ByteIndex::fromSizeT(rawIndex));
            } else {
                return util::impl::invokeLoopFunction(function, byte);
            }
        }();
        if (status != util::LoopStatus::Continue) {
            return util::impl::loopStatusToResult(status);
        }
        ++rawIndex;
    }
    return util::LoopResult::Success;
}

}
