// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Application.hpp"

#include "impl/LibraryVersion.hpp"

#include "../cterm/Terminal.hpp"
#include "../cterm/TerminalOptionsRenderer.hpp"
#include "../cterm/TerminalStream.hpp"
#include "../err/ApplicationError.hpp"
#include "../err/Exception.hpp"
#include "../options/OptionManager.hpp"
#include "../options/OptionModule.hpp"
#include "../options/Options.hpp"
#include "../options/OptionValues.hpp"
#include "../random/SecureRandom.hpp"
#include "../random/ThreadSafeFastRandom.hpp"
#include "../stream/StandardStreams.hpp"

#include <utility>

namespace erbsland::core {

using namespace text::literals;

Application *Application::_instance = nullptr;
std::unique_ptr<Application> Application::_ownedInstance;
std::recursive_mutex Application::_instanceMutex;

Application::Application() : _options{options::Options::create()} {
    registerInstance();
}

Application::Application(const int argc, char *argv[]) :
    _options{options::Options::create()},
    _commandLineArguments{options::OptionManager::convertCommandLineArguments(argc, argv)} {
    registerInstance();
}

Application::Application(const int argc, wchar_t *argv[]) :
    _options{options::Options::create()},
    _commandLineArguments{options::OptionManager::convertCommandLineArguments(argc, argv)} {
    registerInstance();
}

Application::~Application() {
    restoreTerminalIntegration();
    unregisterInstance();
}

auto Application::run() -> int {
    unit::ExitCode exitCode;
    try {
        initialize();
        registerCommandLineOptions(_options);
        parseCommandLine();
        if (_optionValues == nullptr) {
            exitCode = unit::ExitCode::success();
        } else {
            exitCode = main();
        }
        cleanup();
    } catch (const err::Exception &error) {
        cleanup();
        if (const auto applicationError = dynamic_cast<const err::ApplicationError *>(&error);
            applicationError != nullptr) {
            exitCode = applicationError->exitCode();
        } else {
            exitCode = unit::ExitCode::failure();
        }
        stream::stdErr()->writeLine(error.toString());
        stream::stdErr()->flush();
    }
    restoreTerminalIntegration();
    return exitCode.toRawValue();
}

void Application::enableTerminal() {
    if (_isTerminalEnabled) {
        return;
    }
    _terminal = createAndInitializeTerminal();
    if (_terminal == nullptr) {
        return;
    }
    _isTerminalEnabled = true;
    if (_terminal->isInteractive()) {
        const auto [output, error] = cterm::TerminalStream::createStandardStreams(_terminal);
        _standardStreamRedirect = stream::redirectStandardStreams(output, error);
        _optionRenderer = cterm::TerminalOptionsRenderer::create(_terminal);
    }
}

void Application::initialize() {
    // empty by default
}

void Application::registerCommandLineOptions([[maybe_unused]] const options::OptionsPtr &options) {
    // empty by default
}

void Application::parseCommandLine() {
    if (_options == nullptr) {
        return;
    }
    _options->setApplicationInfo(_info);

    auto manager = options::OptionManager{_options};
    if (_optionRenderer != nullptr) {
        manager.setRenderer(_optionRenderer);
    }
    _optionValues = manager.parseOrThrow(_commandLineArguments);
}

auto Application::main() -> unit::ExitCode {
    if (_optionValues == nullptr) {
        return unit::ExitCode::success();
    }
    // Check for a main function from a selected command line module.
    const auto &module = _optionValues->module();
    if (module != nullptr && module->mainFn()) {
        return module->mainFn()(_optionValues);
    }
    // Check if a main function was defined.
    if (_mainFn) {
        return _mainFn();
    }
    return unit::ExitCode::success();
}

void Application::cleanup() noexcept {
    // empty by default.
}

auto Application::createAndInitializeTerminal() -> cterm::TerminalPtr {
    auto result = std::make_shared<cterm::Terminal>();
    result->initializeScreen();
    return result;
}

auto Application::random() -> random::Random & {
    auto lock = std::scoped_lock{_randomMutex};
    if (_random == nullptr) {
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
        initializeRandom(_random);
#endif
        if (_random == nullptr) {
            _random = std::make_unique<random::ThreadSafeFastRandom>();
        }
    }
    return *_random;
}

auto Application::secureRandom() -> random::Random & {
    auto lock = std::scoped_lock{_randomMutex};
    if (_secureRandom == nullptr) {
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
        initializeSecureRandom(_secureRandom);
#endif
        if (_secureRandom == nullptr) {
            _secureRandom = std::make_unique<random::SecureRandom>();
        }
    }
    return *_secureRandom;
}

auto Application::terminal() const -> const cterm::TerminalPtr & {
    if (!_isTerminalEnabled || _terminal == nullptr) {
        // FIXME! That's the wrong exception for this.
        throw err::ApplicationError{"Terminal must be enabled before use."_el};
    }
    return _terminal;
}

auto Application::instance() noexcept -> Application * {
    const auto lock = std::scoped_lock{_instanceMutex};
    return _instance;
}

auto Application::libraryVersion() noexcept -> unit::Version {
    return impl::libraryVersion();
}

auto Application::libraryVersionText() noexcept -> text::StringView {
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

void Application::registerInstance() noexcept {
    const auto lock = std::scoped_lock{_instanceMutex};
    if (_instance == nullptr) {
        _instance = this;
    }
}

void Application::unregisterInstance() noexcept {
    const auto lock = std::scoped_lock{_instanceMutex};
    if (_instance == this) {
        _instance = nullptr;
    }
}

void Application::restoreTerminalIntegration() noexcept {
    if (_standardStreamRedirect.isActive()) {
        try {
            stream::stdOut()->flush();
            stream::stdErr()->flush();
        } catch (...) {
            // ignore during cleanup
        }
    }
    if (_isTerminalEnabled && _terminal != nullptr) {
        _terminal->restoreScreen();
        _isTerminalEnabled = false;
    }
    _standardStreamRedirect.reset();
    _optionRenderer.reset();
}

auto application() -> Application & {
    auto lock = std::scoped_lock{Application::_instanceMutex};
    if (Application::_instance == nullptr) {
        Application::_ownedInstance = std::make_unique<Application>();
    }
    return *Application::_instance;
}

}
