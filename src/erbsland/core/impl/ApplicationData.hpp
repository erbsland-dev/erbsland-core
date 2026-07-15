// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationData_fwd.hpp"

#include "../ApplicationInfo.hpp"
#include "../CommandLineArguments.hpp"
#include "../InitializeFn.hpp"
#include "../MainFn.hpp"

#include "../../cterm/Terminal_fwd.hpp"
#include "../../cterm/TerminalDocumentStyle.hpp"
#include "../../event/EventRegistry.hpp"
#include "../../event/EventThread_fwd.hpp"
#include "../../event/impl/EventLoop.hpp"
#include "../../i18n/DisplayTextMap_fwd.hpp"
#include "../../options/Options.hpp"
#include "../../options/OptionValues.hpp"
#include "../../random/Random_fwd.hpp"
#include "../../stream/StandardStreamRedirect.hpp"
#include "../../system/UserLookup_fwd.hpp"
#include "../../text/TextDocument_fwd.hpp"
#include "../../unit/ExitCode.hpp"

#include <mutex>
#include <vector>

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
            eventIdRegistry{event::EventRegistry::PrivateTag{}} {}
        event::EventLoopPtr eventLoop;
        event::EventRegistry eventIdRegistry; ///< The event registry.
        std::mutex mutex;                     ///< Protects managed event system state.
        bool quitExitCodeSet{false};          ///< True if the application exit code was set by `quit()`.
        unit::ExitCode quitExitCode;          ///< The first exit code passed to `quit()`.
        std::vector<event::ManagedEventThreadWeakPtr> eventThreads; ///< The managed event threads.
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
    /// Render a system-output document to the best available output target.
    virtual void renderSystemOutput(const text::TextDocument &document) = 0;

public: // accessors
    [[nodiscard]] virtual auto info() noexcept -> ApplicationInfo & = 0;
    [[nodiscard]] virtual auto commandLineArguments() const noexcept -> const CommandLineArguments & = 0;
    [[nodiscard]] virtual auto options() noexcept -> const options::OptionsPtr & = 0;
    virtual void setOptions(options::OptionsPtr options) noexcept = 0;
    [[nodiscard]] virtual auto optionValues() noexcept -> const options::OptionValuesPtr & = 0;
    virtual void setOptionValues(options::OptionValuesPtr optionValues) noexcept = 0;
    [[nodiscard]] virtual auto systemOutputStyle() const noexcept -> const cterm::TerminalDocumentStyle & = 0;
    virtual void setSystemOutputStyle(cterm::TerminalDocumentStyle style) noexcept = 0;
    [[nodiscard]] virtual auto initializeFn() noexcept -> const InitializeFn & = 0;
    virtual void setInitializeFn(InitializeFn initializeFn) noexcept = 0;
    [[nodiscard]] virtual auto mainFn() noexcept -> const MainFn & = 0;
    virtual void setMainFn(MainFn mainFn) noexcept = 0;
    [[nodiscard]] virtual auto randomMutex() noexcept -> std::mutex & = 0;
    [[nodiscard]] virtual auto random() noexcept -> const random::RandomPtr & = 0;
    virtual void setRandom(random::RandomPtr random) noexcept = 0;
    [[nodiscard]] virtual auto secureRandom() noexcept -> const random::RandomPtr & = 0;
    virtual void setSecureRandom(random::RandomPtr random) noexcept = 0;
    [[nodiscard]] virtual auto systemMutex() noexcept -> std::mutex & = 0;
    [[nodiscard]] virtual auto displayText() noexcept -> const i18n::DisplayTextMapConstPtr & = 0;
    virtual void setDisplayText(i18n::DisplayTextMapConstPtr displayText) noexcept = 0;
    [[nodiscard]] virtual auto userLookup() noexcept -> const system::UserLookupPtr & = 0;
    virtual void setUserLookup(system::UserLookupPtr userLookup) noexcept = 0;
    [[nodiscard]] virtual auto isTerminalEnabled() const noexcept -> bool = 0;
    virtual void setTerminalEnabled(bool enabled) noexcept = 0;
    [[nodiscard]] virtual auto terminal() noexcept -> const cterm::TerminalPtr & = 0;
    virtual void setTerminal(cterm::TerminalPtr terminal) noexcept = 0;
    virtual void setStandardStreamRedirect(stream::StandardStreamRedirect redirect) noexcept = 0;
};

}
