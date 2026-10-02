// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TestMetadata.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ByteLength.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace app::constant_time {

using namespace el::text::literals;

/// Fixture preparation and one concrete measurement operation.
/// @notest{Abstract contract; concrete cases are validated before measurement.}
class TestCase {
public:
    /// Construct a fresh configured backend variant outside measurement.
    /// @param automatic Whether factory selection may use acceleration.
    /// @return Independently owned experiment.
    [[nodiscard]] virtual auto create([[maybe_unused]] bool automatic) const -> std::unique_ptr<TestCase> {
        throw el::LogicError{"This test double has no variant factory."_el};
    }
    // defaults
    virtual ~TestCase() = default;
    /// Get the experiment description.
    [[nodiscard]] virtual auto metadata() const noexcept -> const TestMetadata & = 0;
    /// Get a conservative memory estimate for one fixture.
    [[nodiscard]] virtual auto fixtureBytes() const noexcept -> el::ByteLength = 0;
    /// Clear the previous batch outside measured execution.
    virtual void clear() = 0;
    /// Prepare and validate one sample outside measured execution.
    /// @param random Reproducible fixture randomness.
    /// @param population The assigned input class.
    virtual void prepare(el::Random &random, bool population) = 0;
    /// Execute repetitions of a prepared sample without per-operation type-erased dispatch.
    /// @param index The prepared sample index.
    /// @param repetitions Fixed operation count shared by both populations.
    /// @return Observable checksum.
    [[nodiscard]] virtual auto execute(std::size_t index, uint64_t repetitions) -> uint64_t = 0;
};
}
