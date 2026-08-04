// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScopeManager.hpp"
#include "IllegalApplicationInstanceAccess.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/core/impl/ApplicationInstanceManager.hpp>

auto ApplicationTestScopeManager::instance() -> ApplicationTestScopeManager & {
    static auto manager = ApplicationTestScopeManager{};
    return manager;
}

void ApplicationTestScopeManager::install() {
    auto &manager = instance();
    erbsland::core::impl::ApplicationInstanceManager::setInstance(&manager);
}

void ApplicationTestScopeManager::openScope(std::unique_ptr<ApplicationInstanceBuilderBase> &&builder) {
    install();
    if (_isScopeOpen.exchange(true)) {
        throw IllegalApplicationInstanceAccess{"Nested application test scopes are not supported."};
    }
    try {
        erbsland::core::impl::ApplicationInstanceManager::startSimulatedMain();
        _applicationInstanceBuilder = std::move(builder);
    } catch (...) {
        _isScopeOpen = false;
        throw;
    }
}

void ApplicationTestScopeManager::closeScope() {
    erbsland::core::impl::ApplicationInstanceManager::stopSimulatedMain();
    _applicationInstanceBuilder.reset();
    _isScopeOpen = false;
}

auto ApplicationTestScopeManager::registerUserInstance(erbsland::core::Application *appInstance)
    -> erbsland::core::impl::ApplicationDataPtr {
    if (!_isScopeOpen) {
        throw IllegalApplicationInstanceAccess{
            "ApplicationInstanceManager::registerUserInstance access outside of an open test scope"};
    }
    return ApplicationInstanceManager::registerUserInstance(appInstance);
}

auto ApplicationTestScopeManager::unregisterInstance(const erbsland::core::Application *appInstance) -> bool {
    if (!_isScopeOpen) {
        throw IllegalApplicationInstanceAccess{
            "ApplicationInstanceManager::unregisterInstance access outside of an open test scope"};
    }
    return ApplicationInstanceManager::unregisterInstance(appInstance);
}
void ApplicationTestScopeManager::linkWith(erbsland::core::Application &app) {
    if (!_isScopeOpen) {
        throw IllegalApplicationInstanceAccess{
            "ApplicationInstanceManager::linkWith access outside of an open test scope"};
    }
    ApplicationInstanceManager::linkWith(app);
}
auto ApplicationTestScopeManager::application() -> erbsland::core::Application & {
    if (!_isScopeOpen) {
        throw IllegalApplicationInstanceAccess{
            "ApplicationInstanceManager::application access outside of an open test scope"};
    }
    return ApplicationInstanceManager::application();
}
auto ApplicationTestScopeManager::createApplicationData() -> erbsland::core::impl::ApplicationDataPtr {
    if (!_isScopeOpen) {
        throw IllegalApplicationInstanceAccess{
            "ApplicationInstanceManager::createApplicationData access outside of an open test scope"};
    }
    if (_applicationInstanceBuilder == nullptr) {
        throw IllegalApplicationInstanceAccess{"No application data builder installed for the open test scope."};
    }
    return _applicationInstanceBuilder->createApplicationData();
}
