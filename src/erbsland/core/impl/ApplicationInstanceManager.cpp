// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationInstanceManager.hpp"

#include "ApplicationDataImpl.hpp"
#include "ApplicationExecutionContext.hpp"

#include "../Application.hpp"

#include <mutex>
#include <utility>

namespace erbsland::core::impl {

auto ApplicationInstanceManager::registerUserInstance(Application *appInstance) -> ApplicationDataPtr {
    if (appInstance == nullptr) {
        std::terminate();
    }
    std::unique_ptr<Application> tempInstance;
    ApplicationDataPtr data;
    {
        const auto lock = std::scoped_lock{_instanceMutex};
        if (_data == nullptr) {
            _data = createApplicationData();
        }
        if (_tempInstance != nullptr) {
            tempInstance = std::move(_tempInstance);
        }
        if (_userInstance == nullptr) {
            _userInstance = appInstance;
        }
        data = _data;
    }
    tempInstance.reset();
    return data;
}

auto ApplicationInstanceManager::unregisterInstance(const Application *appInstance) -> bool {
    if (appInstance == nullptr) {
        std::terminate();
    }
    ApplicationDataPtr releasedData;
    {
        const auto lock = std::scoped_lock{_instanceMutex};
        if (_userInstance != appInstance) {
            return false;
        }
        _userInstance = nullptr;
        releasedData = std::move(_data);
    }
    return true;
}

void ApplicationInstanceManager::linkWith(Application &app) {
    ApplicationDataPtr releasedData;
    ApplicationDataPtr releasedFacadeData;
    {
        const auto lock = std::scoped_lock{_instanceMutex};
        auto data = app._data;
        releasedData = std::move(_data);
        _data = data;
        if (_userInstance != nullptr) {
            releasedFacadeData = std::move(_userInstance->_data);
            _userInstance->_data = std::move(data);
        } else if (_tempInstance != nullptr) {
            releasedFacadeData = std::move(_tempInstance->_data);
            _tempInstance->_data = std::move(data);
        }
    }
}

auto ApplicationInstanceManager::retainData() -> ApplicationDataPtr {
    if (auto data = ApplicationExecutionContext::current(); data != nullptr) {
        return data;
    }
    return retainGlobalData();
}

auto ApplicationInstanceManager::retainGlobalData() -> ApplicationDataPtr {
    const auto lock = std::scoped_lock{_instanceMutex};
    if (_data == nullptr) {
        _data = createApplicationData();
    }
    return _data;
}

auto ApplicationInstanceManager::application() -> Application & {
    auto lock = std::scoped_lock{_instanceMutex};
    if (_userInstance != nullptr) {
        return *_userInstance;
    }
    if (_tempInstance != nullptr) {
        return *_tempInstance;
    }
    // Special case where no user instance was registered yet.
    // May happen if static initialization code is calling `application()` before `main()` is entered.
    // This instance is kept alive until the user instance is registered or the application terminates.
    if (_data == nullptr) {
        _data = createApplicationData();
    }
    _tempInstance.reset(new Application(_data));
    return *_tempInstance;
}

auto ApplicationInstanceManager::createApplicationData() -> ApplicationDataPtr {
    return std::make_shared<ApplicationDataImpl>();
}

auto ApplicationInstanceManager::instance() -> ApplicationInstanceManager * {
    auto lock = std::scoped_lock{_instanceMutex};
    if (_instance == nullptr) {
        _instance = new ApplicationInstanceManager();
    }
    return _instance;
}

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
void ApplicationInstanceManager::startSimulatedMain() {
    instance()->resetSimulatedMain();
}

void ApplicationInstanceManager::stopSimulatedMain() {
    instance()->resetSimulatedMain();
}

void ApplicationInstanceManager::setInstance(ApplicationInstanceManager *instance) noexcept {
    assert(instance != nullptr);
    auto lock = std::scoped_lock{_instanceMutex};
    _instance = instance;
}

void ApplicationInstanceManager::resetSimulatedMain() {
    std::unique_ptr<Application> tempInstance;
    ApplicationDataPtr releasedData;
    {
        auto lock = std::scoped_lock{_instanceMutex};
        _userInstance = nullptr;
        tempInstance = std::move(_tempInstance);
        releasedData = std::move(_data);
    }
    tempInstance.reset();
}

#endif

}
