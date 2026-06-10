// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationData_fwd.hpp"

#include "../ApplicationInfo.hpp"
#include "../CommandLineArguments.hpp"
#include "../MainFn.hpp"

#include "../../cterm/Terminal_fwd.hpp"
#include "../../event/EventIdRegistry.hpp"
#include "../../event/impl/EventLoop.hpp"
#include "../../options/OptionRenderer.hpp"
#include "../../options/Options.hpp"
#include "../../options/OptionValues.hpp"
#include "../../random/Random_fwd.hpp"
#include "../../stream/StandardStreamRedirect.hpp"

#include <mutex>

namespace erbsland::core::impl {

/// The interface for the internal data of the application.
/// This is the actual singleton to allow temporary `Application` instances.
/// Construction and `setCommandLineArguments` are protected by the mutex in `ApplicationInstanceManager`.
/// @tested{ApplicationTestScopeTest}
class ApplicationData {
public:
    struct EventData {
        EventData() :
            eventLoop{std::make_shared<event::impl::EventLoop>()},
            eventIdRegistry{event::EventIdRegistry::PrivateTag{}} {}
        event::EventLoopPtr eventLoop;
        event::EventIdRegistry eventIdRegistry; ///< The event id registry.
    };
    using EventDataPtr = std::unique_ptr<EventData>;

public:
    ApplicationData() = default;
    virtual ~ApplicationData() = default;

public:
    /// Set the command line arguments from the user application instance in `main()`.
    virtual void setCommandLineArguments(int argc, char *argv[]) = 0;
    /// Set the command line arguments from the user application instance in `main()`.
    virtual void setCommandLineArguments(int argc, wchar_t *argv[]) = 0;
    /// Do cleanup tasks before application exit.
    /// - Restore terminal integration after the application instance has been destroyed.
    virtual void cleanupBeforeAppExit() noexcept = 0;
    /// Access and lazy creation of the event system data.
    [[nodiscard]] virtual auto event() -> EventData & = 0;

public: // accessors
    [[nodiscard]] virtual auto info() noexcept -> ApplicationInfo & = 0;
    [[nodiscard]] virtual auto info() const noexcept -> const ApplicationInfo & = 0;
    [[nodiscard]] virtual auto commandLineArguments() const noexcept -> const CommandLineArguments & = 0;
    [[nodiscard]] virtual auto options() noexcept -> options::OptionsPtr & = 0;
    [[nodiscard]] virtual auto options() const noexcept -> const options::OptionsPtr & = 0;
    [[nodiscard]] virtual auto optionValues() noexcept -> options::OptionValuesPtr & = 0;
    [[nodiscard]] virtual auto optionValues() const noexcept -> const options::OptionValuesPtr & = 0;
    [[nodiscard]] virtual auto optionRenderer() noexcept -> options::OptionRendererPtr & = 0;
    [[nodiscard]] virtual auto mainFn() noexcept -> MainFn & = 0;
    [[nodiscard]] virtual auto randomMutex() noexcept -> std::mutex & = 0;
    [[nodiscard]] virtual auto random() noexcept -> random::RandomPtr & = 0;
    [[nodiscard]] virtual auto secureRandom() noexcept -> random::RandomPtr & = 0;
    [[nodiscard]] virtual auto isTerminalEnabled() const noexcept -> bool = 0;
    virtual void setTerminalEnabled(bool enabled) noexcept = 0;
    [[nodiscard]] virtual auto terminal() noexcept -> cterm::TerminalPtr & = 0;
    [[nodiscard]] virtual auto terminal() const noexcept -> const cterm::TerminalPtr & = 0;
    [[nodiscard]] virtual auto standardStreamRedirect() noexcept -> stream::StandardStreamRedirect & = 0;
};

}
