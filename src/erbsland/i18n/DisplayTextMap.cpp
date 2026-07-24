// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DisplayTextMap.hpp"

#include "../text/Literals.hpp"
#include "../text/StringEditor.hpp"
#include "../text/StringSide.hpp"

namespace erbsland::i18n {

using namespace text::literals;
using text::Char;
using text::String;
using text::StringFormat;
using text::StringSide;

DisplayTextMap::DisplayTextMap() {
    addDefaultTexts();
}

auto DisplayTextMap::defaultMap() -> const DisplayTextMapConstPtr & {
    static const auto result = std::make_shared<const DisplayTextMap>();
    return result;
}

auto DisplayTextMap::clone() const -> DisplayTextMapPtr {
    auto lock = std::scoped_lock{_cacheMutex};
    auto result = std::make_shared<DisplayTextMap>();
    result->_sourceTexts = _sourceTexts;
    result->_translator = _translator;
    return result;
}

auto DisplayTextMap::text(const String &key) const -> String {
    auto lock = std::scoped_lock{_cacheMutex};
    if (const auto cached = _textCache.get(key); cached.has_value()) {
        return cached.value();
    }
    const auto sourceText = findSourceText(key);
    auto result = sourceText.value_or(key);
    if (_translator != nullptr && sourceText.has_value()) {
        auto translated = _translator->translate(key, result);
        if (!translated.isEmpty()) {
            result = std::move(translated);
        }
    }
    _textCache.set(key, result);
    return result;
}

auto DisplayTextMap::format(const String &key) const -> StringFormat {
    auto lock = std::scoped_lock{_cacheMutex};
    if (const auto cached = _formatCache.get(key); cached.has_value()) {
        return *cached.value();
    }
    auto sourceText = String{};
    if (const auto cachedText = _textCache.get(key); cachedText.has_value()) {
        sourceText = cachedText.value();
    } else {
        const auto resolvedSourceText = findSourceText(key);
        sourceText = resolvedSourceText.value_or(key);
        if (_translator != nullptr && resolvedSourceText.has_value()) {
            auto translated = _translator->translate(key, sourceText);
            if (!translated.isEmpty()) {
                sourceText = std::move(translated);
            }
        }
        _textCache.set(key, sourceText);
    }
    auto result = StringFormat{sourceText};
    _formatCache.set(key, std::make_shared<const StringFormat>(result));
    return result;
}

auto DisplayTextMap::set(const String &key, String sourceText) -> DisplayTextMap & {
    _sourceTexts.set(key, std::move(sourceText));
    clearCaches();
    return *this;
}

auto DisplayTextMap::remove(const String &key) -> DisplayTextMap & {
    _sourceTexts.remove(key);
    clearCaches();
    return *this;
}

auto DisplayTextMap::setTranslator(DisplayTextTranslatorConstPtr translator) -> DisplayTextMap & {
    _translator = std::move(translator);
    clearCaches();
    return *this;
}

void DisplayTextMap::clearCaches() {
    auto lock = std::scoped_lock{_cacheMutex};
    _textCache.clear();
    _formatCache.clear();
}

auto DisplayTextMap::findSourceText(const String &key) const -> std::optional<String> {
    if (const auto value = _sourceTexts.get(key); value.has_value()) {
        return value.value();
    }
    const auto middleKey = domainKey(key);
    if (middleKey != key) {
        if (const auto value = _sourceTexts.get(middleKey); value.has_value()) {
            return value.value();
        }
    }
    const auto shortKey = finalKey(key);
    if (shortKey != middleKey) {
        if (const auto value = _sourceTexts.get(shortKey); value.has_value()) {
            return value.value();
        }
    }
    return {};
}

auto DisplayTextMap::domainKey(const String &key) -> String {
    const auto firstSeparator = key.find("."_el);
    if (firstSeparator.isNoIndex()) {
        return key;
    }
    auto afterFirstSeparator = firstSeparator;
    key.advance(afterFirstSeparator);
    if (key.find("."_el, afterFirstSeparator).isNoIndex()) {
        return key;
    }
    const auto lastPart = finalKey(key);
    return String::fromJoined({key.slice(StringSide::Front, firstSeparator), "."_el, lastPart});
}

auto DisplayTextMap::finalKey(const String &key) noexcept -> String {
    auto separator = key.indexAt(StringSide::Back);
    while (key.retreat(separator)) {
        if (key.charAt(separator) == Char{U'.'}) {
            auto resultIndex = separator;
            key.advance(resultIndex);
            return key.slice(StringSide::Back, resultIndex);
        }
    }
    return key;
}

void DisplayTextMap::addDefaultTexts() {
    set("UnknownError"_el, "Unknown Error"_el)
        .set("ExceptionWithoutDiagnosticText"_el, "Exception without diagnostic text"_el)
        .set("NoExceptionProvided"_el, "No exception was provided."_el)
        .set("UnknownException"_el, "Unknown exception"_el)
        .set("CauseChainTooDeep"_el, "Cause chain is too deep to display completely."_el)
        .set("CausedByHeading"_el, "Caused By"_el)
        .set("ErrorSourceHeading"_el, "Error Source"_el)
        .set("PathValuesHeading"_el, "Paths"_el)
        .set("PlatformErrorHeading"_el, "Platform Error"_el)
        .set("SourceNameLabel"_el, "Source"_el)
        .set("SourcePathLabel"_el, "Path"_el)
        .set("LineLabel"_el, "Line"_el)
        .set("ColumnLabel"_el, "Column"_el)
        .set("PositionLabel"_el, "Position"_el)
        .set("options.ApplicationNameFallback"_el, "application"_el)
        .set("options.UsageLabel"_el, "Usage"_el)
        .set("options.ModulePlaceholder"_el, "module"_el)
        .set("options.OptionsPlaceholder"_el, "options"_el)
        .set("options.ModulesHeading"_el, "Modules"_el)
        .set("options.OptionsHeading"_el, "Options"_el)
        .set("options.HelpOptionDescription"_el, "Display this help."_el)
        .set("options.VersionOptionDescription"_el, "Display version information."_el)
        .set("options.BooleanPlaceholder"_el, "boolean"_el)
        .set("options.IntegerPlaceholder"_el, "integer"_el)
        .set("options.ValuePlaceholder"_el, "value"_el)
        .set("options.ChoicePlaceholder"_el, "choice"_el)
        .set("options.ChoicesLabel"_el, "Choices"_el)
        .set("options.VersionLabel"_el, "Version"_el)
        .set("options.AuthorLabel"_el, "Author"_el)
        .set("options.LicenseLabel"_el, "License"_el)
        .set("options.ErrorLabel"_el, "Error"_el)
        .set("options.GenericErrorMessage"_el, "Option processing failed"_el)
        .set("options.ModuleLabel"_el, "Module"_el)
        .set("options.OptionLabel"_el, "Option"_el)
        .set("options.ArgumentIndexLabel"_el, "Argument"_el)
        .set("options.CommandLineArgumentsHeading"_el, "Command Line Arguments"_el)
        .set("options.OptionHelpHeading"_el, "Option Help"_el)
        .set("options.OptionGroupHelpHeading"_el, "Option Group Help"_el)
        .set("options.ModuleHelpHeading"_el, "Module Help"_el)
        .set("options.ViewFullHelpHeading"_el, "View Full Help"_el)
        .set("options.ViewFullModuleHelpHeading"_el, "View Full Module Help"_el)
        .set("options.SyntaxErrorTitle"_el, "Command-line syntax error"_el)
        .set("options.UnknownNameTitle"_el, "Unknown command-line name"_el)
        .set("options.UnexpectedValueTypeTitle"_el, "Invalid option value"_el)
        .set("options.ValidationErrorTitle"_el, "Option validation failed"_el)
        .set("options.NotImplementedTitle"_el, "Option is not implemented"_el);
}

}
