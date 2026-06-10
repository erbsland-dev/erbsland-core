// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationData.hpp"

#include "../../cterm/Terminal.hpp"
#include "../../options/OptionManager.hpp"
#include "../../random/Random.hpp"
#include "../../stream/StandardStreams.hpp"

#include <atomic>

namespace erbsland::core::impl {

/// The default storage implementation for the internal application data.
/// @tested{ApplicationTestScopeTest}
class ApplicationDataImpl : public ApplicationData {
public:
    ApplicationDataImpl() : _options{options::Options::create()} {}
    ~ApplicationDataImpl() override = default;

public: // implement ApplicationData
    void setCommandLineArguments(const int argc, char *argv[]) override {
        if (_commandLineArgumentsInitialized) {
            std::terminate(); // Command line arguments are already initialized.
        }
        _commandLineArguments = options::OptionManager::convertCommandLineArguments(argc, argv);
        _commandLineArgumentsInitialized = true;
    }
    void setCommandLineArguments(const int argc, wchar_t *argv[]) override {
        if (_commandLineArgumentsInitialized) {
            std::terminate(); // Command line arguments are already initialized.
        }
        _commandLineArguments = options::OptionManager::convertCommandLineArguments(argc, argv);
        _commandLineArgumentsInitialized = true;
    }
    void cleanupBeforeAppExit() noexcept override {
        try {
            if (_standardStreamRedirect.isActive()) {
                stream::stdOut()->flush();
                stream::stdErr()->flush();
            }
            if (_isTerminalEnabled && _terminal != nullptr) {
                _terminal->restoreScreen();
                _isTerminalEnabled = false;
                _terminal = nullptr; // provoke errors if the terminal is used after application shutdown.
            }
            _standardStreamRedirect.reset();
            _optionRenderer.reset();
        } catch (...) { // NOLINT(*-empty-catch)
            // ignore all exceptions during cleanup as this may be called from the destructor
        }
    }
    [[nodiscard]] auto event() -> EventData & override {
        auto lock = std::scoped_lock{_eventMutex};
        if (_eventData == nullptr) {
            _eventData = std::make_unique<EventData>();
        }
        return *_eventData;
    }

public: // accessors
    [[nodiscard]] auto info() noexcept -> ApplicationInfo & override { return _info; }
    [[nodiscard]] auto info() const noexcept -> const ApplicationInfo & override { return _info; }
    [[nodiscard]] auto commandLineArguments() const noexcept -> const CommandLineArguments & override {
        return _commandLineArguments;
    }
    [[nodiscard]] auto options() noexcept -> options::OptionsPtr & override { return _options; }
    [[nodiscard]] auto options() const noexcept -> const options::OptionsPtr & override { return _options; }
    [[nodiscard]] auto optionValues() noexcept -> options::OptionValuesPtr & override { return _optionValues; }
    [[nodiscard]] auto optionValues() const noexcept -> const options::OptionValuesPtr & override {
        return _optionValues;
    }
    [[nodiscard]] auto optionRenderer() noexcept -> options::OptionRendererPtr & override { return _optionRenderer; }
    [[nodiscard]] auto mainFn() noexcept -> MainFn & override { return _mainFn; }
    [[nodiscard]] auto randomMutex() noexcept -> std::mutex & override { return _randomMutex; }
    [[nodiscard]] auto random() noexcept -> random::RandomPtr & override { return _random; }
    [[nodiscard]] auto secureRandom() noexcept -> random::RandomPtr & override { return _secureRandom; }
    [[nodiscard]] auto isTerminalEnabled() const noexcept -> bool override { return _isTerminalEnabled; }
    void setTerminalEnabled(const bool enabled) noexcept override { _isTerminalEnabled = enabled; }
    [[nodiscard]] auto terminal() noexcept -> cterm::TerminalPtr & override { return _terminal; }
    [[nodiscard]] auto terminal() const noexcept -> const cterm::TerminalPtr & override { return _terminal; }
    [[nodiscard]] auto standardStreamRedirect() noexcept -> stream::StandardStreamRedirect & override {
        return _standardStreamRedirect;
    }

private:
    ApplicationInfo _info;                                     ///< Application metadata.

    std::atomic<bool> _commandLineArgumentsInitialized{false}; ///< If command line arguments are initialized.
    CommandLineArguments _commandLineArguments;                ///< Converted command line arguments.

    options::OptionsPtr _options;                              ///< The global options configuration.
    options::OptionValuesPtr _optionValues;                    ///< The option values after parsing.
    options::OptionRendererPtr _optionRenderer;                ///< Renderer used by the application option manager.

    MainFn _mainFn;                                            ///< Lambda-based override of main().

    std::mutex _randomMutex;                                   ///< Mutex for lazy random generator creation.
    random::RandomPtr _random;                                 ///< Shared fast random generator.
    random::RandomPtr _secureRandom;                           ///< Shared secure random generator.

    bool _isTerminalEnabled{false};                            ///< Flag if the terminal was enabled.
    cterm::TerminalPtr _terminal;                              ///< The terminal instance.

    stream::StandardStreamRedirect _standardStreamRedirect;    ///< Redirects standard streams to the terminal.

    std::mutex _eventMutex;                                    ///< Mutex for lazy event system creation.
    EventDataPtr _eventData;                                   ///< The data for the event system.
};

}
