// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfErrorContext.hpp"

#include "Source.hpp"

#include "../text/Literals.hpp"
#include "../text/StringFormat.hpp"

namespace erbsland::conf {

using namespace text::literals;

ConfErrorContext::ConfErrorContext(
    const ConfErrorCategory category,
    text::String title,
    text::String description,
    const SourcePtr &source,
    const Location &location) :
    ConfErrorContext{category, std::move(title), std::move(description)} {
    assignOptional(location);
    if (source != nullptr && _location.has_value()) {
        _codeSnippet = source->codeSnippet(*_location);
    }
}

ConfErrorContext::ConfErrorContext(
    const ConfErrorCategory category, text::String description, const SourcePtr &source, const Location &location) :
    ConfErrorContext{category, defaultTitle(category), std::move(description), source, location} {
}

auto ConfErrorContext::defaultTitle(const ConfErrorCategory category) noexcept -> text::String {
    switch (static_cast<ConfErrorCategory::Enum>(category)) {
    case ConfErrorCategory::IO:
        return "Reading the Configuration Failed"_el;
    case ConfErrorCategory::Encoding:
        return "The Configuration Document Has Invalid Encoding"_el;
    case ConfErrorCategory::UnexpectedEnd:
        return "The Configuration Document Ended Unexpectedly"_el;
    case ConfErrorCategory::Character:
        return "The Configuration Document Contains an Invalid Character"_el;
    case ConfErrorCategory::Syntax:
        return "Parsing the Configuration Failed Because of a Syntax Error"_el;
    case ConfErrorCategory::LimitExceeded:
        return "A Configuration Limit Was Exceeded"_el;
    case ConfErrorCategory::NameConflict:
        return "A Configuration Name Conflicts With an Existing Value"_el;
    case ConfErrorCategory::Indentation:
        return "The Configuration Document Has Invalid Indentation"_el;
    case ConfErrorCategory::Unsupported:
        return "The Configuration Uses an Unsupported Feature"_el;
    case ConfErrorCategory::Signature:
        return "Validating the Configuration Signature Failed"_el;
    case ConfErrorCategory::Access:
        return "Access to a Configuration Source Was Denied"_el;
    case ConfErrorCategory::Validation:
        return "Validating the Configuration Failed"_el;
    case ConfErrorCategory::ValueNotFound:
        return "Accessing a Configuration Value Failed"_el;
    case ConfErrorCategory::TypeMismatch:
        return "Converting a Configuration Value Failed"_el;
    case ConfErrorCategory::Internal:
        return "Internal Configuration Processing Failed"_el;
    }
    return "Configuration Processing Failed"_el;
}

void ConfErrorContext::assignOptional(const Location &location) noexcept {
    if (!location.codeLocation().isUndefined()) {
        _location = location.codeLocation();
    }
    if (location.sourceIdentifier() != nullptr && !location.sourceIdentifier()->path().isEmpty()) {
        _filePath = path::Path{location.sourceIdentifier()->path()};
    }
}

auto ConfErrorContext::withLocation(const Location &location) const -> ConfErrorContext {
    auto result = *this;
    result.assignOptional(location);
    return result;
}

auto ConfErrorContext::withNamePathAndLocation(const NamePath &namePath, const Location &location) const
    -> ConfErrorContext {
    auto result = withLocation(location);
    result._namePath = namePath;
    return result;
}

auto ConfErrorContext::withDescriptionPrefix(const text::String &prefix) const -> ConfErrorContext {
    auto result = *this;
    result._description = text::StringFormat{"{}{}"_el}.build(prefix, _description);
    return result;
}

auto ConfErrorContext::withDescription(text::String description) const -> ConfErrorContext {
    auto result = *this;
    result._description = std::move(description);
    return result;
}

auto ConfErrorContext::withCodeSnippet(const std::optional<text::CodeSnippet> &codeSnippet) const -> ConfErrorContext {
    if (!codeSnippet.has_value()) {
        return *this;
    }
    auto result = *this;
    result._codeSnippet = codeSnippet;
    return result;
}

}
