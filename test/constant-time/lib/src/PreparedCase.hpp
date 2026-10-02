// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TestCase.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/unit/ByteLength.hpp>
#include <erbsland/unit/ItemIndex.hpp>
#include <erbsland/util/List.hpp>

#include <memory>

namespace app::constant_time {

/// Typed fixture ownership with preparation and measurement dispatch outside operation loops.
/// @tparam tFixture Concrete owned fixture.
/// @notest{Concrete cases and runner tests exercise the fixture lifecycle.}
template <typename tFixture>
class PreparedCase : public TestCase {
public: // implement TestCase
    [[nodiscard]] auto metadata() const noexcept -> const TestMetadata & override { return _metadata; }
    [[nodiscard]] auto fixtureBytes() const noexcept -> el::ByteLength override { return _bytes; }
    void clear() override { _fixtures.clear(); }
    void prepare(el::Random &random, bool population) override { _fixtures.append(createFixture(random, population)); }
    [[nodiscard]] auto execute(std::size_t index, uint64_t repetitions) -> uint64_t override {
        return _fixtures.getRefOrThrow(el::ItemIndex{index})->measure(repetitions);
    }

protected:
    /// Set metadata and the conservative per-fixture memory bound.
    /// @param metadata Stable scenario description and actual backend.
    /// @param bytes Conservative fixture estimate.
    void configure(TestMetadata metadata, el::ByteLength bytes) {
        _metadata = std::move(metadata);
        _bytes = bytes;
    }
    /// Construct and validate a single independently owned fixture.
    /// @param random Reproducible fixture randomness.
    /// @param population Assigned population.
    /// @return Prepared fixture.
    [[nodiscard]] virtual auto createFixture(el::Random &random, bool population) -> std::unique_ptr<tFixture> = 0;

private:
    TestMetadata _metadata{};                      ///< Stable configuration and actual backend.
    el::ByteLength _bytes{};                       ///< Conservative per-fixture memory estimate.
    el::List<std::shared_ptr<tFixture>> _fixtures; ///< Independently owned prepared samples.
};

}
