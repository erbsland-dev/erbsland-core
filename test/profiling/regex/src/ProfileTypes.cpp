// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProfileTypes.hpp"

#include <algorithm>
#include <array>

namespace app::regex {

using namespace el::text::literals;

auto coverageRegistry() -> const std::vector<CoverageDescriptor> & {
    static const auto registry = []() {
        auto result = std::vector<CoverageDescriptor>{};
        constexpr auto stringInputs = std::array{InputKind::StringUtf8, InputKind::StringUtf16, InputKind::StringUtf32};
        constexpr auto allInputs = std::array{
            InputKind::StringUtf8,
            InputKind::StringUtf16,
            InputKind::StringUtf32,
            InputKind::FileUtf8,
            InputKind::FileUtf16,
            InputKind::FileUtf32};
        constexpr auto compileCases =
            std::array{UseCase::Compile, UseCase::LazyCompile, UseCase::LazyFirstUse, UseCase::LazyContendedFirstUse};
        constexpr auto executionCases =
            std::array{UseCase::Match, UseCase::FullMatch, UseCase::FindFirst, UseCase::FindAll, UseCase::CollectAll};
        for (const auto useCase : compileCases) {
            for (const auto input : stringInputs) {
                result.emplace_back(CoverageDescriptor{useCase, input, ReplacementMode::NotApplicable, "RegEx"_el});
            }
        }
        for (const auto useCase : executionCases) {
            for (const auto input : allInputs) {
                result.emplace_back(CoverageDescriptor{useCase, input, ReplacementMode::NotApplicable, "RegEx"_el});
            }
        }
        result.emplace_back(
            CoverageDescriptor{
                UseCase::ReplaceAll,
                InputKind::StringUtf8,
                ReplacementMode::Expression,
                "RegEx::replaceAll(expression)"_el});
        result.emplace_back(
            CoverageDescriptor{
                UseCase::ReplaceAll,
                InputKind::StringUtf8,
                ReplacementMode::Callback,
                "RegEx::replaceAll(callback)"_el});
        return result;
    }();
    return registry;
}

auto isCompatible(const UseCase useCase, const InputKind inputKind, const ReplacementMode replacementMode) noexcept
    -> bool {
    return std::ranges::any_of(coverageRegistry(), [&](const auto &descriptor) {
        return descriptor.useCase == useCase && descriptor.inputKind == inputKind &&
            descriptor.replacementMode == replacementMode;
    });
}

auto isFileInput(const InputKind value) noexcept -> bool {
    return value == InputKind::FileUtf8 || value == InputKind::FileUtf16 || value == InputKind::FileUtf32;
}

auto encodingFor(const InputKind value) -> el::StringEncoding {
    switch (value) {
    case InputKind::FileUtf16:
        return el::StringEncoding::Utf16;
    case InputKind::FileUtf32:
        return el::StringEncoding::Utf32;
    default:
        return el::StringEncoding::Utf8;
    }
}

auto toString(const RunMode value) -> el::String {
    return value == RunMode::Profile ? "profile"_el : "benchmark"_el;
}

auto toString(const UseCase value) -> el::String {
    switch (value) {
    case UseCase::Compile:
        return "compile"_el;
    case UseCase::LazyCompile:
        return "lazy-compile"_el;
    case UseCase::LazyFirstUse:
        return "lazy-first-use"_el;
    case UseCase::LazyContendedFirstUse:
        return "lazy-contended-first-use"_el;
    case UseCase::Match:
        return "match"_el;
    case UseCase::FullMatch:
        return "full-match"_el;
    case UseCase::FindFirst:
        return "find-first"_el;
    case UseCase::FindAll:
        return "find-all"_el;
    case UseCase::CollectAll:
        return "collect-all"_el;
    case UseCase::ReplaceAll:
        return "replace-all"_el;
    }
    return {};
}

auto toString(const InputKind value) -> el::String {
    switch (value) {
    case InputKind::StringUtf8:
        return "string-utf8"_el;
    case InputKind::StringUtf16:
        return "string-utf16"_el;
    case InputKind::StringUtf32:
        return "string-utf32"_el;
    case InputKind::FileUtf8:
        return "file-utf8"_el;
    case InputKind::FileUtf16:
        return "file-utf16"_el;
    case InputKind::FileUtf32:
        return "file-utf32"_el;
    }
    return {};
}

auto toString(const ReplacementMode value) -> el::String {
    switch (value) {
    case ReplacementMode::NotApplicable:
        return "not-applicable"_el;
    case ReplacementMode::Expression:
        return "expression"_el;
    case ReplacementMode::Callback:
        return "callback"_el;
    }
    return {};
}

auto toString(const Backend value) -> el::String {
    return value == Backend::Erbsland ? "erbsland"_el : "std"_el;
}

auto parseUseCase(const el::String &value) -> std::optional<UseCase> {
    for (
        const auto candidate : std::array{
            UseCase::Compile,
            UseCase::LazyCompile,
            UseCase::LazyFirstUse,
            UseCase::LazyContendedFirstUse,
            UseCase::Match,
            UseCase::FullMatch,
            UseCase::FindFirst,
            UseCase::FindAll,
            UseCase::CollectAll,
            UseCase::ReplaceAll}) {
        if (value == toString(candidate)) {
            return candidate;
        }
    }
    return {};
}

auto parseInputKind(const el::String &value) -> std::optional<InputKind> {
    for (
        const auto candidate : std::array{
            InputKind::StringUtf8,
            InputKind::StringUtf16,
            InputKind::StringUtf32,
            InputKind::FileUtf8,
            InputKind::FileUtf16,
            InputKind::FileUtf32}) {
        if (value == toString(candidate)) {
            return candidate;
        }
    }
    return {};
}

auto parseReplacementMode(const el::String &value) -> std::optional<ReplacementMode> {
    if (value == "expression"_el) {
        return ReplacementMode::Expression;
    }
    if (value == "callback"_el) {
        return ReplacementMode::Callback;
    }
    return {};
}

auto parseFlag(const el::String &value) -> std::optional<el::re::Flag> {
    if (value == "ignore-case"_el) {
        return el::re::Flag::IgnoreCase;
    }
    if (value == "multiline"_el) {
        return el::re::Flag::Multiline;
    }
    if (value == "dot-all"_el) {
        return el::re::Flag::DotAll;
    }
    if (value == "ascii"_el) {
        return el::re::Flag::Ascii;
    }
    if (value == "verbose"_el) {
        return el::re::Flag::Verbose;
    }
    if (value == "crlf"_el) {
        return el::re::Flag::CRLF;
    }
    return {};
}

}
