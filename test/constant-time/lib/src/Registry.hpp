// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Registration.hpp"

#include <erbsland/core/Definitions.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/util/List.hpp>

namespace app::constant_time {

/// Explicit factory registry with exact, deduplicated selection.
/// @tested{ConstantTimeRunnerTest}
class Registry final {
public:
    /// Register one uniquely named factory.
    void add(Registration registration);
    /// Construct and register one explicitly named scenario type.
    /// @tparam tCase Concrete scenario type.
    /// @param args Scenario configuration forwarded to its constructor.
    template <typename tCase, typename... tArgs>
    void add(tArgs... args) {
        add(Registration{std::make_shared<tCase>(args...)});
    }
    /// Get registered experiments in stable order.
    [[nodiscard]] auto entries() const noexcept -> const el::List<Registration> & { return _entries; }
    /// Select exact IDs or all tests, preserving registry order.
    [[nodiscard]] auto select(const el::StringList &ids, bool all) const -> el::List<Registration>;
    /// Create the complete built-in registry.
    [[nodiscard]] static auto builtIn() -> Registry;

private:
    /// Register byte comparison scenarios.
    void addComparisons();
    /// Register symmetric primitive scenarios.
    void addSymmetric();
    /// Register authentication and derivation scenarios.
    void addAuthentication();
    /// Register secret arithmetic and signing scenarios.
    void addSigning();
    /// Register equivalent-outcome validation scenarios.
    void addValidation();

private:
    el::List<Registration> _entries; ///< Explicit ordered registrations.
};
}
