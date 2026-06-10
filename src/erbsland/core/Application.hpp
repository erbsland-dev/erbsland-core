// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationInfo.hpp"
#include "CommandLineArguments.hpp"
#include "MainFn.hpp"

#include "impl/ApplicationData_fwd.hpp"
#include "impl/ApplicationInstanceManager_fwd.hpp"

#include "../cterm/Terminal_fwd.hpp"
#include "../event/EventIdRegistry_fwd.hpp"
#include "../event/EventLoop_fwd.hpp"
#include "../event/EventTarget_fwd.hpp"
#include "../options/Options_fwd.hpp"
#include "../options/OptionValues_fwd.hpp"
#include "../random/Random_fwd.hpp"
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
/// @tested{ApplicationOptionsTest, ApplicationTerminalTest, ApplicationTestScopeTest, RandomApplicationTest}
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

    // defaults
    virtual ~Application();
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
    /// @throws err::ApplicationError to exit the application with a message and exit code.
    virtual void initialize();
    /// Register command line options.
    /// This method is called from `run()`, before `parseCommandLine()` is called.
    /// @throws err::ApplicationError to exit the application with a message and exit code.
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
    /// If any `err::Exception` is thrown from one of these methods, it will be handled by calling
    /// `cleanUp()` first, then printing the error to `stdErr()` and exiting the application with an exit code of 1.
    /// In case of an `err::ApplicationError`, the exit code from the exception will be used.
    /// If any non `err::Exception` is thrown, the application will crash.
    [[nodiscard]] auto run() -> int;
    /// Override the main function.
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

public: // random numbers
    /// Get the shared random generator for non-security use.
    [[nodiscard]] auto random() -> random::Random &;
    /// Get the shared secure random generator.
    [[nodiscard]] auto secureRandom() -> random::Random &;

public: // terminal access
    /// Access the application-shared terminal instance.
    /// Must be enabled via `enableTerminal()`.
    [[nodiscard]] auto terminal() const -> const cterm::TerminalPtr &;

public: // event system
    /// Access the main event loop.
    [[nodiscard]] auto eventLoop() -> event::EventLoop &;
    /// Access the target interface of the main event loop.
    [[nodiscard]] auto eventTarget() -> event::EventTargetPtr;
    /// Access the event registry.
    [[nodiscard]] auto eventRegistry() -> event::EventIdRegistry &;

public: // singleton handling.
    /// Get the application instance.
    [[nodiscard]] static auto instance() -> Application &;
    /// Link this application instance with another one.
    /// Only call this method from `initializeMyDll(Application &app)` methods in DLLs that static link
    /// Erbsland Core to synchronize the singleton across DLL borders.
    static auto linkWith(Application &app) -> void;

public: // library version
    /// Get the build-time library version.
    /// @tested{ApplicationVersionTest}
    [[nodiscard]] static auto libraryVersion() noexcept -> unit::Version;
    /// Get the build-time library version text.
    /// @tested{ApplicationVersionTest}
    [[nodiscard]] static auto libraryVersionText() noexcept -> text::StringView;

protected: // debugging methods
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
    virtual void initializeRandom(random::RandomPtr &randomPtr) noexcept;
    virtual void initializeSecureRandom(random::RandomPtr &randomPtr) noexcept;
#endif

private:
    /// Internal constructor.
    /// Only used by the instance manager when creating a temporary application instance.
    explicit Application(impl::ApplicationDataPtr data);

private:
    impl::ApplicationDataPtr _data;
};

/// Access the global application instance, creating a default one on first use.
/// @tested{ApplicationTestScopeTest, RandomApplicationTest}
[[nodiscard]] auto application() -> Application &;

}
