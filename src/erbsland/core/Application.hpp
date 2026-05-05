// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationInfo.hpp"
#include "CommandLineArguments.hpp"
#include "MainFn.hpp"

#include "../options/OptionResult.hpp"
#include "../options/Options_fwd.hpp"
#include "../random/Random.hpp"
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

public: // customizable methods
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
    /// Get the shared random generator for non-security use.
    [[nodiscard]] auto random() -> random::Random &;
    /// Get the shared secure random generator.
    [[nodiscard]] auto secureRandom() -> random::Random &;

public: // singleton
    /// Get the currently registered application instance.
    [[nodiscard]] static auto instance() noexcept -> Application *;

protected: // debugging methods
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
    virtual void initializeRandom(random::RandomPtr &randomPtr) noexcept;
    virtual void initializeSecureRandom(random::RandomPtr &randomPtr) noexcept;
#endif

private:
    void registerInstance() noexcept;
    void unregisterInstance() noexcept;

private:
    options::OptionsPtr _options;               ///< The global options configuration.
    options::OptionValuesPtr _optionValues;     ///< The option values after parsing.
    ApplicationInfo _info;                      ///< Application metadata.
    CommandLineArguments _commandLineArguments; ///< Converted command line arguments.
    MainFn _mainFn;                             ///< Lambda-based override of main().
    random::RandomPtr _random;                  ///< Shared fast random generator.
    random::RandomPtr _secureRandom;            ///< Shared secure random generator.
    std::mutex _randomMutex;                    ///< Mutex for lazy random generator creation.

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
