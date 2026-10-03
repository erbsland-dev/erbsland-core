// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Checksum.hpp"

#include <erbsland/Byte.hpp>
#include <erbsland/ByteBuffer.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/Definitions.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/Random.hpp>
#include <erbsland/String.hpp>

namespace app::constant_time {

using namespace el::text::literals;

/// Owned sample inputs, preparation validation and observable repeated execution.
/// @notest{Concrete fixtures validate their operations before measurement.}
class Fixture {
public:
    // defaults
    virtual ~Fixture() = default;

public:
    /// Measure a concrete operation with dispatch outside its repetition loop.
    /// @param repetitions Common calibrated operation count.
    /// @return Combined observable result.
    [[nodiscard]] virtual auto measure(uint64_t repetitions) -> uint64_t = 0;

protected:
    /// Build equal-length fixed or random input in independently owned storage.
    /// @param random Fixture generator.
    /// @param population False selects repeated 0x55; true selects random bytes.
    /// @param size Exact input size.
    /// @return Prepared input.
    [[nodiscard]] static auto input(el::Random &random, bool population, el::ByteCount size) -> el::ByteBuffer {
        auto bytes = random.buildByteBuffer(size);
        const auto mask = el::Byte{population ? uint8_t{0xffU} : uint8_t{0U}};
        // Generate and visit the same number of bytes for both populations, outside measurement.
        for (auto index = el::ByteIndex::zero(); index.isWithin(size); index.uncheckedIncrement()) {
            const auto position = index;
            bytes.set(position, (bytes.get(position) & mask) | (el::Byte{0x55U} & ~mask));
        }
        return bytes;
    }
    /// Calculate an observable checksum without content-dependent branches.
    /// @param bytes Operation output.
    /// @return Combined output bytes.
    [[nodiscard]] static auto checksum(el::ConstByteSpan bytes) noexcept -> uint64_t {
        auto result = uint64_t{};
        for (const auto byte : bytes) {
            result = result * 33U + byte.toUInt8();
        }
        return result;
    }
    /// Verify a prepared fixture before collecting measurements.
    /// @param condition Required invariant.
    static void require(bool condition) {
        if (!condition) {
            throw el::LogicFailure{"Constant-time fixture validation failed."_el};
        }
    }

    /// Repeat one statically typed operation while keeping its result observable.
    /// @param repetitions Common operation count.
    /// @param operation Concrete operation, never type-erased.
    /// @return Combined observable output.
    template <typename tOperation>
    [[nodiscard]] auto repeat(uint64_t repetitions, tOperation operation) -> uint64_t {
        auto checksum = uint64_t{};
        for (auto index = uint64_t{}; index < repetitions; ++index) {
            const auto value = operation();
            Checksum::instance().retain(value);
            checksum += value;
        }
        return checksum;
    }
};

}
