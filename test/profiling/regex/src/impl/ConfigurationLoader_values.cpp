// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfigurationLoader.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/ConfErrorCategory.hpp>
#include <erbsland/conf/Integer.hpp>
#include <erbsland/StringFormat.hpp>

#include <algorithm>

namespace app::regex::impl {

void ConfigurationLoader::configError(const el::String &message) {
    throw el::ApplicationError{message};
}

void ConfigurationLoader::configError(const el::String &message, const ValuePtr &value) {
    throw el::conf::ConfError{el::conf::ConfErrorCategory::Validation, message, value->namePath(), value->location()};
}

void ConfigurationLoader::requireKnownKeys(const ValuePtr &value, const std::span<const el::StringLiteral> known) {
    for (const auto &child : *value) {
        const auto childName = child->name().asText();
        if (std::ranges::none_of(known, [&](const auto name) { return childName == name; })) {
            configError(el::StringFormat{"Unknown configuration key '{}'."_el}.build(childName), child);
        }
    }
}

auto ConfigurationLoader::positiveInteger(
    const ValuePtr &value, const el::String &name, const std::uint64_t defaultValue, const bool allowZero)
    -> std::uint64_t {
    if (!value->hasValue(name)) {
        return defaultValue;
    }
    const auto result = value->getIntegerOrThrow(name);
    if (result < 0 || (!allowZero && result == 0)) {
        configError(
            el::StringFormat{"Configuration value '{}' must be {}."_el}.build(
                name, allowZero ? "non-negative"_el : "positive"_el),
            value->valueOrThrow(name));
    }
    return static_cast<std::uint64_t>(result);
}

auto ConfigurationLoader::parseMode(const el::String &value) -> RunMode {
    if (value == "profile"_el) {
        return RunMode::Profile;
    }
    if (value == "benchmark"_el) {
        return RunMode::Benchmark;
    }
    configError(el::StringFormat{"Unsupported run mode '{}'."_el}.build(value));
}

auto ConfigurationLoader::duration(
    const ValuePtr &value, const el::String &name, const std::chrono::nanoseconds defaultValue)
    -> std::chrono::nanoseconds {
    if (!value->hasValue(name)) {
        return defaultValue;
    }
    return value->getCalendarDeltaOrThrow(name).toTimeDeltaOrThrow().toStdNanoseconds();
}

auto ConfigurationLoader::parseUseCases(const ValuePtr &value) -> std::vector<UseCase> {
    auto result = std::vector<UseCase>{};
    for (const auto &name : value->getStringListOrThrow("use_cases"_el, el::StringList{"match"_el})) {
        const auto parsed = parseUseCase(name);
        if (!parsed) {
            configError(el::StringFormat{"Unsupported use case '{}'."_el}.build(name), value);
        }
        result.emplace_back(*parsed);
    }
    return result;
}

auto ConfigurationLoader::inputForEncoding(const el::String &name) -> std::optional<InputKind> {
    if (name == "utf8"_el) {
        return InputKind::FileUtf8;
    }
    if (name == "utf16"_el) {
        return InputKind::FileUtf16;
    }
    if (name == "utf32"_el) {
        return InputKind::FileUtf32;
    }
    return {};
}

auto ConfigurationLoader::parseInputs(const ValuePtr &value, el::StringList &fileEncodings) -> std::vector<InputKind> {
    auto result = std::vector<InputKind>{};
    fileEncodings = value->getStringListOrThrow("file_encodings"_el, el::StringList{"utf8"_el, "utf16"_el, "utf32"_el});
    for (const auto &name : value->getStringListOrThrow("inputs"_el, el::StringList{"string-utf8"_el})) {
        if (name == "file"_el) {
            for (const auto &encoding : fileEncodings) {
                const auto input = inputForEncoding(encoding);
                if (!input) {
                    configError(el::StringFormat{"Unsupported file encoding '{}'."_el}.build(encoding), value);
                }
                result.emplace_back(*input);
            }
            continue;
        }
        const auto parsed = parseInputKind(name);
        if (!parsed) {
            configError(el::StringFormat{"Unsupported input kind '{}'."_el}.build(name), value);
        }
        result.emplace_back(*parsed);
    }
    std::ranges::sort(result, {}, [](const auto input) { return static_cast<std::uint8_t>(input); });
    result.erase(std::ranges::unique(result).begin(), result.end());
    return result;
}

}
