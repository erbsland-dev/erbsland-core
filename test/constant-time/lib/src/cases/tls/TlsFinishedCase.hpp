// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsFinishedFixture.hpp"

#include "../../PreparedCase.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/random/Random.hpp>

namespace app::constant_time {

/// Fixed-shape TlsFinished scenario and explicit variant factory.
/// @notest{Its fixtures validate the selected algorithm before timing.}
class TlsFinishedCase final : public PreparedCase<TlsFinishedFixture> {
public:
    /// Configure the scenario and identify its concrete backend.
    /// @param algorithm Fixed-output hash algorithm.
    explicit TlsFinishedCase(el::HashAlgorithm algorithm);

public: // implement TestCase
    [[nodiscard]] auto create(bool automatic) const -> std::unique_ptr<TestCase> override;

protected: // implement PreparedCase
    [[nodiscard]] auto createFixture(el::Random &random, bool population)
        -> std::unique_ptr<TlsFinishedFixture> override;

private:
    el::HashAlgorithm _algorithm; ///< Scenario configuration.
};

}
