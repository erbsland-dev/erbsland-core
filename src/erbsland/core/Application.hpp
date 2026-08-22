// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Application_fwd.hpp"
#include "ApplicationInfo.hpp"
#include "ApplicationPartManager.hpp"
#include "ApplicationPartTraits.hpp"
#include "CommandLineArguments.hpp"
#include "InitializeFn.hpp"
#include "MainFn.hpp"

#include "impl/ApplicationData_fwd.hpp"
#include "impl/ApplicationInstanceManager_fwd.hpp"

#include "../cryptology/configuration/CryptologyConfiguration_fwd.hpp"
#include "../cterm/Terminal_fwd.hpp"
#include "../cterm/TerminalDocumentStyle.hpp"
#include "../event/EventLoop_fwd.hpp"
#include "../event/EventRegistry_fwd.hpp"
#include "../event/Events_fwd.hpp"
#include "../event/EventThread_fwd.hpp"
#include "../i18n/DisplayTextMap_fwd.hpp"
#include "../options/Options_fwd.hpp"
#include "../options/OptionValues_fwd.hpp"
#include "../random/Random_fwd.hpp"
#include "../resource/Resources_fwd.hpp"
#include "../system/UserLookup_fwd.hpp"
#include "../unit/ExitCode.hpp"

#include <memory>

namespace erbsland::core {

/// The core application framework.
/// You must create a single instance of this class or a derived class in your `main()` method.
/// @note Important Information when using Erbsland Core under Windows in DLLs
/// Be careful when creating a Windows application that static links Erbsland Core into DLLs that are used by
/// the application.
/// These DLLs do not automatically share the same `Application` instance.
/// Create an exported **not inlined** method `initializeMyDll(Application &app)` in each DLL that used Erbsland Core,
/// and call `Application::linkWith(app)` in this method. From `main()` call all these `initialize...()` methods
/// of all DLLs that use Erbsland Core, just after creating the `Application` instance.
/// Do not use `application()` or `Application::instance()` in static initialization in DLLs that link Erbsland Core.
/// @tested{ApplicationEventTest ApplicationOptionsTest ApplicationPartApplicationTest ApplicationTestScopeTest}
class Application {
    friend class impl::ApplicationInstanceManager;
    friend auto application() -> Application &;

public:
    /// Create an application without command line arguments.
    /// Only create one instance, as the first action in your `main()` function.
    Application();
    /// Create an application from UTF-8 encoded command line arguments.
    /// Only create one instance, as the first action in your `main()` function.
    Application(int argc, char *argv[]);
    /// Create an application from wide command line arguments.
    /// Only create one instance, as the first action in your `main()` function.
    Application(int argc, wchar_t *argv[]);

    /// Destroys the application after its lifecycle has completed.
    virtual ~Application();

    // defaults/deletions
    Application(const Application &) = delete;
    auto operator=(const Application &) -> Application & = delete;
    Application(Application &&) = delete;
    auto operator=(Application &&) -> Application & = delete;

public: // methods to enable features
    /// Enable advanced terminal output.
    /// Call this immediately after constructing the Application object.
    /// Either from `main` or as one of the first lines in your custom `initialize()` method.
    /// In this call, a terminal instance is created for the lifetime of this application.
    /// The terminal is automatically shutdown when the program exits, do not call `initialize()` or
    /// `shutdown()` from your user code.
    /// If a tty is detected, `el::stdOut()` and `el::stdErr()` are redirected to the terminal.
    /// Output from `el::stdOut()` always resets the color to the default color while output from
    /// `el::stdErr()` is output in bright red. This is just for convenience to keep inferring output
    /// from these channels readable.
    /// The idea is that you use the terminal instance via `terminal()` for all your application output.
    void enableTerminal();

protected: // customizable methods
    /// First initialization of the application.
    /// This method is called from `run()`, before any other methods are called.
    /// @throws core::ApplicationError to exit the application with a message and exit code.
    virtual void initialize();
    /// Register command line options.
    /// This method is called from `run()`, before `parseCommandLine()` is called.
    /// @throws core::ApplicationError to exit the application with a message and exit code.
    virtual void registerCommandLineOptions(const options::OptionsPtr &options);
    /// Parse all command line options.
    virtual void parseCommandLine();
    /// Execute the main functionality of the application.
    [[nodiscard]] virtual auto main() -> unit::ExitCode;
    /// Cleanup after the application has finished.
    /// Must noch block or throw an exception.
    virtual void cleanup() noexcept;
    /// Create and initialize the terminal instance.
    /// This method is called from `enableTerminal()`.
    /// Only override this method if you need to customize the terminal instance.
    [[nodiscard]] virtual auto createAndInitializeTerminal() -> cterm::TerminalPtr;

public:
    /// Run the default application lifecycle.
    /// This is calling the application methods in the following order:
    /// - `initialize()`
    /// - `registerCommandLineOptions()`
    /// - `parseCommandLine()`
    /// - `main()`
    /// - `cleanup()`
    /// Exceptions thrown from callbacks in the automatically managed main event loop use the same boundary.
    /// If any `err::Exception` is thrown from one of these methods or callbacks, it will be handled by calling
    /// `cleanup()` first, then printing the error to `stdErr()` and exiting the application with an exit code of 1.
    /// In case of an `core::ApplicationError`, the exit code from the exception will be used.
    /// Any non-`err::Exception` propagates out of this method.
    [[nodiscard]] auto run() -> int;
    /// Override the initialize function.
    /// Use this to set a custom initialization function without deriving from `Application`.
    void setInitializeFn(InitializeFn initializeFn);
    /// Override the main function.
    /// Use this to set a custom main function without deriving from `Application`.
    void setMainFn(MainFn mainFn);

public: // command line options
    /// Get the global options configuration.
    [[nodiscard]] auto options() const noexcept -> const options::OptionsPtr &;
    /// Release the option configuration to free memory after startup.
    void releaseOptions() noexcept;
    /// Get mutable application information.
    [[nodiscard]] auto info() noexcept -> ApplicationInfo &;
    /// Get application information.
    [[nodiscard]] auto info() const noexcept -> const ApplicationInfo &;
    /// Get the converted command line arguments.
    [[nodiscard]] auto commandLineArguments() const noexcept -> const CommandLineArguments &;
    /// Get the option values.
    [[nodiscard]] auto optionValues() const noexcept -> const options::OptionValuesPtr &;

public: // application parts
    /// Access the lazily created application-part manager.
    [[nodiscard]] auto partManager() -> ApplicationPartManagerPtr;
    /// Register an application-part class before `run()`.
    /// @tparam T The concrete application-part class.
    template <ApplicationPartClass T>
    void registerPart() {
        partManager()->template registerPart<T>();
    }
    /// Access a prepared application part through its interface.
    /// @tparam T The abstract part interface.
    /// @return The prepared part implementing `T`.
    template <ApplicationPartInterface T>
    [[nodiscard]] auto part() -> std::shared_ptr<T> {
        return partManager()->template part<T>();
    }

public: // random numbers
    /// Get the shared random generator for non-security use.
    [[nodiscard]] auto random() -> random::Random &;
    /// Get the shared secure random generator.
    [[nodiscard]] auto secureRandom() -> random::Random &;

public: // cryptology
    /// Get the shared cryptology configuration, creating it on first use.
    [[nodiscard]] auto cryptologyConfiguration() -> cryptology::CryptologyConfiguration &;

public: // system services
    /// Access the application display texts. The returned pointer is always non-null.
    [[nodiscard]] auto displayText() const -> const i18n::DisplayTextMapConstPtr &;
    /// Replace the application display texts. A null pointer restores the English defaults.
    void setDisplayTextMap(i18n::DisplayTextMapConstPtr displayText);
    /// Get the shared user and group lookup service.
    [[nodiscard]] auto userLookup() -> system::UserLookup &;

public: // compiled resources
    /// Access the lazily initialized compiled-resource manager.
    [[nodiscard]] auto resources() -> const resource::Resources &;

public: // terminal access
    /// Access the application-shared terminal instance.
    /// Must be enabled via `enableTerminal()`.
    [[nodiscard]] auto terminal() const -> const cterm::TerminalPtr &;
    /// Get the style used for application-rendered system output.
    [[nodiscard]] auto systemOutputStyle() const noexcept -> const cterm::TerminalDocumentStyle &;
    /// Set the style used for application-rendered system output.
    void setSystemOutputStyle(cterm::TerminalDocumentStyle style) noexcept;

public: // event system
    /// Access the target interface of the main event loop.
    /// Use these events to post/schedule events and calls that shall run in the main thread.
    [[nodiscard]] auto events() -> event::EventsPtr;
    /// Access the event registry.
    [[nodiscard]] auto eventRegistry() -> event::EventRegistry &;
    /// Create a managed event thread.
    [[nodiscard]] auto createEventThread() -> event::ManagedEventThreadPtr;
    /// Quit the main event loop, and the event loops of all registered threads.
    void quit(unit::ExitCode exitCode = unit::ExitCode::success()) noexcept;

protected: // event system
    /// Access the main event loop.
    /// @note Only access the event loop if you need to manually control it from a derived application class.
    ///     The default implementation of `main()` will enter the loop and wait for events.
    [[nodiscard]] auto eventLoop() -> event::EventLoop &;
    /// Run the main event loop.
    /// Call this from your derived `main()` method to enter the main loop.
    [[nodiscard]] auto runEventLoop() -> unit::ExitCode;

public: // singleton handling.
    /// Get the application instance.
    [[nodiscard]] static auto instance() -> Application &;
    /// Link this application instance with another one.
    /// Only call this method from `initializeMyDll(Application &app)` methods in DLLs that static link
    /// Erbsland Core to synchronize the singleton across DLL borders.
    static auto linkWith(Application &app) -> void;

public: // library version
    /// Get the build-time library version.
    [[nodiscard]] static auto libraryVersion() noexcept -> unit::Version;
    /// Get the build-time library version text.
    [[nodiscard]] static auto libraryVersionText() noexcept -> text::String;

protected: // debugging methods
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
           /// Initialize the application's regular random-number generator.
    virtual void initializeRandom(random::RandomPtr &randomPtr) noexcept;
    /// Initialize the application's cryptographically secure random-number generator.
    virtual void initializeSecureRandom(random::RandomPtr &randomPtr) noexcept;
#endif

private:
    /// Internal constructor.
    /// Only used by the instance manager when creating a temporary application instance.
    explicit Application(impl::ApplicationDataPtr data);
    /// Access the part manager without creating it.
    [[nodiscard]] auto partManagerIfCreated() const noexcept -> ApplicationPartManagerPtr;
    /// Bring a created part manager to a terminal state before application cleanup.
    void stopPartManager();
    /// Quit the application event system after part shutdown.
    void quitEventSystem() noexcept;

private:
    impl::ApplicationDataPtr _data;
};

/// Access the global application instance, creating a default one on first use.
[[nodiscard]] auto application() -> Application &;

}
