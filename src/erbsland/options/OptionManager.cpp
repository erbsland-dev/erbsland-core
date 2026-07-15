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

void OptionManager::renderPlainDocument(const text::TextDocument &document, const stream::TextOutputStreamPtr &output) {
    auto renderer = text::PlainTextRenderer{document};
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
    return parse(convertCommandLineArguments(argc, argv));
}

auto OptionManager::parse(const int argc, wchar_t *argv[]) -> OptionResult {
    return parse(convertCommandLineArguments(argc, argv));
}

auto OptionManager::parse(const core::CommandLineArguments &args) -> OptionResult {
    return impl::OptionParser{_options, args, _displayText}.parse();
}

auto OptionManager::parseOrThrow(const int argc, char *argv[]) -> OptionValuesPtr {
    return parseOrThrow(convertCommandLineArguments(argc, argv));
}

auto OptionManager::parseOrThrow(const int argc, wchar_t *argv[]) -> OptionValuesPtr {
    return parseOrThrow(convertCommandLineArguments(argc, argv));
}

auto OptionManager::parseOrThrow(const core::CommandLineArguments &args) -> OptionValuesPtr {
    auto result = parse(args);
    const auto &values = result.values();
    if (result.status() == OptionResultStatus::Success) {
        return values;
    }
    if (result.status() == OptionResultStatus::DisplayHelp) {
        displayHelp(values->moduleName());
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

void OptionManager::displayHelp(text::StringView moduleName) const {
    renderPlainDocument(helpDocument(std::move(moduleName)), stream::stdOut());
}

void OptionManager::displayVersion(text::StringView moduleName) const {
    renderPlainDocument(versionDocument(std::move(moduleName)), stream::stdOut());
}

void OptionManager::displayError(const OptionErrorContext &errorContext) const {
    renderPlainDocument(errorDocument(errorContext), stream::stdErr());
}

auto OptionManager::helpDocument(text::StringView moduleName) const -> text::TextDocument {
    return impl::OptionDocumentBuilder{_options, _displayText}.helpDocument(std::move(moduleName));
}

auto OptionManager::versionDocument(text::StringView moduleName) const -> text::TextDocument {
    return impl::OptionDocumentBuilder{_options, _displayText}.versionDocument(std::move(moduleName));
}

auto OptionManager::errorDocument(const OptionErrorContext &errorContext) const -> text::TextDocument {
    return impl::OptionDocumentBuilder{_options, _displayText}.errorDocument(errorContext);
}

auto OptionManager::convertCommandLineArguments(const int argc, char *argv[]) -> core::CommandLineArguments {
    auto result = core::CommandLineArguments{};
    if (argc <= 0 || argv == nullptr) {
        return result;
    }
    result.reserve(unit::ElementCount{static_cast<unit::ElementCount::Value>(argc)});
    for (int index = 0; index < argc; ++index) {
        if (argv[index] == nullptr) {
            result.append(text::StringView{});
        } else {
            result.append(text::String{std::string_view{argv[index]}});
        }
    }
    return result;
}

auto OptionManager::convertCommandLineArguments(const int argc, wchar_t *argv[]) -> core::CommandLineArguments {
    auto result = core::CommandLineArguments{};
    if (argc <= 0 || argv == nullptr) {
        return result;
    }
    result.reserve(unit::ElementCount{static_cast<unit::ElementCount::Value>(argc)});
    for (int index = 0; index < argc; ++index) {
        if (argv[index] == nullptr) {
            result.append(text::StringView{});
        } else {
            result.append(text::StringConverter{std::wstring_view{argv[index]}}.toString());
        }
    }
    return result;
}

}
