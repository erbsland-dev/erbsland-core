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
#include "../log/ConsoleLogWriter.hpp"
#include "../log/LastErrorsLogWriter.hpp"
#include "../log/LogConfiguration.hpp"
#include "../log/LogManager.hpp"
#include "../log/LogStream.hpp"
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

Application::Application() {
    _data = impl::ApplicationInstanceManager::instance()->registerUserInstance(this);
}

Application::Application(const int argc, char *argv[]) {
    _data = impl::ApplicationInstanceManager::instance()->registerUserInstance(this);
    _data->setCommandLineArguments(argc, argv);
}

Application::Application(const int argc, wchar_t *argv[]) {
    _data = impl::ApplicationInstanceManager::instance()->registerUserInstance(this);
    _data->setCommandLineArguments(argc, argv);
}

Application::Application(impl::ApplicationDataPtr data) : _data{std::move(data)} {
}

Application::~Application() {
    if (impl::ApplicationInstanceManager::instance()->unregisterInstance(this)) {
        _data->cleanupBeforeAppExit(_exitCode);
    }
}

auto Application::run() -> int {
    unit::ExitCode exitCode;
    try {
        initialize();
        if (const auto manager = partManagerIfCreated(); manager != nullptr) {
            manager->prepare();
        }
        registerCommandLineOptions(_data->options());
        if (const auto manager = partManagerIfCreated(); manager != nullptr) {
            manager->registerCommandLineOptions(_data->options());
        }
        parseCommandLine();
        if (_data->optionValues() == nullptr) {
            exitCode = unit::ExitCode::success();
        } else {
            if (const auto manager = partManagerIfCreated(); manager != nullptr) {
                manager->parseCommandLine(_data->optionValues());
            }
            exitCode = main();
        }
        stopPartManager();
        cleanup();
    } catch (const err::Exception &error) {
        try {
            stopPartManager();
        } catch (...) { // NOLINT(*-empty-catch)
            // Preserve the original application error.
        }
        cleanup();
        if (const auto applicationError = dynamic_cast<const core::ApplicationError *>(&error);
            applicationError != nullptr) {
            exitCode = applicationError->exitCode();
        } else {
            exitCode = unit::ExitCode::failure();
        }
        _data->renderSystemOutput(err::DiagnosticHelper{error, _data->displayText()}.toDocument());
    } catch (...) {
        const auto error = std::current_exception();
        try {
            stopPartManager();
        } catch (...) { // NOLINT(*-empty-catch)
            // Preserve the original foreign exception.
        }
        cleanup();
        _exitCode = unit::ExitCode::failure();
        std::rethrow_exception(error);
    }
    _exitCode = exitCode;
    return exitCode.toRawValue();
}

void Application::setInitializeFn(InitializeFn initializeFn) {
    _data->setInitializeFn(std::move(initializeFn));
}

void Application::setMainFn(MainFn mainFn) {
    _data->setMainFn(std::move(mainFn));
}

auto Application::options() const noexcept -> const options::OptionsPtr & {
    return _data->options();
}

void Application::releaseOptions() noexcept {
    _data->setOptions(nullptr);
}

auto Application::info() noexcept -> ApplicationInfo & {
    return _data->info();
}

auto Application::info() const noexcept -> const ApplicationInfo & {
    return _data->info();
}

auto Application::commandLineArguments() const noexcept -> const CommandLineArguments & {
    return _data->commandLineArguments();
}

auto Application::optionValues() const noexcept -> const options::OptionValuesPtr & {
    return _data->optionValues();
}

auto Application::partManager() -> ApplicationPartManagerPtr {
    const auto lock = std::scoped_lock{_data->partManagerMutex()};
    if (_data->partManager() == nullptr) {
        auto manager = ApplicationPartManager::create(events());
        manager->setOwnerStateChangedFn([this](const ApplicationPartManagerState state) -> void {
            if (state == ApplicationPartManagerState::Stopped || state == ApplicationPartManagerState::Failed) {
                quitEventSystem();
            }
        });
        _data->setPartManager(std::move(manager));
    }
    return _data->partManager();
}

void Application::enableTerminal() {
    if (_data->isTerminalEnabled()) {
        return;
    }
    _data->setTerminal(createAndInitializeTerminal());
    if (_data->terminal() == nullptr) {
        return;
    }
    _data->setTerminalEnabled(true);
    if (_data->terminal()->isInteractive()) {
        const auto [output, error] = cterm::TerminalStream::createStandardStreams(_data->terminal());
        _data->setStandardStreamRedirect(stream::redirectStandardStreams(output, error));
    }
}

void Application::initialize() {
    if (_data->initializeFn()) {
        _data->initializeFn()();
    }
}

void Application::registerCommandLineOptions([[maybe_unused]] const options::OptionsPtr &options) {
    // empty by default
}

void Application::parseCommandLine() {
    if (_data->options() == nullptr) {
        return;
    }
    _data->options()->setApplicationInfo(_data->info());

    auto manager = options::OptionManager{_data->options(), _data->displayText()};
    const auto result = manager.parse(_data->commandLineArgumentsForParsing());
    _data->maskSensitiveCommandLineText(result.sensitiveTextLocations());
    const auto &values = result.values();
    switch (result.status()) {
    case options::OptionResultStatus::Success:
        _data->setOptionValues(values);
        return;
    case options::OptionResultStatus::DisplayHelp:
        _data->renderSystemOutput(manager.helpDocument(values->moduleName()));
        _data->setOptionValues(nullptr);
        return;
    case options::OptionResultStatus::DisplayVersion:
        _data->renderSystemOutput(manager.versionDocument(values->moduleName()));
        _data->setOptionValues(nullptr);
        return;
    case options::OptionResultStatus::Error:
        _data->setOptionValues(nullptr);
        if (result.errorContext().has_value()) {
            throw options::OptionError{result.errorContext().value()};
        }
        throw options::OptionError{};
    }
}

auto Application::main() -> unit::ExitCode {
    if (_data->optionValues() == nullptr) {
        return unit::ExitCode::success();
    }
    // Check for a main function from a selected command line module.
    const auto &module = _data->optionValues()->module();
    if (module != nullptr && module->mainFn()) {
        return module->mainFn()(_data->optionValues());
    }
    // Check if a main function was defined.
    if (_data->mainFn()) {
        return _data->mainFn()();
    }
    // By default, run the main loop of the application.
    if (const auto manager = partManagerIfCreated();
        manager != nullptr && manager->state() == ApplicationPartManagerState::Ready) {
        manager->start();
    }
    return runEventLoop();
}

void Application::cleanup() noexcept {
    // empty by default.
}

auto Application::createAndInitializeTerminal() -> cterm::TerminalPtr {
    auto result = std::make_shared<cterm::Terminal>();
    result->setSafeMarginEnabled(false);
    result->initializeScreen();
    return result;
}

auto Application::random() -> random::Random & {
    auto lock = std::scoped_lock{_data->randomMutex()};
    if (_data->random() == nullptr) {
        auto random = random::RandomPtr{};
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
        initializeRandom(random);
#endif
        if (random == nullptr) {
            random = std::make_unique<random::ThreadSafeFastRandom>();
        }
        _data->setRandom(std::move(random));
    }
    return *_data->random();
}

auto Application::secureRandom() -> random::Random & {
    auto lock = std::scoped_lock{_data->randomMutex()};
    if (_data->secureRandom() == nullptr) {
        auto random = random::RandomPtr{};
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
        initializeSecureRandom(random);
#endif
        if (random == nullptr) {
            random = std::make_unique<random::SecureRandom>();
        }
        _data->setSecureRandom(std::move(random));
    }
    return *_data->secureRandom();
}

auto Application::cryptologyConfiguration() -> cryptology::CryptologyConfiguration & {
    return _data->cryptologyConfiguration();
}

auto Application::log() -> log::LogManager & {
    auto lock = std::scoped_lock{_data->logMutex()};
    if (_data->logManager() == nullptr) {
        if (!_data->isTerminalEnabled()) {
            enableTerminal();
        }
        auto manager = log::LogManager::create();
        auto consoleWriter = log::ConsoleLogWriterPtr{};
        auto configuration = log::LogConfiguration{};
        if (_data->terminal() != nullptr) {
            consoleWriter = std::make_shared<log::ConsoleLogWriter>(_data->terminal());
            configuration.addWriter(
                consoleWriter,
                log::LogWriterFilter{
                    log::LogLevels{log::LogLevel::Information, log::LogLevel::Warning, log::LogLevel::Error}});
        }
        manager->setConfiguration(std::move(configuration));
        _data->setLogManager(std::move(manager), std::move(consoleWriter));
    }
    return *_data->logManager();
}

auto Application::logStream() -> const log::LogStreamPtr & {
    return log().rootStream();
}

void Application::enableLastErrorDump(const LastErrorDumpMode mode) {
    auto &manager = log();
    const auto lock = std::scoped_lock{_data->logMutex()};
    _data->setLastErrorDumpMode(mode);
    if (_data->lastErrorsLogWriter() != nullptr) {
        return;
    }
    auto writer = std::make_shared<log::LastErrorsLogWriter>();
    manager.addPersistentWriter(writer, log::LogWriterFilter{log::LogLevel::Error});
    _data->setLastErrorsLogWriter(std::move(writer));
}

auto Application::resources() -> const resource::Resources & {
    return _data->resources();
}

auto Application::userLookup() -> system::UserLookup & {
    auto lock = std::scoped_lock{_data->systemMutex()};
    if (_data->userLookup() == nullptr) {
        _data->setUserLookup(std::make_unique<system::UserLookup>());
    }
    return *_data->userLookup();
}

auto Application::displayText() const -> const i18n::DisplayTextMapConstPtr & {
    auto lock = std::scoped_lock{_data->systemMutex()};
    return _data->displayText();
}

void Application::setDisplayTextMap(i18n::DisplayTextMapConstPtr displayText) {
    auto lock = std::scoped_lock{_data->systemMutex()};
    _data->setDisplayText(std::move(displayText));
}

auto Application::terminal() const -> const cterm::TerminalPtr & {
    if (!_data->isTerminalEnabled() || _data->terminal() == nullptr) {
        throw err::LogicError{"Terminal must be enabled before use."_el};
    }
    return _data->terminal();
}

auto Application::systemOutputStyle() const noexcept -> const cterm::TerminalDocumentStyle & {
    return _data->systemOutputStyle();
}

void Application::setSystemOutputStyle(cterm::TerminalDocumentStyle style) noexcept {
    _data->setSystemOutputStyle(std::move(style));
}

auto Application::eventLoop() -> EventLoop & {
    return *_data->event().eventLoop;
}

}
