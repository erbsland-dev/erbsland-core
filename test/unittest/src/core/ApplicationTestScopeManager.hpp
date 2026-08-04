// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationInstanceBuilderBase.hpp"

#include <erbsland/core/impl/ApplicationInstanceManager.hpp>

#include <atomic>
#include <memory>

/// Coordinates application-instance overrides for unit-test scopes.
/// @tested{ApplicationTestScopeTest}
class ApplicationTestScopeManager final : public erbsland::core::impl::ApplicationInstanceManager {
public:
    /// Return the shared manager used by all application test-scope specializations.
    [[nodiscard]] static auto instance() -> ApplicationTestScopeManager &;
    /// Install the shared manager as the application-instance override.
    static void install();

private:
    /// Create the shared test-scope manager.
    ApplicationTestScopeManager() = default;

    // defaults/deletions
    ~ApplicationTestScopeManager() override = default;
    ApplicationTestScopeManager(const ApplicationTestScopeManager &) = delete;
    ApplicationTestScopeManager(ApplicationTestScopeManager &&) = delete;
    auto operator=(const ApplicationTestScopeManager &) -> ApplicationTestScopeManager & = delete;
    auto operator=(ApplicationTestScopeManager &&) -> ApplicationTestScopeManager & = delete;

public:
    /// Open a test scope with the supplied application-instance builder.
    void openScope(std::unique_ptr<ApplicationInstanceBuilderBase> &&builder);
    /// Close the active application test scope.
    void closeScope();

public: // override ApplicationInstanceManager
    /// Register a user-owned application instance for the active scope.
    auto registerUserInstance(erbsland::core::Application *appInstance)
        -> erbsland::core::impl::ApplicationDataPtr override;
    /// Unregister the active user-owned application instance.
    auto unregisterInstance(const erbsland::core::Application *appInstance) -> bool override;
    /// Link application data to a new application instance.
    void linkWith(erbsland::core::Application &app) override;
    /// Return the application instance for the active scope.
    [[nodiscard]] auto application() -> erbsland::core::Application & override;
    /// Create application data using the active scope builder.
    [[nodiscard]] auto createApplicationData() -> erbsland::core::impl::ApplicationDataPtr override;

private:
    std::atomic<bool> _isScopeOpen{};                                            ///< Whether a test scope is active.
    std::unique_ptr<ApplicationInstanceBuilderBase> _applicationInstanceBuilder; ///< Builder.
};
