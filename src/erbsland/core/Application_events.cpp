// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Application.hpp"

#include "impl/application_data/ApplicationEventData.hpp"
#include "impl/application_data/ApplicationLifecycleData.hpp"
#include "impl/application_data/ApplicationPartsData.hpp"
#include "impl/ApplicationData.hpp"
#include "impl/ApplicationInstanceManager.hpp"
#include "impl/LibraryVersion.hpp"

#include "../err/RuntimeError.hpp"
#include "../event/EventLoop.hpp"
#include "../event/ManagedEventThread.hpp"

#include <exception>

namespace erbsland::core {

using namespace unit;
using namespace event;

auto Application::runMainEventLoop() -> ExitCode {
    const auto eventData = _data->events().get();
    eventData->runMainEventLoop();
    const auto partsData = _data->parts().getIfExists();
    const auto manager = partsData != nullptr ? partsData->managerIfExists() : nullptr;
    if (manager != nullptr) {
        const auto managerState = manager->state();
        if (managerState == ApplicationPartManagerState::Starting ||
            managerState == ApplicationPartManagerState::Running) {
            manager->stop();
            if (!eventData->isQuitRequested()) {
                eventData->runMainEventLoop();
            }
        }
    }
    const auto exitCode = eventData->finishRun();
    if (manager != nullptr && manager->hasError()) {
        std::rethrow_exception(manager->takeError());
    }
    return exitCode;
}

auto Application::loop() -> EventTargetPtr {
    return _data->events().get()->loop();
}

auto Application::eventRegistry() -> EventRegistry & {
    return _data->events().get()->eventRegistry();
}

auto Application::createEventThread(const event::EventLoopOptions &options) -> ManagedEventThreadPtr {
    return _data->events().get()->createEventThread(options);
}

void Application::quit(const ExitCode exitCode) noexcept {
    requestQuit(_data, exitCode);
}

void Application::requestQuit(const impl::ApplicationDataPtr &data, const ExitCode exitCode) noexcept {
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
    } catch (const err::RuntimeError &) {}
}

auto Application::instance() -> Application & {
    return core::application();
}

auto Application::linkWith(Application &app) -> void {
    impl::ApplicationInstanceManager::instance()->linkWith(app);
}

auto Application::libraryVersion() noexcept -> Version {
    return impl::libraryVersion();
}

auto Application::libraryVersionText() noexcept -> text::String {
    return impl::libraryVersionText();
}

auto application() -> Application & {
    return impl::ApplicationInstanceManager::instance()->application();
}

}
