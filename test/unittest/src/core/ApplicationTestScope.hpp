// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationInstanceBuilder.hpp"
#include "ApplicationTestScopeManager.hpp"
#include "IllegalApplicationInstanceAccess.hpp"

#include <memory>
#include <type_traits>

/// Opens a scoped application-instance override for a test.
/// Usage
/// <code>
/// void testExample() {
///     ApplicationTestScope<el::Application> appTestScope;
///     // tests
///     appTestScope.app() // access the locally created application instance
/// }
/// </code>
template <
    typename tApplication = erbsland::core::Application,
    typename tApplicationData = erbsland::core::impl::ApplicationDataImpl>
    requires std::is_base_of_v<erbsland::core::Application, tApplication> &&
    std::is_base_of_v<erbsland::core::impl::ApplicationData, tApplicationData>
class ApplicationTestScope {
public:
    /// Select construction without a local application instance.
    struct NoLocalAppInstance {};

public:
    /// Install the application-instance override before running unit tests.
    static void installApplicationInstanceManagerOverride() { ApplicationTestScopeManager::install(); }

public:
    /// Create a new scope with a locally created application instance
    ApplicationTestScope() : _scopeManager{ApplicationTestScopeManager::instance()} {
        _scopeManager.openScope(std::make_unique<ApplicationInstanceBuilder<tApplication, tApplicationData>>());
        try {
            _application = std::make_unique<tApplication>();
        } catch (...) {
            _scopeManager.closeScope();
            throw;
        }
    }
    /// Create a new scope with a locally created application instance
    ApplicationTestScope(int argc, char *argv[]) : _scopeManager{ApplicationTestScopeManager::instance()} {
        _scopeManager.openScope(std::make_unique<ApplicationInstanceBuilder<tApplication, tApplicationData>>());
        try {
            _application = std::make_unique<tApplication>(argc, argv);
        } catch (...) {
            _scopeManager.closeScope();
            throw;
        }
    }
    /// Create a new scope with a locally created application instance
    ApplicationTestScope(int argc, wchar_t *argv[]) : _scopeManager{ApplicationTestScopeManager::instance()} {
        _scopeManager.openScope(std::make_unique<ApplicationInstanceBuilder<tApplication, tApplicationData>>());
        try {
            _application = std::make_unique<tApplication>(argc, argv);
        } catch (...) {
            _scopeManager.closeScope();
            throw;
        }
    }
    /// Create a new scope without an app instance.
    /// `app()` will not work in this scope!
    explicit ApplicationTestScope(NoLocalAppInstance) : _scopeManager{ApplicationTestScopeManager::instance()} {
        _scopeManager.openScope(std::make_unique<ApplicationInstanceBuilder<tApplication, tApplicationData>>());
    }

    /// Destroy this scope and restore the previous application state.
    ~ApplicationTestScope() {
        _application.reset();
        _scopeManager.closeScope();
    };

public:
    /// Access the locally created application instance
    auto app() -> tApplication & {
        if (_application == nullptr) {
            throw IllegalApplicationInstanceAccess("Scope without local app instance.");
        }
        return *_application;
    }

private:
    ApplicationTestScopeManager &_scopeManager; ///< Shared test-scope manager.
    std::unique_ptr<tApplication> _application; ///< Locally owned application instance.
};
