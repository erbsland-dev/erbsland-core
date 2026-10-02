// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ComparisonContainer.hpp"
#include "ComparisonMode.hpp"

#include "../../Fixture.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteArray.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

/// Prepared and validated Comparison sample with owned sensitive inputs.
/// @notest{Exercised by on-demand fixture validation and timing smoke runs.}
class ComparisonFixture final : public Fixture {
public:
    /// Prepare a sample outside measured execution.
    /// @param random Reproducible fixture generator.
    /// @param population Assigned population.
    /// @param size Public input length in bytes.
    /// @param mode Equal content or targeted mismatch populations.
    /// @param container Core byte container path under measurement.
    ComparisonFixture(
        el::Random &random, bool population, std::size_t size, ComparisonMode mode, ComparisonContainer container);

public: // implement Fixture
    [[nodiscard]] auto measure(uint64_t repetitions) -> uint64_t override;

private:
    /// Execute the statically selected operation once.
    template <ComparisonContainer tOperation>
    [[nodiscard]] auto sample() -> uint64_t;

private:
    ComparisonContainer _container;  ///< Scenario configuration.
    el::ByteBuffer _left{};          ///< Secret comparison bytes.
    el::ByteBuffer _right{};         ///< Same length expected bytes.
    el::ByteBlock _leftBlock{};      ///< Immutable comparison path.
    el::ByteBlock _rightBlock{};     ///< Immutable expected path.
    el::ByteArray<32> _leftArray{};  ///< Fixed-size path.
    el::ByteArray<32> _rightArray{}; ///< Fixed-size expected bytes.
};

}
