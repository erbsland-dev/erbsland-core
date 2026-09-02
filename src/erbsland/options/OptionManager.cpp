// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "OptionManager.hpp"

#include "OptionError.hpp"
#include "Options.hpp"
#include "OptionValues.hpp"

#include "impl/OptionDocumentBuilder.hpp"
#include "impl/OptionParser.hpp"

#include "../i18n/DisplayTextMap.hpp"
#include "../stream/StandardStreams.hpp"
#include "../stream/TextOutputStream.hpp"
#include "../text/PlainTextRenderer.hpp"
#include "../text/StringConverter.hpp"
#include "../text/TextDocument.hpp"

#include <utility>

namespace erbsland::options {

using text::PlainTextRenderer;
using text::String;
using text::StringConverter;
using text::TextDocument;

void OptionManager::renderPlainDocument(const TextDocument &document, const stream::TextOutputStreamPtr &output) {
    auto renderer = PlainTextRenderer{document};
    output->writeLine(renderer.build());
    output->flush();
}

OptionManager::OptionManager() : OptionManager{Options::create()} {
}

OptionManager::OptionManager(OptionsPtr options, const i18n::DisplayTextMapConstPtr &displayText) :
    _options{std::move(options)},
    _displayText{displayText != nullptr ? displayText : i18n::DisplayTextMap::defaultMap()} {
    if (_options == nullptr) {
        _options = Options::create();
    }
}

void OptionManager::setDisplayTextMap(i18n::DisplayTextMapConstPtr displayText) noexcept {
    _displayText = displayText != nullptr ? std::move(displayText) : i18n::DisplayTextMap::defaultMap();
}

auto OptionManager::parse(const int argc, char *argv[]) -> OptionResult {
    auto arguments = convertCommandLineArguments(argc, argv);
    return parse(arguments);
}

auto OptionManager::parse(const int argc, wchar_t *argv[]) -> OptionResult {
    auto arguments = convertCommandLineArguments(argc, argv);
    return parse(arguments);
}

auto OptionManager::parse(core::CommandLineArguments &args) -> OptionResult {
    return impl::OptionParser{_options, args, _displayText}.parse();
}

auto OptionManager::parseOrThrow(const int argc, char *argv[]) -> OptionValuesPtr {
    auto arguments = convertCommandLineArguments(argc, argv);
    return parseOrThrow(arguments);
}

auto OptionManager::parseOrThrow(const int argc, wchar_t *argv[]) -> OptionValuesPtr {
    auto arguments = convertCommandLineArguments(argc, argv);
    return parseOrThrow(arguments);
}

auto OptionManager::parseOrThrow(core::CommandLineArguments &args) -> OptionValuesPtr {
    auto result = parse(args);
    const auto &values = result.values();
    if (result.status() == OptionResultStatus::Success) {
        return values;
    }
    if (result.status() == OptionResultStatus::DisplayHelp) {
        if (result.helpName().isEmpty()) {
            displayHelp(values->moduleName());
        } else {
            try {
                displayDetailedHelp(values->moduleName(), result.helpName());
            } catch (const OptionError &error) {
                displayError(error.context());
                throw;
            }
        }
        return {};
    }
    if (result.status() == OptionResultStatus::DisplayModuleOverview) {
        displayModuleOverview();
        return {};
    }
    if (result.status() == OptionResultStatus::DisplayVersion) {
        displayVersion(values->moduleName());
        return {};
    }
    if (result.errorContext().has_value()) {
        displayError(result.errorContext().value());
    }
    if (!result.errorContext().has_value()) {
        throw options::OptionError{}; // fallback
    }
    throw options::OptionError{result.errorContext().value()};
}

void OptionManager::displayHelp(const String &moduleName) const {
    renderPlainDocument(helpDocument(moduleName), stream::stdOut());
}

void OptionManager::displayDetailedHelp(const String &moduleName, const String &helpName) const {
    renderPlainDocument(detailedHelpDocument(moduleName, helpName), stream::stdOut());
}

void OptionManager::displayModuleOverview() const {
    renderPlainDocument(moduleOverviewDocument(), stream::stdOut());
}

void OptionManager::displayVersion(const String &moduleName) const {
    renderPlainDocument(versionDocument(moduleName), stream::stdOut());
}

void OptionManager::displayError(const OptionErrorContext &errorContext) const {
    renderPlainDocument(errorDocument(errorContext), stream::stdErr());
}

auto OptionManager::helpDocument(const String &moduleName) const -> TextDocument {
    return impl::OptionDocumentBuilder{_options, _displayText}.helpDocument(moduleName);
}

auto OptionManager::detailedHelpDocument(const String &moduleName, const String &helpName) const -> TextDocument {
    return impl::OptionDocumentBuilder{_options, _displayText}.detailedHelpDocument(moduleName, helpName);
}

auto OptionManager::moduleOverviewDocument() const -> TextDocument {
    return impl::OptionDocumentBuilder{_options, _displayText}.moduleOverviewDocument();
}

auto OptionManager::versionDocument(const String &moduleName) const -> TextDocument {
    return impl::OptionDocumentBuilder{_options, _displayText}.versionDocument(moduleName);
}

auto OptionManager::errorDocument(const OptionErrorContext &errorContext) const -> TextDocument {
    return impl::OptionDocumentBuilder{_options, _displayText}.errorDocument(errorContext);
}

auto OptionManager::convertCommandLineArguments(const int argc, char *argv[]) -> core::CommandLineArguments {
    auto result = core::CommandLineArguments{};
    if (argc <= 0 || argv == nullptr) {
        return result;
    }
    result.reserve(unit::ItemCount{static_cast<unit::ItemCount::Value>(argc)});
    for (int index = 0; index < argc; ++index) {
        if (argv[index] == nullptr) {
            result.append(String{});
        } else {
            result.append(String{std::string_view{argv[index]}});
        }
    }
    return result;
}

auto OptionManager::convertCommandLineArguments(const int argc, wchar_t *argv[]) -> core::CommandLineArguments {
    auto result = core::CommandLineArguments{};
    if (argc <= 0 || argv == nullptr) {
        return result;
    }
    result.reserve(unit::ItemCount{static_cast<unit::ItemCount::Value>(argc)});
    for (int index = 0; index < argc; ++index) {
        if (argv[index] == nullptr) {
            result.append(String{});
        } else {
            result.append(StringConverter{std::wstring_view{argv[index]}}.toString());
        }
    }
    return result;
}

}
