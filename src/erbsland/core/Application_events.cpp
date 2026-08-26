// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Application.hpp"

#include "ApplicationError.hpp"

#include "impl/ApplicationData.hpp"
#include "impl/ApplicationInstanceManager.hpp"
#include "impl/EventData.hpp"
#include "impl/LibraryVersion.hpp"

#include "../cryptology/configuration/CryptologyConfiguration.hpp"
#include "../cterm/Terminal.hpp"
#include "../cterm/TerminalStream.hpp"
#include "../err/DiagnosticHelper.hpp"
#include "../event/EventLoop.hpp"
#include "../event/EventLoopErrorAction.hpp"
#include "../event/impl/CurrentEventsScope.hpp"
#include "../event/impl/ManagedEventThread.hpp"
#include "../event/ManagedEventThread.hpp"
#include "../i18n/DisplayTextMap.hpp"
#include "../options/OptionError.hpp"
#include "../options/OptionManager.hpp"
#include "../options/OptionModule.hpp"
#include "../random/SecureRandom.hpp"
#include "../random/ThreadSafeFastRandom.hpp"
#include "../resource/Resources.hpp"
#include "../stream/StandardStreams.hpp"
#include "../system/UserLookup.hpp"
#include "../text/TextDocument.hpp"

#include <algorithm>
#include <exception>
#include <memory>
#include <utility>
#include <vector>

namespace erbsland::core {
using namespace text::literals;
using namespace event;

auto Application::runEventLoop() -> unit::ExitCode {
    auto &eventData = _data->event();
    const auto loopError = std::make_shared<std::exception_ptr>();
    eventData.eventLoop->setErrorHandler([loopError](std::exception_ptr error) -> EventLoopErrorAction {
        if (*loopError == nullptr) {
            *loopError = std::move(error);
        }
        return EventLoopErrorAction::Stop;
    });
    {
        auto currentEventsScope = event::impl::CurrentEventsScope{eventData.eventLoop};
        eventData.eventLoop->run();
    }
    if (const auto manager = partManagerIfCreated(); manager != nullptr) {
        const auto managerState = manager->state();
        if (managerState == ApplicationPartManagerState::Starting ||
            managerState == ApplicationPartManagerState::Running) {
            manager->stop();
            if (!eventData.eventLoop->isQuitRequested()) {
                auto currentEventsScope = event::impl::CurrentEventsScope{eventData.eventLoop};
                eventData.eventLoop->run();
            }
        }
    }
    eventData.eventLoop->setErrorHandler({});
    auto exitCode = unit::ExitCode::success();
    auto eventThreads = std::vector<ManagedEventThreadPtr>{};
    {
        std::scoped_lock lock{eventData.mutex};
        if (eventData.quitExitCodeSet) {
            exitCode = eventData.quitExitCode;
        }
        for (const auto &eventThreadWeakPtr : eventData.eventThreads) {
            if (auto eventThread = eventThreadWeakPtr.lock(); eventThread != nullptr) {
                eventThreads.emplace_back(std::move(eventThread));
            }
        }
    }
    for (const auto &eventThread : eventThreads) {
        eventThread->quit();
    }
    for (const auto &eventThread : eventThreads) {
        eventThread->join();
    }
    if (*loopError != nullptr) {
        std::rethrow_exception(*loopError);
    }
    if (const auto manager = partManagerIfCreated(); manager != nullptr && manager->hasError()) {
        std::rethrow_exception(manager->takeError());
    }
    return exitCode;
}

auto Application::events() -> EventsPtr {
    return _data->event().eventLoop;
}

auto Application::eventRegistry() -> EventRegistry & {
    return _data->event().eventIdRegistry;
}

auto Application::createEventThread() -> ManagedEventThreadPtr {
    auto result = std::make_shared<event::impl::ManagedEventThread>();
    auto &eventData = _data->event();
    {
        std::scoped_lock lock{eventData.mutex};
        std::erase_if(eventData.eventThreads, [](const ManagedEventThreadWeakPtr &eventThreadWeakPtr) -> bool {
            return eventThreadWeakPtr.expired();
        });
        eventData.eventThreads.emplace_back(result);
    }
    return result;
}

void Application::quit(unit::ExitCode exitCode) noexcept {
    try {
        auto &eventData = _data->event();
        {
            std::scoped_lock lock{eventData.mutex};
            if (!eventData.quitExitCodeSet) {
                eventData.quitExitCode = exitCode;
                eventData.quitExitCodeSet = true;
            }
        }
        if (const auto manager = partManagerIfCreated(); manager != nullptr) {
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
        quitEventSystem();
    } catch (...) { // NOLINT(*-empty-catch)
        // `quit()` must be safe to call from cleanup paths.
    }
}

auto Application::partManagerIfCreated() const noexcept -> ApplicationPartManagerPtr {
    const auto lock = std::scoped_lock{_data->partManagerMutex()};
    return _data->partManager();
}

void Application::stopPartManager() {
    const auto manager = partManagerIfCreated();
    if (manager == nullptr) {
        return;
    }
    auto managerState = manager->state();
    if (managerState == ApplicationPartManagerState::Ready || managerState == ApplicationPartManagerState::Starting ||
        managerState == ApplicationPartManagerState::Running) {
        manager->stop();
        managerState = manager->state();
    }
    if (managerState == ApplicationPartManagerState::Ready || managerState == ApplicationPartManagerState::Starting ||
        managerState == ApplicationPartManagerState::Running || managerState == ApplicationPartManagerState::Stopping) {
        auto &eventLoop = _data->event().eventLoop;
        auto currentEventsScope = event::impl::CurrentEventsScope{eventLoop};
        eventLoop->run();
    }
    if (manager->hasError()) {
        std::rethrow_exception(manager->takeError());
    }
}

void Application::quitEventSystem() noexcept {
    try {
        auto eventThreads = std::vector<ManagedEventThreadPtr>{};
        auto eventLoop = EventLoopPtr{};
        auto &eventData = _data->event();
        {
            const auto lock = std::scoped_lock{eventData.mutex};
            eventLoop = eventData.eventLoop;
            std::erase_if(eventData.eventThreads, [](const ManagedEventThreadWeakPtr &eventThreadWeakPtr) -> bool {
                return eventThreadWeakPtr.expired();
            });
            for (const auto &eventThreadWeakPtr : eventData.eventThreads) {
                if (auto eventThread = eventThreadWeakPtr.lock(); eventThread != nullptr) {
                    eventThreads.emplace_back(std::move(eventThread));
                }
            }
        }
        eventLoop->quit();
        for (const auto &eventThread : eventThreads) {
            eventThread->quit();
        }
    } catch (...) { // NOLINT(*-empty-catch)
        // Application event shutdown must remain safe in cleanup paths.
    }
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
