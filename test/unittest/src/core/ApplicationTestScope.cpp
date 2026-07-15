// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/core/impl/ApplicationInstanceManager.hpp>

std::atomic<bool> ApplicationTestScopeBase::_applicationTestScopeOpen{};
std::unique_ptr<TestApplicationInstanceManagerOverride> ApplicationTestScopeBase::_applicationInstanceManagerOverride;

TestApplicationInstanceManagerOverride::TestApplicationInstanceManagerOverride(
    std::atomic<bool> &applicationTestScopeOpen) :
    _applicationTestScopeOpen{applicationTestScopeOpen} {
}

auto TestApplicationInstanceManagerOverride::registerUserInstance(erbsland::core::Application *appInstance)
    -> erbsland::core::impl::ApplicationDataPtr {
    if (!_applicationTestScopeOpen) {
        throw IllegalApplicationInstanceAccess{
            "ApplicationInstanceManager::registerUserInstance access outside of an open test scope"};
    }
    return ApplicationInstanceManager::registerUserInstance(appInstance);
}

auto TestApplicationInstanceManagerOverride::unregisterInstance(const erbsland::core::Application *appInstance)
    -> bool {
    if (!_applicationTestScopeOpen) {
        throw IllegalApplicationInstanceAccess{
            "ApplicationInstanceManager::unregisterInstance access outside of an open test scope"};
    }
    return ApplicationInstanceManager::unregisterInstance(appInstance);
}
void TestApplicationInstanceManagerOverride::linkWith(erbsland::core::Application &app) {
    if (!_applicationTestScopeOpen) {
        throw IllegalApplicationInstanceAccess{
            "ApplicationInstanceManager::linkWith access outside of an open test scope"};
    }
    ApplicationInstanceManager::linkWith(app);
}
auto TestApplicationInstanceManagerOverride::application() -> erbsland::core::Application & {
    if (!_applicationTestScopeOpen) {
        throw IllegalApplicationInstanceAccess{
            "ApplicationInstanceManager::application access outside of an open test scope"};
    }
    return ApplicationInstanceManager::application();
}
auto TestApplicationInstanceManagerOverride::createApplicationData() -> erbsland::core::impl::ApplicationDataPtr {
    if (!_applicationTestScopeOpen) {
        throw IllegalApplicationInstanceAccess{
            "ApplicationInstanceManager::createApplicationData access outside of an open test scope"};
    }
    if (_applicationInstanceBuilder == nullptr) {
        throw IllegalApplicationInstanceAccess{"No application data builder installed for the open test scope."};
    }
    return _applicationInstanceBuilder->createApplicationData();
}

void ApplicationTestScopeBase::installApplicationInstanceManagerOverride() {
    _applicationInstanceManagerOverride =
        std::make_unique<TestApplicationInstanceManagerOverride>(_applicationTestScopeOpen);
    erbsland::core::impl::ApplicationInstanceManager::setInstance(_applicationInstanceManagerOverride.get());
}
