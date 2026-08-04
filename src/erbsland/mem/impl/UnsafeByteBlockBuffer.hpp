// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BestGrowth.hpp"
#include "ByteBlockData.hpp"
#include "UnsafeByteBlockBuffer_fwd.hpp"

#include "../ByteBlockEditor.hpp"
#include "../ByteSpan.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteLength.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <exception>
#include <span>
#include <utility>

namespace erbsland::mem::impl {

using namespace text::literals;

/// Owns uncommitted byte block storage for low-level native APIs.
/// Storage is uninitialized; every byte included in `take()` must be written first.
/// @warning Do not use this class in user code!
/// @tested{ByteStreamTest}
class UnsafeByteBlockBuffer {
public:
    /// Create an empty buffer.
    UnsafeByteBlockBuffer() = default;
    /// Create a buffer with the given usable capacity.
    /// @throws err::OutOfRangeError If `capacity` exceeds the supported storage limit.
    explicit UnsafeByteBlockBuffer(unit::ByteLength capacity, bool sensitive = false) : _sensitive{sensitive} {
        const auto capacityValue = capacity.toSizeTOrThrow();
        if (capacityValue != 0U) {
            if (!ByteBlockData::canAllocateWithCapacity(capacityValue)) {
                throw err::OutOfRangeError{"Byte block capacity exceeds the supported limit"_el};
            }
            _data = ByteBlockDataPtr{ByteBlockData::create(
                0U,
                static_cast<ByteBlockData::SizeType>(capacityValue),
                _sensitive ? ByteBlockData::cSensitiveFlag : std::uint8_t{})};
        }
    }

    // defaults
    ~UnsafeByteBlockBuffer() = default;
    UnsafeByteBlockBuffer(const UnsafeByteBlockBuffer &) = delete;
    UnsafeByteBlockBuffer(UnsafeByteBlockBuffer &&) noexcept = default;
    auto operator=(const UnsafeByteBlockBuffer &) -> UnsafeByteBlockBuffer & = delete;
    auto operator=(UnsafeByteBlockBuffer &&) noexcept -> UnsafeByteBlockBuffer & = default;

public:
    /// Access the writable buffer span.
    [[nodiscard]] auto data() noexcept -> ByteSpan {
        if (_data.isNull()) {
            return {};
        }
        return ByteSpan{_data.get()->data(), _data.get()->capacity()};
    }
    /// Access the remaining data after a given index or length.
    [[nodiscard]] auto remainingData(const unit::ByteLength initialLength) noexcept -> ByteSpan {
        if (initialLength > capacity()) {
            return {};
        }
        return data().subspan(initialLength.toSizeT());
    }
    /// Access the usable buffer capacity.
    [[nodiscard]] auto capacity() const noexcept -> unit::ByteLength {
        return _data.isNull() ? unit::ByteLength::zero() : unit::ByteLength::fromSizeT(_data.constGet()->capacity());
    }
    /// Grow the buffer while preserving a written prefix.
    /// @param minimumCapacity The minimum capacity after growth.
    /// @param preservedLength The initialized prefix that must survive relocation.
    /// @param maximumCapacity The hard upper bound for growth.
    /// @return The complete writable buffer after any relocation.
    /// @throws err::OutOfRangeError If the requested capacity exceeds the supported storage limit.
    auto grow(
        unit::ByteLength minimumCapacity,
        unit::ByteLength preservedLength,
        unit::ByteLength maximumCapacity = unit::ByteLength::infinite()) -> ByteSpan {
        if (!minimumCapacity.isFinite() || !preservedLength.isFinite() || preservedLength > capacity() ||
            preservedLength > minimumCapacity || (maximumCapacity.isFinite() && minimumCapacity > maximumCapacity)) {
            std::terminate();
        }
        if (minimumCapacity <= capacity()) {
            return data();
        }
        const auto minimum = minimumCapacity.toSizeT();
        auto target =
            BestGrowth{capacity().toSizeT(), minimum}.bestGrowth<ByteBlockData>(BestGrowthStrategy::Geometric);
        if (maximumCapacity.isFinite()) {
            target = std::min(target, maximumCapacity.toSizeT());
        }
        if (!ByteBlockData::canAllocateWithCapacity(target)) {
            throw err::OutOfRangeError{"Byte block capacity exceeds the supported limit"_el};
        }
        const auto flags =
            _data.isNull() ? (_sensitive ? ByteBlockData::cSensitiveFlag : std::uint8_t{}) : _data.constGet()->flags();
        auto replacement =
            ByteBlockDataPtr{ByteBlockData::create(0U, static_cast<ByteBlockData::SizeType>(target), flags)};
        if (!preservedLength.isZero()) {
            std::memcpy(replacement.get()->data(), _data.constGet()->data(), preservedLength.toSizeT() * sizeof(Byte));
        }
        _data = std::move(replacement);
        return data();
    }
    /// Release all buffer storage.
    void reset() noexcept { _data.reset(); }
    /// Create a byte block from the buffer and release the buffer.
    [[nodiscard]] auto take(unit::ByteLength length = unit::ByteLength::infinite()) -> ByteBlockEditor {
        const auto finalLength = checkedFinalLength(length);
        if (finalLength.isZero()) {
            _data.reset();
            return {};
        }
        _data.get()->setSize(static_cast<ByteBlockData::SizeType>(finalLength.toSizeT()));
        return ByteBlockEditor{std::move(_data)};
    }

public:
    /// Test if a capacity would exceed the maximum capacity.
    [[nodiscard]] static auto wouldExceedCapacity(const std::size_t capacityValue) noexcept -> bool {
        return !ByteBlockData::canAllocateWithCapacity(capacityValue);
    }

private:
    /// Validate the final byte length for take().
    [[nodiscard]] auto checkedFinalLength(unit::ByteLength length) const -> unit::ByteLength {
        if (length.isInfinite()) {
            return capacity();
        }
        if (length > capacity()) {
            std::terminate();
        }
        return length;
    }

private:
    ByteBlockDataPtr _data; ///< The uncommitted low-level byte storage.
    bool _sensitive{};      ///< Sensitivity mode retained while storage is empty.
};

}
