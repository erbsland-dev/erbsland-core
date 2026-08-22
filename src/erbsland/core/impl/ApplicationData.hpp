// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationData_fwd.hpp"
#include "EventData_fwd.hpp"

#include "../ApplicationInfo.hpp"
#include "../ApplicationPartManager_fwd.hpp"
#include "../CommandLineArguments.hpp"
#include "../InitializeFn.hpp"
#include "../MainFn.hpp"

#include "../../cryptology/configuration/CryptologyConfiguration_fwd.hpp"
#include "../../cterm/Terminal_fwd.hpp"
#include "../../cterm/TerminalDocumentStyle.hpp"
#include "../../i18n/DisplayTextMap_fwd.hpp"
#include "../../options/Options.hpp"
#include "../../options/OptionSensitiveTextLocation.hpp"
#include "../../options/OptionValues.hpp"
#include "../../random/Random_fwd.hpp"
#include "../../resource/Resources_fwd.hpp"
#include "../../stream/StandardStreamRedirect.hpp"
#include "../../system/UserLookup_fwd.hpp"
#include "../../text/TextDocument_fwd.hpp"
#include "../../unit/ExitCode.hpp"

#include <mutex>

namespace erbsland::core::impl {

/// The interface for the internal data of the application.
/// This is the actual singleton to allow temporary `Application` instances.
/// Construction and `setCommandLineArguments` are protected by the mutex in `ApplicationInstanceManager`.
/// @tested{ApplicationOptionsTest ApplicationPartApplicationTest ApplicationTestScopeTest}
class ApplicationData {
public:
    // defaults
    ApplicationData() = default;
    virtual ~ApplicationData() = default;

public:
    /// Set and convert borrowed narrow command-line arguments from `main()`.
    /// The argument vector remains owned by the caller and must remain valid for the application lifetime.
    virtual void setCommandLineArguments(int argc, char *argv[]) = 0;
    /// Set and convert borrowed wide command-line arguments from `wmain()`.
    /// The argument vector remains owned by the caller and must remain valid for the application lifetime.
    virtual void setCommandLineArguments(int argc, wchar_t *argv[]) = 0;
    /// Do cleanup tasks before application exit.
    /// - Restore terminal integration after the application instance has been destroyed.
    virtual void cleanupBeforeAppExit() noexcept = 0;
    /// Access and lazy creation of the event system data.
    [[nodiscard]] virtual auto event() -> EventData & = 0;
    /// Render a system-output document to the best available output target.
    virtual void renderSystemOutput(const text::TextDocument &document) = 0;

public: // accessors
    /// Access the immutable application information.
    [[nodiscard]] virtual auto info() noexcept -> ApplicationInfo & = 0;
    /// Access the converted command-line arguments.
    [[nodiscard]] virtual auto commandLineArguments() const noexcept -> const CommandLineArguments & = 0;
    /// Access mutable converted arguments exclusively for option parsing and sensitive-text masking.
    [[nodiscard]] virtual auto commandLineArgumentsForParsing() noexcept -> CommandLineArguments & = 0;
    /// Mask sensitive suffixes in the borrowed native argument vector without changing its layout.
    /// Existing bytes or code units are replaced with stars; terminators and buffer sizes are preserved.
    /// @param locations The sensitive locations reported by the option parser.
    virtual void maskSensitiveCommandLineText(const options::OptionSensitiveTextLocations &locations) noexcept = 0;
    /// Access the configured command-line options.
    [[nodiscard]] virtual auto options() noexcept -> const options::OptionsPtr & = 0;
    /// Set the configured command-line options.
    virtual void setOptions(options::OptionsPtr options) noexcept = 0;
    /// Access the resolved option values.
    [[nodiscard]] virtual auto optionValues() noexcept -> const options::OptionValuesPtr & = 0;
    /// Set the resolved option values.
    virtual void setOptionValues(options::OptionValuesPtr optionValues) noexcept = 0;
    /// Access the style used for system output.
    [[nodiscard]] virtual auto systemOutputStyle() const noexcept -> const cterm::TerminalDocumentStyle & = 0;
    /// Set the style used for system output.
    virtual void setSystemOutputStyle(cterm::TerminalDocumentStyle style) noexcept = 0;
    /// Access the application's initialization function.
    [[nodiscard]] virtual auto initializeFn() noexcept -> const InitializeFn & = 0;
    /// Set the application's initialization function.
    virtual void setInitializeFn(InitializeFn initializeFn) noexcept = 0;
    /// Access the application's main function.
    [[nodiscard]] virtual auto mainFn() noexcept -> const MainFn & = 0;
    /// Set the application's main function.
    virtual void setMainFn(MainFn mainFn) noexcept = 0;
    /// Access the mutex protecting application-part manager creation.
    [[nodiscard]] virtual auto partManagerMutex() noexcept -> std::mutex & = 0;
    /// Access the application-part manager, if created.
    [[nodiscard]] virtual auto partManager() noexcept -> const ApplicationPartManagerPtr & = 0;
    /// Set the application-part manager.
    virtual void setPartManager(ApplicationPartManagerPtr manager) noexcept = 0;
    /// Access the mutex protecting the random generators.
    [[nodiscard]] virtual auto randomMutex() noexcept -> std::mutex & = 0;
    /// Access the standard random generator.
    [[nodiscard]] virtual auto random() noexcept -> const random::RandomPtr & = 0;
    /// Set the standard random generator.
    virtual void setRandom(random::RandomPtr random) noexcept = 0;
    /// Access the cryptographically secure random generator.
    [[nodiscard]] virtual auto secureRandom() noexcept -> const random::RandomPtr & = 0;
    /// Set the cryptographically secure random generator.
    virtual void setSecureRandom(random::RandomPtr random) noexcept = 0;
    /// Access the cryptology configuration.
    [[nodiscard]] virtual auto cryptologyConfiguration() -> cryptology::CryptologyConfiguration & = 0;
    /// Access the compiled-resource manager.
    [[nodiscard]] virtual auto resources() -> const resource::Resources & = 0;
    /// Access the mutex protecting system integration state.
    [[nodiscard]] virtual auto systemMutex() noexcept -> std::mutex & = 0;
    /// Access the application display-text map.
    [[nodiscard]] virtual auto displayText() noexcept -> const i18n::DisplayTextMapConstPtr & = 0;
    /// Set the application display-text map.
    virtual void setDisplayText(i18n::DisplayTextMapConstPtr displayText) noexcept = 0;
    /// Access the user lookup service.
    [[nodiscard]] virtual auto userLookup() noexcept -> const system::UserLookupPtr & = 0;
    /// Set the user lookup service.
    virtual void setUserLookup(system::UserLookupPtr userLookup) noexcept = 0;
    /// Test whether terminal integration is enabled.
    [[nodiscard]] virtual auto isTerminalEnabled() const noexcept -> bool = 0;
    /// Enable or disable terminal integration.
    virtual void setTerminalEnabled(bool enabled) noexcept = 0;
    /// Access the configured terminal.
    [[nodiscard]] virtual auto terminal() noexcept -> const cterm::TerminalPtr & = 0;
    /// Set the configured terminal.
    virtual void setTerminal(cterm::TerminalPtr terminal) noexcept = 0;
    /// Set the standard-stream redirect used by the application.
    virtual void setStandardStreamRedirect(stream::StandardStreamRedirect redirect) noexcept = 0;
};

}
