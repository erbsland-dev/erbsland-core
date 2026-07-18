// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Application.hpp"

#include "ApplicationError.hpp"

#include "impl/ApplicationData.hpp"
#include "impl/ApplicationInstanceManager.hpp"
#include "impl/LibraryVersion.hpp"

#include "../cterm/Terminal.hpp"
#include "../cterm/TerminalStream.hpp"
#include "../err/DiagnosticHelper.hpp"
#include "../event/EventLoop.hpp"
#include "../event/EventLoopErrorAction.hpp"
#include "../event/impl/CurrentEventsScope.hpp"
#include "../event/impl/ManagedEventThread.hpp"
#include "../event/ManagedEventThread.hpp"
#include "../i18n/DisplayTextMap.hpp"
#include "../options/OptionError.hpp"
#include "../options/OptionManager.hpp"
#include "../options/OptionModule.hpp"
#include "../random/SecureRandom.hpp"
#include "../random/ThreadSafeFastRandom.hpp"
#include "../stream/StandardStreams.hpp"
#include "../system/UserLookup.hpp"
#include "../text/TextDocument.hpp"

#include <algorithm>
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
        _data->cleanupBeforeAppExit();
    }
}

auto Application::run() -> int {
    unit::ExitCode exitCode;
    try {
        initialize();
        registerCommandLineOptions(_data->options());
        parseCommandLine();
        if (_data->optionValues() == nullptr) {
            exitCode = unit::ExitCode::success();
        } else {
            exitCode = main();
        }
        cleanup();
    } catch (const err::Exception &error) {
        cleanup();
        if (const auto applicationError = dynamic_cast<const core::ApplicationError *>(&error);
            applicationError != nullptr) {
            exitCode = applicationError->exitCode();
        } else {
            exitCode = unit::ExitCode::failure();
        }
        _data->renderSystemOutput(err::DiagnosticHelper{error, _data->displayText()}.toDocument());
    }
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
    // empty by default
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
    const auto result = manager.parse(_data->commandLineArguments());
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

auto Application::runEventLoop() -> unit::ExitCode {
    auto &eventData = _data->event();
    eventData.eventLoop->setErrorHandler([this]([[maybe_unused]] std::exception_ptr error) -> EventLoopErrorAction {
        quit(unit::ExitCode::failure());
        return EventLoopErrorAction::Stop;
    });
    {
        auto currentEventsScope = event::impl::CurrentEventsScope{eventData.eventLoop};
        eventData.eventLoop->run();
    }
    auto exitCode = unit::ExitCode::success();
    auto eventThreads = std::vector<ManagedEventThreadPtr>{};
    {
        std::scoped_lock lock{eventData.mutex};
        if (eventData.quitExitCodeSet) {
            exitCode = eventData.quitExitCode;
        }
        for (const auto &eventThreadWeakPtr : eventData.eventThreads) {
            if (auto eventThread = eventThreadWeakPtr.lock(); eventThread != nullptr) {
                eventThreads.emplace_back(std::move(eventThread));
            }
        }
    }
    for (const auto &eventThread : eventThreads) {
        eventThread->quit();
    }
    for (const auto &eventThread : eventThreads) {
        eventThread->join();
    }
    return exitCode;
}

auto Application::events() -> EventsPtr {
    return _data->event().eventLoop;
}

auto Application::eventRegistry() -> EventRegistry & {
    return _data->event().eventIdRegistry;
}

auto Application::createEventThread() -> ManagedEventThreadPtr {
    auto result = std::make_shared<event::impl::ManagedEventThread>();
    auto &eventData = _data->event();
    {
        std::scoped_lock lock{eventData.mutex};
        std::erase_if(eventData.eventThreads, [](const ManagedEventThreadWeakPtr &eventThreadWeakPtr) -> bool {
            return eventThreadWeakPtr.expired();
        });
        eventData.eventThreads.emplace_back(result);
    }
    return result;
}

void Application::quit(unit::ExitCode exitCode) noexcept {
    try {
        auto eventThreads = std::vector<ManagedEventThreadPtr>{};
        auto eventLoop = EventLoopPtr{};
        auto &eventData = _data->event();
        {
            std::scoped_lock lock{eventData.mutex};
            if (!eventData.quitExitCodeSet) {
                eventData.quitExitCode = exitCode;
                eventData.quitExitCodeSet = true;
            }
            eventLoop = eventData.eventLoop;
            std::erase_if(eventData.eventThreads, [](const ManagedEventThreadWeakPtr &eventThreadWeakPtr) -> bool {
                return eventThreadWeakPtr.expired();
            });
            for (const auto &eventThreadWeakPtr : eventData.eventThreads) {
                if (auto eventThread = eventThreadWeakPtr.lock(); eventThread != nullptr) {
                    eventThreads.emplace_back(std::move(eventThread));
                }
            }
        }
        eventLoop->quit();
        for (const auto &eventThread : eventThreads) {
            eventThread->quit();
        }
    } catch (...) { // NOLINT(*-empty-catch)
        // `quit()` must be safe to call from cleanup paths.
    }
}

auto Application::instance() -> Application & {
    return impl::ApplicationInstanceManager::instance()->application();
}

auto Application::linkWith(Application &app) -> void {
    impl::ApplicationInstanceManager::instance()->linkWith(app);
}

auto Application::libraryVersion() noexcept -> unit::Version {
    return impl::libraryVersion();
}

auto Application::libraryVersionText() noexcept -> text::String {
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

auto application() -> Application & {
    return impl::ApplicationInstanceManager::instance()->application();
}

}
