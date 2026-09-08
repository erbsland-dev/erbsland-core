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
#include "../cterm/Terminal.hpp"
#include "../err/DiagnosticHelper.hpp"
#include "../err/LogicError.hpp"
#include "../event/EventLoop.hpp"
#include "../log/LogManager.hpp"
#include "../log/LogStream.hpp"
#include "../options/OptionError.hpp"
#include "../options/OptionManager.hpp"
#include "../options/OptionModule.hpp"
#include "../random/SecureRandom.hpp"
#include "../random/ThreadSafeFastRandom.hpp"
#include "../system/UserLookup.hpp"

#include <exception>
#include <memory>
#include <utility>

namespace erbsland::core {

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
    unit::ExitCode exitCode;
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
            exitCode = unit::ExitCode::success();
        } else {
            const auto partsData = _data->parts().getIfExists();
            const auto manager = partsData != nullptr ? partsData->managerIfExists() : nullptr;
            if (manager != nullptr) {
                manager->parseCommandLine(optionsData->optionValues());
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
        _data->terminal().get()->renderSystemOutput(
            err::DiagnosticHelper{error, _data->system().get()->displayText()}.toDocument());
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
    _data->runtime().get()->setInitializeFn(std::move(initializeFn));
}

void Application::setMainFn(MainFn mainFn) {
    _data->runtime().get()->setMainFn(std::move(mainFn));
}

auto Application::options() const noexcept -> const options::OptionsPtr & {
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

auto Application::optionValues() const noexcept -> const options::OptionValuesPtr & {
    return _data->options().get()->optionValues();
}

auto Application::partManager() -> ApplicationPartManagerPtr {
    return _data->parts().get()->manager(_data->events().get(), _data->lifecycle().getIfExists());
}

void Application::enableTerminal() {
    const auto terminalData = _data->terminal().get();
    if (terminalData->isEnabled()) {
        return;
    }
    terminalData->enable(createAndInitializeTerminal());
}

void Application::initialize() {
    const auto runtimeData = _data->runtime().get();
    if (runtimeData->initializeFn()) {
        runtimeData->initializeFn()();
    }
}

void Application::registerCommandLineOptions([[maybe_unused]] const options::OptionsPtr &options) {
    // empty by default
}

void Application::parseCommandLine() {
    const auto optionsData = _data->options().get();
    if (optionsData->options() == nullptr) {
        return;
    }
    optionsData->options()->setApplicationInfo(_data->runtime().get()->info());

    auto manager = options::OptionManager{optionsData->options(), _data->system().get()->displayText()};
    const auto result = manager.parse(optionsData->commandLineArgumentsForParsing());
    optionsData->maskSensitiveCommandLineText(result.sensitiveTextLocations());
    const auto &values = result.values();
    switch (result.status()) {
    case options::OptionResultStatus::Success:
        optionsData->setOptionValues(values);
        return;
    case options::OptionResultStatus::DisplayHelp:
        if (result.helpName().isEmpty()) {
            _data->terminal().get()->renderSystemOutput(manager.helpDocument(values->moduleName()));
        } else {
            _data->terminal().get()->renderSystemOutput(
                manager.detailedHelpDocument(values->moduleName(), result.helpName()));
        }
        optionsData->setOptionValues(nullptr);
        return;
    case options::OptionResultStatus::DisplayModuleOverview:
        _data->terminal().get()->renderSystemOutput(manager.moduleOverviewDocument());
        optionsData->setOptionValues(nullptr);
        return;
    case options::OptionResultStatus::DisplayVersion:
        _data->terminal().get()->renderSystemOutput(manager.versionDocument(values->moduleName()));
        optionsData->setOptionValues(nullptr);
        return;
    case options::OptionResultStatus::Error:
        optionsData->setOptionValues(nullptr);
        if (result.errorContext().has_value()) {
            throw options::OptionError{result.errorContext().value()};
        }
        throw options::OptionError{};
    }
}

auto Application::main() -> unit::ExitCode {
    const auto optionsData = _data->options().get();
    if (optionsData->optionValues() == nullptr) {
        return unit::ExitCode::success();
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
    return _data->random().get()->random([this]() -> random::RandomPtr {
        auto random = random::RandomPtr{};
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
        initializeRandom(random);
#endif
        if (random == nullptr) {
            random = std::make_unique<random::ThreadSafeFastRandom>();
        }
        return random;
    });
}

auto Application::secureRandom() -> random::Random & {
    return _data->random().get()->secureRandom([this]() -> random::RandomPtr {
        auto random = random::RandomPtr{};
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
        initializeSecureRandom(random);
#endif
        if (random == nullptr) {
            random = std::make_unique<random::SecureRandom>();
        }
        return random;
    });
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

auto Application::displayText() const -> const i18n::DisplayTextMapConstPtr & {
    return _data->system().get()->displayText();
}

void Application::setDisplayTextMap(i18n::DisplayTextMapConstPtr displayText) {
    _data->system().get()->setDisplayText(std::move(displayText));
}

auto Application::terminal() const -> const cterm::TerminalPtr & {
    const auto terminalData = _data->terminal().get();
    if (!terminalData->isEnabled() || terminalData->terminal() == nullptr) {
        throw err::LogicError{"Terminal must be enabled before use."_el};
    }
    return terminalData->terminal();
}

auto Application::systemOutputStyle() const noexcept -> const cterm::TerminalDocumentStyle & {
    return _data->terminal().get()->systemOutputStyle();
}

void Application::setSystemOutputStyle(cterm::TerminalDocumentStyle style) noexcept {
    _data->terminal().get()->setSystemOutputStyle(std::move(style));
}

auto Application::eventLoop() -> EventLoop & {
    return _data->events().get()->eventLoop();
}

void Application::stopPartManager() {
    const auto partsData = _data->parts().getIfExists();
    if (partsData == nullptr || partsData->managerIfExists() == nullptr) {
        return;
    }
    partsData->stop(*_data->events().get());
}

void Application::cleanupBeforeAppExit() noexcept {
    const auto terminalData = _data->terminal().getIfExists();
    const auto logData = _data->logging().getIfExists();
    if (logData != nullptr) {
        logData->cleanup(_exitCode, _data->system().get()->displayText(), terminalData.get());
    }
    if (terminalData != nullptr) {
        terminalData->cleanup();
    }
}

}
