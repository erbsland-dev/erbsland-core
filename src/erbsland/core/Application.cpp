// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Application.hpp"

#include "impl/ApplicationData.hpp"
#include "impl/ApplicationInstanceManager.hpp"
#include "impl/LibraryVersion.hpp"

#include "../cterm/Terminal.hpp"
#include "../cterm/TerminalOptionsRenderer.hpp"
#include "../cterm/TerminalStream.hpp"
#include "../err/ApplicationError.hpp"
#include "../event/EventLoop.hpp"
#include "../options/OptionManager.hpp"
#include "../options/OptionModule.hpp"
#include "../random/SecureRandom.hpp"
#include "../random/ThreadSafeFastRandom.hpp"
#include "../stream/StandardStreams.hpp"

#include <utility>

namespace erbsland::core {

using namespace text::literals;

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
        if (const auto applicationError = dynamic_cast<const err::ApplicationError *>(&error);
            applicationError != nullptr) {
            exitCode = applicationError->exitCode();
        } else {
            exitCode = unit::ExitCode::failure();
        }
        stream::stdErr()->writeLine(error.toString());
        stream::stdErr()->flush();
    }
    return exitCode.toRawValue();
}

void Application::setMainFn(MainFn mainFn) {
    _data->mainFn() = std::move(mainFn);
}

auto Application::options() const noexcept -> const options::OptionsPtr & {
    return _data->options();
}

void Application::releaseOptions() noexcept {
    _data->options().reset();
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
    _data->terminal() = createAndInitializeTerminal();
    if (_data->terminal() == nullptr) {
        return;
    }
    _data->setTerminalEnabled(true);
    if (_data->terminal()->isInteractive()) {
        const auto [output, error] = cterm::TerminalStream::createStandardStreams(_data->terminal());
        _data->standardStreamRedirect() = stream::redirectStandardStreams(output, error);
        _data->optionRenderer() = cterm::TerminalOptionsRenderer::create(_data->terminal());
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

    auto manager = options::OptionManager{_data->options()};
    if (_data->optionRenderer() != nullptr) {
        manager.setRenderer(_data->optionRenderer());
    }
    _data->optionValues() = manager.parseOrThrow(_data->commandLineArguments());
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
    auto lock = std::scoped_lock{_data->randomMutex()};
    if (_data->random() == nullptr) {
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
        initializeRandom(_data->random());
#endif
        if (_data->random() == nullptr) {
            _data->random() = std::make_unique<random::ThreadSafeFastRandom>();
        }
    }
    return *_data->random();
}

auto Application::secureRandom() -> random::Random & {
    auto lock = std::scoped_lock{_data->randomMutex()};
    if (_data->secureRandom() == nullptr) {
#ifdef ERBSLAND_CORE_DEVELOPER_BUILD
        initializeSecureRandom(_data->secureRandom());
#endif
        if (_data->secureRandom() == nullptr) {
            _data->secureRandom() = std::make_unique<random::SecureRandom>();
        }
    }
    return *_data->secureRandom();
}

auto Application::terminal() const -> const cterm::TerminalPtr & {
    if (!_data->isTerminalEnabled() || _data->terminal() == nullptr) {
        // FIXME! That's the wrong exception for this.
        throw err::ApplicationError{"Terminal must be enabled before use."_el};
    }
    return _data->terminal();
}

auto Application::eventLoop() -> event::EventLoop & {
    return *_data->event().eventLoop;
}

auto Application::eventTarget() -> event::EventTargetPtr {
    return _data->event().eventLoop;
}

auto Application::eventRegistry() -> event::EventIdRegistry & {
    return _data->event().eventIdRegistry;
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

auto application() -> Application & {
    return impl::ApplicationInstanceManager::instance()->application();
}

}
