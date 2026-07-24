// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationInstanceManager.hpp"

#include "ApplicationDataImpl.hpp"

#include "../Application.hpp"

#include <mutex>
#include <thread>

namespace erbsland::core::impl {

auto ApplicationInstanceManager::registerUserInstance(Application *appInstance) -> ApplicationDataPtr {
    if (appInstance == nullptr) {
        std::terminate();
    }
    std::unique_ptr<Application> tempInstance;
    ApplicationDataPtr data;
    {
        const auto lock = std::scoped_lock{_instanceMutex};
        if (_tempInstance != nullptr) {
            // Extract the data from the temporary instance and delete it after releasing the mutex.
            data = _tempInstance->_data;
            tempInstance = std::move(_tempInstance);
        }
        if (_userInstance != nullptr) {
            // This is a conflict situation where the user creates a second instance.
            // We handle it gracefully but return existing data.
            // `Application` will terminate if also new command line arguments
            data = _userInstance->_data;
        } else {
            // Only keep the first user registered instance.
            _userInstance = appInstance;
        }
        if (data == nullptr) {
            // If this is the first instance, create a new data object.
            data = createApplicationData();
        }
    }
    tempInstance.reset();
    return data;
}

auto ApplicationInstanceManager::unregisterInstance(const Application *appInstance) -> bool {
    if (appInstance == nullptr) {
        std::terminate();
    }
    const auto lock = std::scoped_lock{_instanceMutex};
    if (_userInstance == appInstance) {
        // In case of a conflict situation, make sure to only delete the original user instance.
        _userInstance = nullptr;
        return true;
    }
    return false;
}

void ApplicationInstanceManager::linkWith(Application &app) {
    auto lock = std::scoped_lock{_instanceMutex};
    if (_userInstance != nullptr) {
        _userInstance->_data = app._data;
        return;
    }
    if (_tempInstance != nullptr) {
        _tempInstance->_data = app._data;
        return;
    }
    _tempInstance.reset(new Application(app._data));
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
    _tempInstance.reset(new Application(createApplicationData()));
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
    {
        auto lock = std::scoped_lock{_instanceMutex};
        _userInstance = nullptr;
        tempInstance = std::move(_tempInstance);
    }
    tempInstance.reset();
}
#endif

}
