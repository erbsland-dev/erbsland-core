// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationData_fwd.hpp"
#include "ApplicationInstanceManager_fwd.hpp"

#include "../Application_fwd.hpp"

#include <memory>
#include <mutex>

namespace erbsland::core::impl {

/// The manager for the global application instance.
class ApplicationInstanceManager {
public:
    ApplicationInstanceManager() = default;
    virtual ~ApplicationInstanceManager() = default;

public:
    /// Register the user-created application instance.
    /// @return An existing application data instance, or nullptr if no application instance was registered.
    virtual auto registerUserInstance(Application *appInstance) -> ApplicationDataPtr;
    /// Unregister the user-created application instance.
    /// @return `true` if this unregistered the user-owned application instance,
    ///    `false` if the application instance was not registered or was not owned by the user.
    virtual auto unregisterInstance(const Application *appInstance) -> bool;
    /// Link the singleton in this DLL with the data from the main application.
    virtual void linkWith(Application &app);

public:
    /// Access the application instance.
    /// Guaranteed to return a valid application instance.
    /// If `application()` is called before the application instance is created in `main()`, a temporary
    /// application instance is created and its data is passed to the user-owned instance as soon it is created
    /// in the main method.
    [[nodiscard]] virtual auto application() -> Application &;

    /// Create a new data instance for the application.
    [[nodiscard]] virtual auto createApplicationData() -> ApplicationDataPtr;

    /// Access the instance manager instance.
    [[nodiscard]] static auto instance() -> ApplicationInstanceManager *;

#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
public: // methods for unit tests
    /// Set up the system as at the start of the main method.
    static void startSimulatedMain();
    /// Simulate the end of the main method.
    static void stopSimulatedMain();
    /// Replace the instance manager instance;
    static void setInstance(ApplicationInstanceManager *instance) noexcept;
#endif

protected:
#if defined(ERBSLAND_CORE_DEVELOPER_BUILD) || defined(ERBSLAND_UNITTEST_BUILD)
    /// Reset the simulated main method state.
    virtual void resetSimulatedMain();
#endif

private:
    inline static std::mutex _instanceMutex{};             ///< Mutex to protext access to the application instance.
    inline static ApplicationInstanceManager *_instance{}; ///< The singleton instance.
    Application *_userInstance{};                          ///< The user-owned application instance.
    std::unique_ptr<Application> _tempInstance;            ///< A temporary application instance.
};

}
