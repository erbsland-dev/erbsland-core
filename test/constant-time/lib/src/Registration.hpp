// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TestCase.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/text/String.hpp>

#include <memory>

namespace app::constant_time {

using namespace el::text::literals;

/// Explicit named scenario whose object supplies fresh measurement variants.
/// @tested{ConstantTimeRunnerTest}
class Registration final {
public:
    /// Register a concrete scenario object.
    /// @param prototype Immutable scenario configuration and variant factory.
    explicit Registration(std::shared_ptr<const TestCase> prototype) : _prototype{std::move(prototype)} {
        if (!_prototype) {
            throw el::LogicError{"A registered scenario requires a factory object."_el};
        }
        _id = _prototype->metadata().id;
    }
    /// Construct a description-only registration for selection checks.
    /// @param id Stable identifier.
    /// @param prototype Optional concrete factory.
    Registration(el::String id, std::shared_ptr<const TestCase> prototype = {}) :
        _id{std::move(id)}, _prototype{std::move(prototype)} {}
    /// Get the exact selection ID.
    [[nodiscard]] auto id() const noexcept -> const el::String & { return _id; }
    /// Construct a fresh backend variant.
    /// @param automatic Whether factory selection may use acceleration.
    /// @return Independently owned measurement scenario.
    /// @throws el::LogicError If no concrete factory is registered.
    [[nodiscard]] auto create(bool automatic) const -> std::unique_ptr<TestCase> {
        if (!_prototype) {
            throw el::LogicError{"No measurement factory registered."_el};
        }
        return _prototype->create(automatic);
    }

private:
    el::String _id;                             ///< Stable exact selector.
    std::shared_ptr<const TestCase> _prototype; ///< Immutable scenario-owned factory.
};

}
