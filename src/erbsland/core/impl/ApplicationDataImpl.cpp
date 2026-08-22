// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationDataImpl.hpp"

#include "EventData.hpp"

#include "../../cryptology/configuration/CryptologyConfiguration.hpp"
#include "../../cterm/BlockStyle.hpp"
#include "../../cterm/Terminal.hpp"
#include "../../cterm/TerminalDocumentRenderer.hpp"
#include "../../i18n/DisplayTextMap.hpp"
#include "../../options/OptionManager.hpp"
#include "../../options/Options.hpp"
#include "../../random/Random.hpp"
#include "../../resource/ResourceManager.hpp"
#include "../../stream/StandardStreams.hpp"
#include "../../stream/TextOutputStream.hpp"
#include "../../system/UserLookup.hpp"
#include "../../text/Literals.hpp"
#include "../../text/PlainTextRenderer.hpp"
#include "../../text/TextDocument.hpp"
#include "../../text/TextNode.hpp"
#include "../../text/TextNodeType.hpp"

#include <exception>
#include <string>
#include <utility>

namespace erbsland::core::impl {

using namespace text::literals;

template <typename Char>
void ApplicationDataImpl::maskNativeArguments(
    const int argumentCount, Char **arguments, const options::OptionSensitiveTextLocations &locations) noexcept {
    if (argumentCount <= 0 || arguments == nullptr) {
        return;
    }
    for (const auto &location : locations) {
        if (location.argumentIndex().isNoIndex() || location.startIndex().isNoIndex()) {
            continue;
        }
        const auto argumentIndex = location.argumentIndex().toSizeT();
        if (argumentIndex >= static_cast<std::size_t>(argumentCount) || arguments[argumentIndex] == nullptr) {
            continue;
        }
        const auto length = std::char_traits<Char>::length(arguments[argumentIndex]);
        const auto startIndex = location.startIndex().toSizeT();
        if (startIndex > length) {
            continue;
        }
        for (auto index = startIndex; index < length; ++index) {
            arguments[argumentIndex][index] = static_cast<Char>('*');
        }
    }
}

ApplicationDataImpl::ApplicationDataImpl() : _options{options::Options::create()} {
}

ApplicationDataImpl::~ApplicationDataImpl() = default;

void ApplicationDataImpl::setCommandLineArguments(const int argc, char *argv[]) {
    if (_commandLineArgumentsInitialized) {
        std::terminate(); // Command line arguments are already initialized.
    }
    _nativeArgumentCount = argc;
    _nativeArguments = argv;
    _nativeWideArguments = nullptr;
    _commandLineArguments = options::OptionManager::convertCommandLineArguments(argc, argv);
    _commandLineArgumentsInitialized = true;
}

void ApplicationDataImpl::setCommandLineArguments(const int argc, wchar_t *argv[]) {
    if (_commandLineArgumentsInitialized) {
        std::terminate(); // Command line arguments are already initialized.
    }
    _nativeArgumentCount = argc;
    _nativeArguments = nullptr;
    _nativeWideArguments = argv;
    _commandLineArguments = options::OptionManager::convertCommandLineArguments(argc, argv);
    _commandLineArgumentsInitialized = true;
}

void ApplicationDataImpl::cleanupBeforeAppExit() noexcept {
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
    } catch (...) { // NOLINT(*-empty-catch)
        // ignore all exceptions during cleanup as this may be called from the destructor
    }
}

auto ApplicationDataImpl::event() -> EventData & {
    auto lock = std::scoped_lock{_eventMutex};
    if (_eventData == nullptr) {
        _eventData = std::make_unique<EventData>();
    }
    return *_eventData;
}

void ApplicationDataImpl::renderSystemOutput(const text::TextDocument &document) {
    if (document.isEmpty()) {
        return;
    }
    if (_isTerminalEnabled && _terminal != nullptr && _terminal->isInteractive()) {
        auto renderer = cterm::TerminalDocumentRenderer{_systemOutputStyle};
        renderer.renderTo(*_terminal, document);
        _terminal->setStyle(cterm::BlockStyle::reset());
        _terminal->flush();
        return;
    }
    const auto output = plainSystemOutputStream(document);
    auto renderer = text::PlainTextRenderer{document};
    output->writeLine(renderer.build());
    output->flush();
}

auto ApplicationDataImpl::info() noexcept -> ApplicationInfo & {
    return _info;
}

auto ApplicationDataImpl::commandLineArguments() const noexcept -> const CommandLineArguments & {
    return _commandLineArguments;
}

auto ApplicationDataImpl::commandLineArgumentsForParsing() noexcept -> CommandLineArguments & {
    return _commandLineArguments;
}

void ApplicationDataImpl::maskSensitiveCommandLineText(
    const options::OptionSensitiveTextLocations &locations) noexcept {
    if (_nativeArguments != nullptr) {
        maskNativeArguments(_nativeArgumentCount, _nativeArguments, locations);
    } else {
        maskNativeArguments(_nativeArgumentCount, _nativeWideArguments, locations);
    }
}

auto ApplicationDataImpl::options() noexcept -> const options::OptionsPtr & {
    return _options;
}

void ApplicationDataImpl::setOptions(options::OptionsPtr options) noexcept {
    _options = std::move(options);
}

auto ApplicationDataImpl::optionValues() noexcept -> const options::OptionValuesPtr & {
    return _optionValues;
}

void ApplicationDataImpl::setOptionValues(options::OptionValuesPtr optionValues) noexcept {
    _optionValues = std::move(optionValues);
}

auto ApplicationDataImpl::systemOutputStyle() const noexcept -> const cterm::TerminalDocumentStyle & {
    return _systemOutputStyle;
}

void ApplicationDataImpl::setSystemOutputStyle(cterm::TerminalDocumentStyle style) noexcept {
    _systemOutputStyle = std::move(style);
}

auto ApplicationDataImpl::initializeFn() noexcept -> const InitializeFn & {
    return _initializeFn;
}

void ApplicationDataImpl::setInitializeFn(InitializeFn initializeFn) noexcept {
    _initializeFn = std::move(initializeFn);
}

auto ApplicationDataImpl::mainFn() noexcept -> const MainFn & {
    return _mainFn;
}

void ApplicationDataImpl::setMainFn(MainFn mainFn) noexcept {
    _mainFn = std::move(mainFn);
}

auto ApplicationDataImpl::partManagerMutex() noexcept -> std::mutex & {
    return _partManagerMutex;
}

auto ApplicationDataImpl::partManager() noexcept -> const ApplicationPartManagerPtr & {
    return _partManager;
}

void ApplicationDataImpl::setPartManager(ApplicationPartManagerPtr manager) noexcept {
    _partManager = std::move(manager);
}

auto ApplicationDataImpl::randomMutex() noexcept -> std::mutex & {
    return _randomMutex;
}

auto ApplicationDataImpl::random() noexcept -> const random::RandomPtr & {
    return _random;
}

void ApplicationDataImpl::setRandom(random::RandomPtr random) noexcept {
    _random = std::move(random);
}

auto ApplicationDataImpl::secureRandom() noexcept -> const random::RandomPtr & {
    return _secureRandom;
}

void ApplicationDataImpl::setSecureRandom(random::RandomPtr random) noexcept {
    _secureRandom = std::move(random);
}

auto ApplicationDataImpl::cryptologyConfiguration() -> cryptology::CryptologyConfiguration & {
    const auto lock = std::scoped_lock{_cryptologyMutex};
    if (_cryptologyConfiguration == nullptr) {
        _cryptologyConfiguration = std::make_unique<cryptology::CryptologyConfiguration>();
    }
    return *_cryptologyConfiguration;
}

auto ApplicationDataImpl::resources() -> const resource::Resources & {
    const auto lock = std::scoped_lock{_resourceMutex};
    if (_resourceManager == nullptr) {
        _resourceManager = std::make_unique<resource::ResourceManager>();
    }
    return *_resourceManager;
}

auto ApplicationDataImpl::systemMutex() noexcept -> std::mutex & {
    return _systemMutex;
}

auto ApplicationDataImpl::displayText() noexcept -> const i18n::DisplayTextMapConstPtr & {
    if (_displayText == nullptr) {
        _displayText = i18n::DisplayTextMap::defaultMap();
    }
    return _displayText;
}

void ApplicationDataImpl::setDisplayText(i18n::DisplayTextMapConstPtr displayText) noexcept {
    if (displayText != nullptr) {
        _displayText = std::move(displayText);
    } else {
        _displayText = i18n::DisplayTextMap::defaultMap();
    }
}

auto ApplicationDataImpl::userLookup() noexcept -> const system::UserLookupPtr & {
    return _userLookup;
}

void ApplicationDataImpl::setUserLookup(system::UserLookupPtr userLookup) noexcept {
    _userLookup = std::move(userLookup);
}

auto ApplicationDataImpl::isTerminalEnabled() const noexcept -> bool {
    return _isTerminalEnabled;
}

void ApplicationDataImpl::setTerminalEnabled(const bool enabled) noexcept {
    _isTerminalEnabled = enabled;
}

auto ApplicationDataImpl::terminal() noexcept -> const cterm::TerminalPtr & {
    return _terminal;
}

void ApplicationDataImpl::setTerminal(cterm::TerminalPtr terminal) noexcept {
    _terminal = std::move(terminal);
}

void ApplicationDataImpl::setStandardStreamRedirect(stream::StandardStreamRedirect redirect) noexcept {
    _standardStreamRedirect = std::move(redirect);
}

auto ApplicationDataImpl::plainSystemOutputStream(const text::TextDocument &document) -> stream::TextOutputStreamPtr {
    const auto root = document.root();
    const auto isErrorDocument = root->style() == "error"_el;
    return isErrorDocument ? stream::stdErr() : stream::stdOut();
}
}
