// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationInfo.hpp"
#include "CommandLineArguments.hpp"
#include "MainFn.hpp"

#include "../cterm/Terminal_fwd.hpp"
#include "../options/OptionRenderer_fwd.hpp"
#include "../options/OptionResult.hpp"
#include "../options/Options_fwd.hpp"
#include "../random/Random.hpp"
#include "../stream/StandardStreams.hpp"
#include "../unit/ExitCode.hpp"

#include <memory>
#include <mutex>
#include <utility>

namespace erbsland::core {

/// The core application framework.
/// @tested{OptionsFrameworkTest, RandomApplicationTest}
class Application {
    friend auto application() -> Application &;

public:
    /// Create an application without command line arguments.
    Application();
    /// Create an application from UTF-8 encoded command line arguments.
    Application(int argc, char *argv[]);
    /// Create an application from wide command line arguments.
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
    void setMainFn(MainFn mainFn) { _mainFn = std::move(mainFn); }
    /// Get the global options configuration.
    [[nodiscard]] auto options() const noexcept -> const options::OptionsPtr & { return _options; }
    /// Release the option configuration to free memory after startup.
    void releaseOptions() noexcept { _options.reset(); }
    /// Get mutable application information.
    [[nodiscard]] auto info() noexcept -> ApplicationInfo & { return _info; }
    /// Get application information.
    [[nodiscard]] auto info() const noexcept -> const ApplicationInfo & { return _info; }
    /// Get the converted command line arguments.
    [[nodiscard]] auto commandLineArguments() const noexcept -> const CommandLineArguments & {
        return _commandLineArguments;
    }
    /// Get the option values.
    [[nodiscard]] auto optionValues() const noexcept -> const options::OptionValuesPtr & { return _optionValues; }

public: // built-in components
    /// Get the shared random generator for non-security use.
    [[nodiscard]] auto random() -> random::Random &;
    /// Get the shared secure random generator.
    [[nodiscard]] auto secureRandom() -> random::Random &;
    /// Access the application-shared terminal instance.
    /// Must be enabled via `enableTerminal()`.
    [[nodiscard]] auto terminal() const -> const cterm::TerminalPtr &;

public: // singleton
    /// Get the currently registered application instance.
    [[nodiscard]] static auto instance() noexcept -> Application *;

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
    void registerInstance() noexcept;
    void unregisterInstance() noexcept;
    void restoreTerminalIntegration() noexcept;

private:
    options::OptionsPtr _options;                           ///< The global options configuration.
    options::OptionValuesPtr _optionValues;                 ///< The option values after parsing.
    options::OptionRendererPtr _optionRenderer;             ///< Renderer used by the application option manager.
    ApplicationInfo _info;                                  ///< Application metadata.
    CommandLineArguments _commandLineArguments;             ///< Converted command line arguments.
    MainFn _mainFn;                                         ///< Lambda-based override of main().
    random::RandomPtr _random;                              ///< Shared fast random generator.
    random::RandomPtr _secureRandom;                        ///< Shared secure random generator.
    std::mutex _randomMutex;                                ///< Mutex for lazy random generator creation.
    bool _isTerminalEnabled{false};                         ///< Flag if the terminal was enabled.
    cterm::TerminalPtr _terminal;                           ///< The terminal instance.
    stream::StandardStreamRedirect _standardStreamRedirect; ///< Redirects standard streams to the terminal.

private:
    // A raw non-owning pointer is used here because the singleton can refer either to a user-owned stack instance
    // or to the lazily allocated fallback instance owned by _ownedInstance.
    static Application *_instance;
    static std::unique_ptr<Application> _ownedInstance;
    static std::recursive_mutex _instanceMutex;
};

/// Access the global application instance, creating a default one on first use.
/// @tested{OptionsFrameworkTest, RandomApplicationTest}
[[nodiscard]] auto application() -> Application &;

}
