// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/StringList.hpp>

#include <functional>

namespace tool::test::impl {

using namespace el::text::literals;

/// Shared callback observations for default-constructed data-source fixtures.
class DataSourceTestState {
public: // shared observations
    /// Get the state observed by default-constructed test fixtures.
    [[nodiscard]] static auto shared() -> DataSourceTestState &;

public: // callbacks
    /// Record a callback, run phase assertions, and inject requested failures.
    /// @param name The callback identity.
    void phase(const el::String &name);

public:
    el::StringList events;                               ///< Callback order observed by the fixture.
    el::StringList dependencies;                         ///< Dependencies declared by the fixture tool.
    el::String failure;                                  ///< Callback that must fail in a failure scenario.
    bool withRules = true;                               ///< Select configured or configuration-free sources.
    std::function<void(const el::String &)> phaseAction; ///< Assertions executed within callbacks.
};

}
