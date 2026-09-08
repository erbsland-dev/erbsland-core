// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Application.hpp"

#include "impl/application_data/ApplicationEventData.hpp"
#include "impl/application_data/ApplicationLifecycleData.hpp"
#include "impl/application_data/ApplicationPartsData.hpp"
#include "impl/ApplicationData.hpp"
#include "impl/ApplicationInstanceManager.hpp"
#include "impl/LibraryVersion.hpp"

#include "../event/EventLoop.hpp"
#include "../event/ManagedEventThread.hpp"

#include <exception>

namespace erbsland::core {

using namespace event;

auto Application::runEventLoop() -> unit::ExitCode {
    const auto eventData = _data->events().get();
    eventData->runLoop();
    const auto partsData = _data->parts().getIfExists();
    const auto manager = partsData != nullptr ? partsData->managerIfExists() : nullptr;
    if (manager != nullptr) {
        const auto managerState = manager->state();
        if (managerState == ApplicationPartManagerState::Starting ||
            managerState == ApplicationPartManagerState::Running) {
            manager->stop();
            if (!eventData->isQuitRequested()) {
                eventData->runLoop();
            }
        }
    }
    const auto exitCode = eventData->finishRun();
    if (manager != nullptr && manager->hasError()) {
        std::rethrow_exception(manager->takeError());
    }
    return exitCode;
}

auto Application::events() -> EventsPtr {
    return _data->events().get()->events();
}

auto Application::eventRegistry() -> EventRegistry & {
    return _data->events().get()->eventRegistry();
}

auto Application::createEventThread() -> ManagedEventThreadPtr {
    return _data->events().get()->createEventThread();
}

void Application::quit(const unit::ExitCode exitCode) noexcept {
    requestQuit(_data, exitCode);
}

void Application::requestQuit(const impl::ApplicationDataPtr &data, const unit::ExitCode exitCode) noexcept {
    try {
        if (const auto lifecycleData = data->lifecycle().getIfExists(); lifecycleData != nullptr) {
            lifecycleData->requestShutdown();
            lifecycleData->reportServiceStopping();
        }
        const auto eventData = data->events().get();
        eventData->recordExitCode(exitCode);
        const auto partsData = data->parts().getIfExists();
        const auto manager = partsData != nullptr ? partsData->managerIfExists() : nullptr;
        if (manager != nullptr) {
            const auto managerState = manager->state();
            if (managerState == ApplicationPartManagerState::Ready ||
                managerState == ApplicationPartManagerState::Starting ||
                managerState == ApplicationPartManagerState::Running) {
                manager->stop();
                return;
            }
            if (managerState == ApplicationPartManagerState::Stopping) {
                return;
            }
        }
        eventData->quit();
    } catch (...) {}
}

auto Application::instance() -> Application & {
    return impl::ApplicationInstanceManager::instance()->application();
}

auto Application::linkWith(Application &app) -> void {
    impl::ApplicationInstanceManager::instance()->linkWith(app);
}

auto Application::libraryVersion() noexcept -> unit::Version {
    return impl::libraryVersion();
}

auto Application::libraryVersionText() noexcept -> text::String {
    return impl::libraryVersionText();
}

#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
// These virtual functions are only available in developer builds.
// They are not available in regular release *and* debug builds to prevent accidental or intentional
// manipulation of the random number generator (corrupting the vtable).
void Application::initializeRandom([[maybe_unused]] random::RandomPtr &randomPtr) noexcept {
}
void Application::initializeSecureRandom([[maybe_unused]] random::RandomPtr &randomPtr) noexcept {
}
#endif

auto application() -> Application & {
    return impl::ApplicationInstanceManager::instance()->application();
}

}
