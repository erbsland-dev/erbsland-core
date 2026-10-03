// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ComparisonFixture.hpp"

#include "../../PreparedCase.hpp"

#include <erbsland/ByteCount.hpp>
#include <erbsland/Definitions.hpp>
#include <erbsland/Random.hpp>
#include <erbsland/String.hpp>

namespace app::constant_time {

/// Fixed-shape Comparison scenario and explicit variant factory.
/// @notest{Its fixtures validate the selected algorithm before timing.}
class ComparisonCase final : public PreparedCase<ComparisonFixture> {
public:
    /// Configure the scenario and identify its concrete backend.
    /// @param size Public input length in bytes.
    /// @param mode Equal content or targeted mismatch populations.
    /// @param container Core byte container path under measurement.
    explicit ComparisonCase(el::ByteCount size, ComparisonMode mode, ComparisonContainer container);

public: // implement TestCase
    [[nodiscard]] auto create(bool automatic) const -> std::unique_ptr<TestCase> override;

protected: // implement PreparedCase
    [[nodiscard]] auto createFixture(el::Random &random, bool population)
        -> std::unique_ptr<ComparisonFixture> override;

private:
    /// Get the configured comparison container label.
    [[nodiscard]] auto containerName() const -> el::String;

private:
    /// Get the configured comparison mode label.
    [[nodiscard]] auto modeName() const -> el::String;

private:
    /// Get the configured comparison population label.
    [[nodiscard]] auto populationName() const -> el::String;

private:
    el::ByteCount _size;            ///< Scenario configuration.
    ComparisonMode _mode;           ///< Scenario configuration.
    ComparisonContainer _container; ///< Scenario configuration.
};

}
