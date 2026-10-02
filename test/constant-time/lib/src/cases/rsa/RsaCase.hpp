// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RsaFixture.hpp"

#include "../../PreparedCase.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>

namespace app::constant_time {

/// Fixed-shape Rsa scenario and explicit variant factory.
/// @notest{Its fixtures validate the selected algorithm before timing.}
class RsaCase final : public PreparedCase<RsaFixture> {
public:
    /// Configure the scenario and identify its concrete backend.
    /// @param operation One fixed operation selected before repetitions.
    explicit RsaCase(RsaOperation operation);

public: // implement TestCase
    [[nodiscard]] auto create(bool automatic) const -> std::unique_ptr<TestCase> override;

protected: // implement PreparedCase
    [[nodiscard]] auto createFixture(el::Random &random, bool population) -> std::unique_ptr<RsaFixture> override;

private:
    /// Get the stable configured operation identifier.
    [[nodiscard]] auto identifier() const -> el::String;

private:
    RsaOperation _operation;              ///< Scenario configuration.
    std::shared_ptr<RsaContext> _context; ///< Lazily parsed embedded key, shared by this batch.
};

}
