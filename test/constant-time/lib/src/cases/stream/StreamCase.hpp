// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StreamFixture.hpp"

#include "../../PreparedCase.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>

namespace app::constant_time {

/// Fixed-shape Stream scenario and explicit variant factory.
/// @notest{Its fixtures validate the selected algorithm before timing.}
class StreamCase final : public PreparedCase<StreamFixture> {
public:
    /// Configure the scenario and identify its concrete backend.
    /// @param size Public input length in bytes.
    /// @param operation One fixed operation selected before repetitions.
    /// @param automatic Allow factory selection of an accelerated backend.
    explicit StreamCase(std::size_t size, StreamOperation operation, bool automatic);

public: // implement TestCase
    [[nodiscard]] auto create(bool automatic) const -> std::unique_ptr<TestCase> override;

protected: // implement PreparedCase
    [[nodiscard]] auto createFixture(el::Random &random, bool population) -> std::unique_ptr<StreamFixture> override;

private:
    /// Get the stable configured operation name.
    [[nodiscard]] auto operationName() const -> el::String;

private:
    std::size_t _size;          ///< Scenario configuration.
    StreamOperation _operation; ///< Scenario configuration.
    bool _automatic;            ///< Scenario configuration.
};

}
