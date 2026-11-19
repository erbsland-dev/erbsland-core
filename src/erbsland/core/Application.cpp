// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Application.hpp"

#include "ApplicationError.hpp"

#include "impl/application_data/ApplicationCryptologyData.hpp"
#include "impl/application_data/ApplicationEventData.hpp"
#include "impl/application_data/ApplicationLifecycleData.hpp"
#include "impl/application_data/ApplicationLogData.hpp"
#include "impl/application_data/ApplicationOptionsData.hpp"
#include "impl/application_data/ApplicationPartsData.hpp"
#include "impl/application_data/ApplicationRandomData.hpp"
#include "impl/application_data/ApplicationResourceData.hpp"
#include "impl/application_data/ApplicationRuntimeData.hpp"
#include "impl/application_data/ApplicationSystemData.hpp"
#include "impl/application_data/ApplicationTerminalData.hpp"
#include "impl/ApplicationData.hpp"
#include "impl/ApplicationInstanceManager.hpp"
#include "impl/LibraryVersion.hpp"

#include "../cryptology/configuration/CryptologyConfiguration.hpp"
#include "../cterm/impl/TerminalDefaults.hpp"
#include "../cterm/Terminal.hpp"
#include "../err/DiagnosticHelper.hpp"
#include "../err/LogicFailure.hpp"
#include "../err/RuntimeError.hpp"
#include "../event/EventLoop.hpp"
#include "../i18n/DisplayText.hpp"
#include "../log/LogManager.hpp"
#include "../log/LogStream.hpp"
#include "../options/OptionError.hpp"
#include "../options/OptionManager.hpp"
#include "../options/OptionModule.hpp"
#include "../random/SecureRandom.hpp"
#include "../random/ThreadSafeFastRandom.hpp"
#include "../stream/StandardStreams.hpp"
#include "../stream/TextOutputStream.hpp"
#include "../system/UserLookup.hpp"

#include <exception>
#include <memory>
#include <utility>

namespace erbsland::core {

using namespace err;
using namespace options;
using namespace unit;
using namespace text::literals;
using namespace event;

Application::Application() {
    _data = impl::ApplicationInstanceManager::instance()->registerUserInstance(this);
}

Application::Application(const int argc, char *argv[]) {
    _data = impl::ApplicationInstanceManager::instance()->registerUserInstance(this);
    _data->options().get()->setCommandLineArguments(argc, argv);
}

Application::Application(const int argc, wchar_t *argv[]) {
    _data = impl::ApplicationInstanceManager::instance()->registerUserInstance(this);
    _data->options().get()->setCommandLineArguments(argc, argv);
}

Application::Application(impl::ApplicationDataPtr data) : _data{std::move(data)} {
}

Application::~Application() {
    if (impl::ApplicationInstanceManager::instance()->unregisterInstance(this)) {
        cleanupBeforeAppExit();
    }
}

auto Application::run() -> int {
    _data->runtime().get()->startRun();
    const auto lifecycleData = _data->lifecycle().getIfExists();
    if (lifecycleData != nullptr) {
        return lifecycleData->run([this]() -> int { return runApplicationLifecycle(); });
    }
    return runApplicationLifecycle();
}

auto Application::runApplicationLifecycle() -> int {
    ExitCode exitCode;
    try {
        initialize();
        if (const auto partsData = _data->parts().getIfExists(); partsData != nullptr) {
            if (const auto manager = partsData->managerIfExists(); manager != nullptr) {
                manager->prepare();
            }
        }
        const auto optionsData = _data->options().get();
        registerCommandLineOptions(optionsData->options());
        if (const auto partsData = _data->parts().getIfExists(); partsData != nullptr) {
            if (const auto manager = partsData->managerIfExists(); manager != nullptr) {
                manager->registerCommandLineOptions(optionsData->options());
            }
        }
        parseCommandLine();
        if (optionsData->optionValues() == nullptr) {
            exitCode = ExitCode::success();
        } else {
            handleCommandLineOptions(optionsData->optionValues());
            const auto partsData = _data->parts().getIfExists();
            const auto manager = partsData != nullptr ? partsData->managerIfExists() : nullptr;
            if (manager != nullptr) {
                manager->handleCommandLineOptions(optionsData->optionValues());
            }
            if (const auto lifecycleData = _data->lifecycle().getIfExists(); lifecycleData != nullptr) {
                if (manager != nullptr) {
                    lifecycleData->reportAutomaticStartupPending();
                } else {
                    lifecycleData->reportAutomaticStartupComplete();
                }
            }
            exitCode = main();
        }
        stopPartManager();
        finishEventThreads();
        cleanup();
    } catch (const RuntimeError &error) {
        shutdownPartsAndEventThreadsInErrorHandler();
        cleanup();
        if (const auto applicationError = dynamic_cast<const ApplicationError *>(&error); applicationError != nullptr) {
            exitCode = applicationError->exitCode();
        } else {
            exitCode = ExitCode::failure();
        }
        _data->terminal().get()->renderSystemOutput(DiagnosticHelper{error}.toDocument());
    } catch ([[maybe_unused]] const LogicFailure &error) {
        if (auto logging = _data->logging().getIfExists()) {
            logStream()->error("LOGIC FAILURE: "_el, error.toString());
        }
        shutdownPartsAndEventThreadsInErrorHandler();
        cleanup();
        throw; // let the application crash with an unhandled exception.
    } catch (...) {
        shutdownPartsAndEventThreadsInErrorHandler();
        cleanup();
        _exitCode = ExitCode::failure();
        throw; // let the application crash with an unhandled exception.
    }
    _exitCode = exitCode;
    return exitCode.toRawValue();
}

void Application::setInitializeFn(InitializeFn initializeFn) {
    _data->runtime().get()->setInitializeFn(std::move(initializeFn));
}

void Application::setMainFn(MainFn mainFn) {
    _data->runtime().get()->setMainFn(std::move(mainFn));
}

auto Application::options() const noexcept -> const OptionsPtr & {
    return _data->options().get()->options();
}

void Application::releaseOptions() noexcept {
    _data->options().get()->releaseOptions();
}

auto Application::info() noexcept -> ApplicationInfo & {
    return _data->runtime().get()->info();
}

auto Application::info() const noexcept -> const ApplicationInfo & {
    return _data->runtime().get()->info();
}

auto Application::commandLineArguments() const noexcept -> const CommandLineArguments & {
    return _data->options().get()->commandLineArguments();
}

auto Application::optionValues() const noexcept -> const OptionValuesPtr & {
    return _data->options().get()->optionValues();
}

auto Application::partManager() -> ApplicationPartManagerPtr {
    return _data->parts().get()->manager(_data->events().get(), _data->lifecycle().getIfExists());
}

void Application::enableTerminal(const cterm::TerminalFlags flags) {
    enableTerminal(cterm::impl::cDefaultTerminalSize, flags);
}

void Application::enableTerminal(const block::Size size, const cterm::TerminalFlags flags) {
    const auto terminalData = _data->terminal().get();
    if (terminalData->isEnabled()) {
        return;
    }
    terminalData->enable(createAndInitializeTerminal(size, flags));
}

void Application::initialize() {
    const auto runtimeData = _data->runtime().get();
    if (runtimeData->initializeFn()) {
        runtimeData->initializeFn()();
    }
}

void Application::registerCommandLineOptions([[maybe_unused]] const OptionsPtr &options) {
    // empty by default
}

void Application::parseCommandLine() {
    const auto optionsData = _data->options().get();
    if (optionsData->options() == nullptr) {
        return;
    }
    optionsData->options()->setApplicationInfo(_data->runtime().get()->info());

    auto manager = OptionManager{optionsData->options()};
    const auto result = manager.parse(optionsData->commandLineArgumentsForParsing());
    optionsData->maskSensitiveCommandLineText(result.sensitiveTextLocations());
    const auto &values = result.values();
    switch (result.status()) {
    case OptionResultStatus::Success:
        optionsData->setOptionValues(values);
        return;
    case OptionResultStatus::DisplayHelp:
        if (result.helpName().isEmpty()) {
            _data->terminal().get()->renderSystemOutput(manager.helpDocument(values->moduleName()));
        } else {
            _data->terminal().get()->renderSystemOutput(
                manager.detailedHelpDocument(values->moduleName(), result.helpName()));
        }
        optionsData->setOptionValues(nullptr);
        return;
    case OptionResultStatus::DisplayModuleOverview:
        _data->terminal().get()->renderSystemOutput(manager.moduleOverviewDocument());
        optionsData->setOptionValues(nullptr);
        return;
    case OptionResultStatus::DisplayVersion:
        _data->terminal().get()->renderSystemOutput(manager.versionDocument(values->moduleName()));
        optionsData->setOptionValues(nullptr);
        return;
    case OptionResultStatus::Error:
        optionsData->setOptionValues(nullptr);
        if (result.errorContext().has_value()) {
            throw OptionError{result.errorContext().value()};
        }
        throw OptionError{};
    }
}

void Application::handleCommandLineOptions([[maybe_unused]] const OptionValuesPtr &values) {
    // do nothing
}

auto Application::main() -> ExitCode {
    const auto optionsData = _data->options().get();
    if (optionsData->optionValues() == nullptr) {
        return ExitCode::success();
    }
    // Check for a main function from a selected command line module.
    const auto &module = optionsData->optionValues()->module();
    if (module != nullptr && module->mainFn()) {
        return module->mainFn()(optionsData->optionValues());
    }
    // Check if a main function was defined.
    const auto runtimeData = _data->runtime().get();
    if (runtimeData->mainFn()) {
        return runtimeData->mainFn()();
    }
    // By default, run the main loop of the application.
    const auto partsData = _data->parts().getIfExists();
    const auto manager = partsData != nullptr ? partsData->managerIfExists() : nullptr;
    if (manager != nullptr && manager->state() == ApplicationPartManagerState::Ready) {
        const auto lifecycleData = _data->lifecycle().getIfExists();
        const auto eventData = _data->events().getIfExists();
        const auto isShutdownRequested = (lifecycleData != nullptr && lifecycleData->isShutdownRequested()) ||
            (eventData != nullptr && eventData->isQuitRequested());
        if (isShutdownRequested) {
            manager->stop();
        } else {
            manager->start();
        }
    }
    return runMainEventLoop();
}

void Application::cleanup() noexcept {
    // empty by default.
}

auto Application::createAndInitializeTerminal(const block::Size size, const cterm::TerminalFlags flags)
    -> cterm::TerminalPtr {
    auto result = std::make_shared<cterm::Terminal>(size, flags);
    result->initializeScreen();
    return result;
}

auto Application::random() -> random::Random & {
    return _data->random().get()->random();
}

auto Application::secureRandom() -> random::Random & {
    return _data->random().get()->secureRandom();
}

auto Application::cryptologyConfiguration() -> cryptology::CryptologyConfiguration & {
    return _data->cryptology().get()->configuration();
}

auto Application::log() -> log::LogManager & {
    const auto terminalData = _data->terminal().get();
    if (!terminalData->isEnabled()) {
        enableTerminal();
    }
    return _data->logging().get()->manager(terminalData->terminal());
}

auto Application::logStream() -> const log::LogStreamPtr & {
    return log().rootStream();
}

void Application::enableLastErrorDump(const LastErrorDumpMode mode) {
    auto &manager = log();
    _data->logging().get()->enableLastErrorDump(manager, mode);
}

auto Application::resources() -> const resource::Resources & {
    return _data->resources().get()->resources();
}

auto Application::userLookup() -> system::UserLookup & {
    return _data->system().get()->userLookup();
}

auto Application::displayText() const -> i18n::DisplayTextPtr {
    return _data->system().get()->displayText();
}

auto Application::terminal() const -> const cterm::TerminalPtr & {
    const auto terminalData = _data->terminal().get();
    if (!terminalData->isEnabled() || terminalData->terminal() == nullptr) {
        throw LogicFailure{"Terminal must be enabled before use."_el};
    }
    return terminalData->terminal();
}

auto Application::systemOutputStyle() const noexcept -> const cterm::TerminalDocumentStyle & {
    return _data->terminal().get()->systemOutputStyle();
}

void Application::setSystemOutputStyle(cterm::TerminalDocumentStyle style) noexcept {
    _data->terminal().get()->setSystemOutputStyle(std::move(style));
}

auto Application::mainEventLoop() -> EventLoop & {
    return _data->events().get()->mainEventLoop();
}

void Application::stopPartManager() {
    const auto partsData = _data->parts().getIfExists();
    if (partsData == nullptr || partsData->managerIfExists() == nullptr) {
        return;
    }
    partsData->stop(*_data->events().get());
}

void Application::finishEventThreads() {
    if (const auto eventData = _data->events().getIfExists(); eventData != nullptr) {
        eventData->finishThreads();
    }
}

void Application::cleanupBeforeAppExit() noexcept {
    const auto terminalData = _data->terminal().getIfExists();
    const auto logData = _data->logging().getIfExists();
    if (logData != nullptr) {
        // Retain this application's registry through cleanup after the user facade has been unregistered.
        logData->cleanup(_exitCode, displayText(), terminalData.get());
    }
    if (terminalData != nullptr) {
        terminalData->cleanup();
    }
    // Standard streams queue writes on I/O workers, even without terminal redirection.
    // Drain both streams before process teardown can abandon their pending output.
    try {
        stream::stdOut()->flush();
    } catch (const RuntimeError &) {}
    try {
        stream::stdErr()->flush();
    } catch (const RuntimeError &) {}
}

void Application::shutdownPartsAndEventThreadsInErrorHandler() noexcept {
    try {
        stopPartManager();
    } catch (const RuntimeError &) { // NOLINT(*-empty-catch)
        // Preserve the original application error.
    }
    try {
        finishEventThreads();
    } catch (const RuntimeError &) { // NOLINT(*-empty-catch)
        // Preserve the original application error.
    }
}

}
