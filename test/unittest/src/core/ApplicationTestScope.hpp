// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/impl/ApplicationDataImpl.hpp>
#include <erbsland/core/impl/ApplicationInstanceManager.hpp>

#include <atomic>
#include <exception>
#include <memory>
#include <stdexcept>
#include <type_traits>

class TestApplicationInstanceManagerOverride;

/// Exception that is thrown when an application instance is accessed outside of an open test scope.
class IllegalApplicationInstanceAccess : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
    ~IllegalApplicationInstanceAccess() override = default;
};

/// Builder base to create the application and data instance.
class ApplicationInstanceBuilderBase {
public:
    virtual ~ApplicationInstanceBuilderBase() = default;

public:
    virtual auto createApplication() -> std::unique_ptr<erbsland::core::Application> = 0;
    virtual auto createApplication(int argc, char *argv[]) -> std::unique_ptr<erbsland::core::Application> = 0;
    virtual auto createApplication(int argc, wchar_t *argv[]) -> std::unique_ptr<erbsland::core::Application> = 0;
    virtual auto createApplicationData() -> erbsland::core::impl::ApplicationDataPtr = 0;
};

/// Concrete builder
template <
    typename tApplication = erbsland::core::Application,
    typename tApplicationData = erbsland::core::impl::ApplicationDataImpl>
    requires std::is_base_of_v<erbsland::core::Application, tApplication> &&
    std::is_base_of_v<erbsland::core::impl::ApplicationData, tApplicationData>
class ApplicationInstanceBuilder : public ApplicationInstanceBuilderBase {
public:
    ~ApplicationInstanceBuilder() override = default;

public:
    auto createApplication() -> std::unique_ptr<erbsland::core::Application> override {
        if constexpr (std::is_default_constructible_v<tApplication>) {
            return std::make_unique<tApplication>();
        } else {
            throw IllegalApplicationInstanceAccess{"The configured test application has no default constructor."};
        }
    }
    auto createApplication(int argc, char *argv[]) -> std::unique_ptr<erbsland::core::Application> override {
        if constexpr (std::is_constructible_v<tApplication, int, char **>) {
            return std::make_unique<tApplication>(argc, argv);
        } else {
            throw IllegalApplicationInstanceAccess{
                "The configured test application has no constructor for UTF-8 arguments."};
        }
    }
    auto createApplication(int argc, wchar_t *argv[]) -> std::unique_ptr<erbsland::core::Application> override {
        if constexpr (std::is_constructible_v<tApplication, int, wchar_t **>) {
            return std::make_unique<tApplication>(argc, argv);
        } else {
            throw IllegalApplicationInstanceAccess{
                "The configured test application has no constructor for wide arguments."};
        }
    }
    auto createApplicationData() -> erbsland::core::impl::ApplicationDataPtr override {
        return std::make_shared<tApplicationData>();
    }
};

/// Our own version of the application instance manager
class TestApplicationInstanceManagerOverride : public erbsland::core::impl::ApplicationInstanceManager {
public:
    explicit TestApplicationInstanceManagerOverride(std::atomic<bool> &applicationTestScopeOpen);
    ~TestApplicationInstanceManagerOverride() override = default;

public: // override ApplicationInstanceManager
    auto registerUserInstance(erbsland::core::Application *appInstance)
        -> erbsland::core::impl::ApplicationDataPtr override;
    auto unregisterInstance(const erbsland::core::Application *appInstance) -> bool override;
    void linkWith(erbsland::core::Application &app) override;
    [[nodiscard]] auto application() -> erbsland::core::Application & override;
    [[nodiscard]] auto createApplicationData() -> erbsland::core::impl::ApplicationDataPtr override;

public:
    void setBuilder(std::unique_ptr<ApplicationInstanceBuilderBase> &&builder) {
        _applicationInstanceBuilder = std::move(builder);
    }
    void clearBuilder() noexcept { _applicationInstanceBuilder.reset(); }

private:
    std::atomic<bool> &_applicationTestScopeOpen;                                /// Access the global scope flag.
    std::unique_ptr<ApplicationInstanceBuilderBase> _applicationInstanceBuilder; ///< builder
};

/// An application test scope simulates the lifetime of an `Application` class (or derived class)
/// for the duration of a single unit test.
/// Any access to `application()` or `Application::instance()` outside an open test scope
/// results in an exception.
/// This protection detects hidden bindings to the application instance that shouldn't be there.
class ApplicationTestScopeBase {
public:
    ApplicationTestScopeBase() = default;
    virtual ~ApplicationTestScopeBase() = default;

public:
    static void openScope(std::unique_ptr<ApplicationInstanceBuilderBase> &&builder) {
        if (_applicationInstanceManagerOverride == nullptr) {
            installApplicationInstanceManagerOverride();
        }
        if (_applicationTestScopeOpen.exchange(true)) {
            throw IllegalApplicationInstanceAccess{"Nested application test scopes are not supported."};
        }
        try {
            erbsland::core::impl::ApplicationInstanceManager::startSimulatedMain();
            _applicationInstanceManagerOverride->setBuilder(std::move(builder));
        } catch (...) {
            _applicationTestScopeOpen = false;
            throw;
        }
    }
    static void closeScope() {
        erbsland::core::impl::ApplicationInstanceManager::stopSimulatedMain();
        _applicationInstanceManagerOverride->clearBuilder();
        _applicationTestScopeOpen = false;
    }
    static void installApplicationInstanceManagerOverride();

private:
    static std::atomic<bool> _applicationTestScopeOpen;
    static std::unique_ptr<TestApplicationInstanceManagerOverride> _applicationInstanceManagerOverride;
};

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
class ApplicationTestScope : public ApplicationTestScopeBase {
public:
    struct NoLocalAppInstance {};

public:
    /// Create a new scope with a locally created application instance
    ApplicationTestScope() {
        openScope(std::make_unique<ApplicationInstanceBuilder<tApplication, tApplicationData>>());
        try {
            _application = std::make_unique<tApplication>();
        } catch (...) {
            closeScope();
            throw;
        }
    }
    /// Create a new scope with a locally created application instance
    ApplicationTestScope(int argc, char *argv[]) {
        openScope(std::make_unique<ApplicationInstanceBuilder<tApplication, tApplicationData>>());
        try {
            _application = std::make_unique<tApplication>(argc, argv);
        } catch (...) {
            closeScope();
            throw;
        }
    }
    /// Create a new scope with a locally created application instance
    ApplicationTestScope(int argc, wchar_t *argv[]) {
        openScope(std::make_unique<ApplicationInstanceBuilder<tApplication, tApplicationData>>());
        try {
            _application = std::make_unique<tApplication>(argc, argv);
        } catch (...) {
            closeScope();
            throw;
        }
    }
    /// Create a new scope without an app instance.
    /// `app()` will not work in this scope!
    explicit ApplicationTestScope(NoLocalAppInstance) {
        openScope(std::make_unique<ApplicationInstanceBuilder<tApplication, tApplicationData>>());
    }

    ~ApplicationTestScope() override {
        _application.reset();
        closeScope();
    };

public:
    /// Access the locally created application instance
    auto app() -> tApplication & {
        if (_application == nullptr) {
            throw IllegalApplicationInstanceAccess("Scope without local app instance.");
        }
        return *_application;
    }

public:
    std::unique_ptr<tApplication> _application;
};
