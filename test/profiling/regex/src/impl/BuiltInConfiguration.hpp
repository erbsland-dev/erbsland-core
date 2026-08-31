// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BuiltInCatalog.hpp"
#include "BuiltInConfiguration_fwd.hpp"

#include <algorithm>
#include <array>

namespace app::regex::impl {

using namespace el::text::literals;

/// Provides the deterministic built-in profiling suites.
/// @notest{Covered by regex profiler CTest entries.}
class BuiltInConfiguration final {
public:
    /// Create the requested deterministic profiling suite.
    [[nodiscard]] static auto suite(const RunSettings &run) -> std::vector<Scenario> {
        auto result = std::vector<Scenario>{};
        if (run.suite == "smoke"_el || run.suite == "coverage"_el) {
            addCoverageSuite(result, run, run.suite);
            return result;
        }
        if (run.suite == "comparison"_el) {
            addComparisonSuite(result, run);
            return result;
        }
        if (run.suite != "snapshot"_el) {
            configError(el::StringFormat{"Unsupported built-in suite '{}'."_el}.build(run.suite));
        }
        addCoverageSuite(result, run, "api-coverage"_el);
        addCompilerStress(result, run);
        addRuntimeStress(result, run);
        addRealWorld(result, run);
        addReplacementStress(result, run);
        return result;
    }

private:
    /// Raise a configuration error with the given message.
    [[noreturn]] static void configError(const el::String &message) { throw el::ApplicationError{message}; }

    /// Create a string consisting of a repeated character.
    [[nodiscard]] static auto repeatCharacter(const el::Char character, const std::size_t count) -> el::String {
        return el::String::fromCharacter(character, el::CpLength::fromSizeTOrThrow(count));
    }

    /// Create a pattern with the requested number of alternatives.
    [[nodiscard]] static auto alternativePattern(const std::size_t count) -> el::String {
        auto result = el::StringEditor{"(?:"_el};
        for (auto index = std::size_t{}; index < count; ++index) {
            if (index != 0U) {
                result.append("|"_el);
            }
            result.append(el::StringFormat{"alt{:04}"_el}.build(index));
        }
        result.append(")"_el);
        return result;
    }

    /// Create a pattern with the requested nesting depth.
    [[nodiscard]] static auto nestedPattern(const std::size_t depth) -> el::String {
        auto result = el::StringEditor{};
        for (auto index = std::size_t{}; index < depth; ++index) {
            result.append("(?:"_el);
        }
        result.append("a"_el);
        for (auto index = std::size_t{}; index < depth; ++index) {
            result.append(")"_el);
        }
        return result;
    }

    /// Create a pattern with the requested number of captures.
    [[nodiscard]] static auto capturePattern(const std::size_t count) -> el::String {
        auto result = el::StringEditor{};
        for (auto index = std::size_t{}; index < count; ++index) {
            result.append("(a)"_el);
        }
        return result;
    }

    /// Create a pattern with the requested number of counters.
    [[nodiscard]] static auto counterPattern(const std::size_t count) -> el::String {
        auto result = el::StringEditor{};
        for (auto index = std::size_t{}; index < count; ++index) {
            result.append("a{1,2}"_el);
        }
        return result;
    }

    /// Create a character-class pattern with the requested number of ranges.
    [[nodiscard]] static auto classPattern(const std::size_t count) -> el::String {
        auto result = el::StringEditor{"["_el};
        for (auto index = std::size_t{}; index < count; ++index) {
            const auto start = static_cast<std::uint32_t>(0x100U + index * 2U);
            result.append(el::StringFormat{"\\u{{{:x}}}-\\u{{{:x}}}"_el}.build(start, start + 1U));
        }
        result.append("]"_el);
        return result;
    }

public:
    /// Add one profiling scenario to the target collection.
    static void addScenario(
        std::vector<Scenario> &target,
        const el::String &group,
        const UseCase useCase,
        const InputKind input,
        const el::String &patternName,
        const el::String &patternValue,
        const el::String &corpusName,
        const CorpusSource corpusSource,
        const el::String &subject,
        const el::String &sourceFile,
        const ReplacementMode replacementMode = ReplacementMode::NotApplicable,
        const el::String &replacement = "{0}"_el,
        const el::re::Flags flags = {},
        const std::chrono::milliseconds timeout = std::chrono::seconds{30},
        const std::uint32_t weight = 1U,
        const std::uint32_t repetitionCount = 1U) {
        if (!isCompatible(useCase, input, replacementMode)) {
            configError(
                el::StringFormat{"Incompatible use-case '{}', input '{}', and replacement mode '{}'."_el}.build(
                    toString(useCase), toString(input), toString(replacementMode)));
        }
        const auto id = el::StringFormat{"{}:{}:{}:{}:{}:{}"_el}.build(
            group, toString(useCase), toString(input), patternName, corpusName, toString(replacementMode));
        target.emplace_back(
            Scenario{
                .id = id,
                .group = group,
                .useCase = useCase,
                .inputKind = input,
                .replacementMode = replacementMode,
                .patternName = patternName,
                .pattern = patternValue,
                .flags = flags,
                .corpusName = corpusName,
                .corpusSource = corpusSource,
                .subject = subject,
                .sourceFile = sourceFile,
                .repetitionCount = repetitionCount,
                .replacement = replacement,
                .timeout = timeout,
                .weight = weight,
                .backend = Backend::Erbsland,
                .comparisonName = {}});
    }

private:
    /// Add the scenarios that cover the public regular-expression API.
    static void addCoverageSuite(std::vector<Scenario> &target, const RunSettings &run, const el::String &group) {
        const auto patternValue = el::String{"a"_el};
        const auto subject = el::String{"a a"_el};
        for (const auto &descriptor : coverageRegistry()) {
            addScenario(
                target,
                group,
                descriptor.useCase,
                descriptor.inputKind,
                "literal-a"_el,
                patternValue,
                "micro-match"_el,
                CorpusSource::Inline,
                subject,
                {},
                descriptor.replacementMode,
                "<{0}>"_el,
                {},
                run.regexTimeout);
        }
    }

    /// Add a pair of equivalent Erbsland Core and standard-library scenarios.
    static void addComparisonPair(
        std::vector<Scenario> &target,
        const RunSettings &run,
        const el::String &name,
        const UseCase useCase,
        const el::String &pattern,
        const el::String &subject,
        const std::uint32_t repetitionCount = 1U) {
        addScenario(
            target,
            "comparison"_el,
            useCase,
            InputKind::StringUtf8,
            name,
            pattern,
            name,
            CorpusSource::Inline,
            subject,
            {},
            ReplacementMode::NotApplicable,
            "{0}"_el,
            {},
            run.regexTimeout,
            1U,
            repetitionCount);
        auto &erbslandScenario = target.back();
        erbslandScenario.id = el::StringFormat{"comparison:{}:erbsland"_el}.build(name);
        erbslandScenario.comparisonName = name;
        auto standardScenario = erbslandScenario;
        standardScenario.id = el::StringFormat{"comparison:{}:std"_el}.build(name);
        standardScenario.backend = Backend::Standard;
        target.emplace_back(std::move(standardScenario));
    }

    /// Add the scenarios that compare Erbsland Core and standard-library regular expressions.
    static void addComparisonSuite(std::vector<Scenario> &target, const RunSettings &run) {
        addComparisonPair(
            target, run, "compile-alternatives-8"_el, UseCase::Compile, alternativePattern(8U), "alt0000"_el);
        addComparisonPair(
            target,
            run,
            "compile-http-request"_el,
            UseCase::Compile,
            R"((?:GET|POST|PUT|DELETE) /[a-zA-Z0-9/_\-]+ HTTP/1\.[01])"_el,
            "GET /index HTTP/1.1"_el);
        addComparisonPair(target, run, "full-match-4k"_el, UseCase::FullMatch, "a+"_el, repeatCharacter(U'a', 4096U));
        addComparisonPair(
            target, run, "find-first-miss-64k"_el, UseCase::FindFirst, "z"_el, repeatCharacter(U'a', 65536U));
        addComparisonPair(
            target, run, "find-all-words"_el, UseCase::FindAll, R"(\b[a-zA-Z]+\b)"_el, "alpha beta gamma "_el, 4096U);
    }

    /// Add scenarios that stress regular-expression compilation.
    static void addCompilerStress(std::vector<Scenario> &target, const RunSettings &run) {
        for (const auto length : std::array{3U, 4U, 255U, 256U, 4096U}) {
            addScenario(
                target,
                "compiler-boundary"_el,
                UseCase::Compile,
                InputKind::StringUtf8,
                el::StringFormat{"literal-{}"_el}.build(length),
                repeatCharacter(U'a', length),
                "micro-match"_el,
                CorpusSource::Inline,
                "a"_el,
                {},
                ReplacementMode::NotApplicable,
                "{0}"_el,
                {},
                run.regexTimeout);
        }
        for (const auto width : std::array{2U, 16U, 128U, 512U}) {
            addScenario(
                target,
                "compiler-alternatives"_el,
                UseCase::Compile,
                InputKind::StringUtf8,
                el::StringFormat{"alternatives-{}"_el}.build(width),
                alternativePattern(width),
                "micro-match"_el,
                CorpusSource::Inline,
                "alt0000"_el,
                {},
                ReplacementMode::NotApplicable,
                "{0}"_el,
                {},
                run.regexTimeout);
        }
        for (const auto depth : std::array{1U, 16U, 64U}) {
            addScenario(
                target,
                "compiler-nesting"_el,
                UseCase::Compile,
                InputKind::StringUtf8,
                el::StringFormat{"nesting-{}"_el}.build(depth),
                nestedPattern(depth),
                "micro-match"_el,
                CorpusSource::Inline,
                "a"_el,
                {},
                ReplacementMode::NotApplicable,
                "{0}"_el,
                {},
                run.regexTimeout);
        }
        for (const auto count : std::array{0U, 1U, 2U, 4U, 8U, 16U, 32U, 64U, 100U}) {
            const auto patternValue = count == 0U ? el::String{"a"_el} : capturePattern(count);
            const auto subject = repeatCharacter(U'a', std::max(1U, count));
            addScenario(
                target,
                "compiler-captures"_el,
                UseCase::Compile,
                InputKind::StringUtf8,
                el::StringFormat{"captures-{}"_el}.build(count),
                patternValue,
                "capture-subject"_el,
                CorpusSource::Inline,
                subject,
                {},
                ReplacementMode::NotApplicable,
                "{0}"_el,
                {},
                run.regexTimeout);
        }
        for (const auto count : std::array{4U, 32U, 128U}) {
            addScenario(
                target,
                "compiler-classes"_el,
                UseCase::Compile,
                InputKind::StringUtf8,
                el::StringFormat{"class-ranges-{}"_el}.build(count),
                classPattern(count),
                "micro-match"_el,
                CorpusSource::Inline,
                "a"_el,
                {},
                ReplacementMode::NotApplicable,
                "{0}"_el,
                {},
                run.regexTimeout);
        }
        for (const auto count : std::array{1U, 8U, 15U}) {
            addScenario(
                target,
                "compiler-counters"_el,
                UseCase::Compile,
                InputKind::StringUtf8,
                el::StringFormat{"counters-{}"_el}.build(count),
                counterPattern(count),
                "counter-subject"_el,
                CorpusSource::Inline,
                repeatCharacter(U'a', count),
                {},
                ReplacementMode::NotApplicable,
                "{0}"_el,
                {},
                run.regexTimeout);
        }
        for (const auto input : std::array{InputKind::StringUtf8, InputKind::StringUtf16, InputKind::StringUtf32}) {
            addScenario(
                target,
                "compiler-representation"_el,
                UseCase::Compile,
                input,
                "literal-256"_el,
                repeatCharacter(U'a', 256U),
                "micro-match"_el,
                CorpusSource::Inline,
                "a"_el,
                {},
                ReplacementMode::NotApplicable,
                "{0}"_el,
                {},
                run.regexTimeout);
        }
    }

    /// Add scenarios that stress regular-expression execution.
    static void addRuntimeStress(std::vector<Scenario> &target, const RunSettings &run) {
        struct Stress {
            el::String name;
            el::String pattern;
            el::String corpus;
            UseCase useCase;
            el::re::Flags flags;
        };
        const auto stresses = std::vector<Stress>{
            {"empty-success"_el, ""_el, "empty"_el, UseCase::FullMatch, {}},
            {"empty-failure"_el, "a"_el, "empty"_el, UseCase::FullMatch, {}},
            {"prefix-success"_el, "alpha"_el, "micro-match"_el, UseCase::Match, {}},
            {"prefix-near-miss"_el, "a+b"_el, "adversarial-64k"_el, UseCase::Match, {}},
            {"full-success"_el, "a+"_el, "dense-64k"_el, UseCase::FullMatch, {}},
            {"full-trailing-failure"_el, "a+"_el, "adversarial-64k"_el, UseCase::FullMatch, {}},
            {"find-start"_el, "a"_el, "dense-64k"_el, UseCase::FindFirst, {}},
            {"find-middle"_el, "b"_el, "middle-64k"_el, UseCase::FindFirst, {}},
            {"find-end"_el, "b"_el, "late-64k"_el, UseCase::FindFirst, {}},
            {"find-none"_el, "z"_el, "dense-64k"_el, UseCase::FindFirst, {}},
            {"sparse-enumeration"_el, "b"_el, "sparse-1m"_el, UseCase::FindAll, {}},
            {"dense-enumeration"_el, "a"_el, "dense-64k"_el, UseCase::FindAll, {}},
            {"dense-collection"_el, "a"_el, "dense-4k"_el, UseCase::CollectAll, {}},
            {"zero-width"_el, ""_el, "dense-4k"_el, UseCase::FindAll, {}},
            {"overlap-success"_el, "(?:a|aa|aaa|aaaa)+b"_el, "overlap-success"_el, UseCase::FullMatch, {}},
            {"overlap-miss"_el, "(?:a|aa|aaa|aaaa)+b"_el, "overlap-miss"_el, UseCase::FullMatch, {}},
            {"ambiguous-success"_el, "(?:a+)+b"_el, "overlap-success"_el, UseCase::FullMatch, {}},
            {"disjoint-alternatives"_el, "(?:alpha|beta|gamma)"_el, "micro-match"_el, UseCase::FindAll, {}},
            {"greedy"_el, "a*b"_el, "micro-match"_el, UseCase::FindFirst, {}},
            {"lazy"_el, "a*?b"_el, "micro-match"_el, UseCase::FindFirst, {}},
            {"possessive"_el, "a*+b"_el, "micro-match"_el, UseCase::FindFirst, {}},
            {"atomic"_el, "(?>a|aa)+b"_el, "micro-match"_el, UseCase::FindFirst, {}},
            {"bounded-counter"_el, "a{3,8}b"_el, "micro-match"_el, UseCase::FindFirst, {}},
            {"unicode-category"_el, R"(\w+)"_el, "unicode-4k"_el, UseCase::FindAll, {}},
            {"unicode-ignore-case"_el, "grüezi"_el, "unicode-4k"_el, UseCase::FindAll, el::re::Flag::IgnoreCase},
            {"ascii-category"_el, R"(\w+)"_el, "unicode-4k"_el, UseCase::FindAll, el::re::Flag::Ascii},
            {"multiline-crlf"_el,
                "^gamma"_el,
                "crlf-4k"_el,
                UseCase::FindAll,
                el::re::Flags{el::re::Flag::Multiline, el::re::Flag::CRLF}},
        };
        for (const auto &stress : stresses) {
            const auto subject = built_in_catalog::generatedCorpus(stress.corpus).value();
            addScenario(
                target,
                "engine-stress"_el,
                stress.useCase,
                InputKind::StringUtf8,
                stress.name,
                stress.pattern,
                stress.corpus,
                CorpusSource::Inline,
                subject,
                {},
                ReplacementMode::NotApplicable,
                "{0}"_el,
                stress.flags,
                run.regexTimeout);
        }
        addScenario(
            target,
            "engine-stress"_el,
            UseCase::FullMatch,
            InputKind::StringUtf8,
            "ambiguous-miss"_el,
            "(?:a+)+b"_el,
            "ambiguous-miss"_el,
            CorpusSource::Inline,
            built_in_catalog::generatedCorpus("ambiguous-miss"_el).value(),
            {},
            ReplacementMode::NotApplicable,
            "{0}"_el,
            {},
            std::chrono::milliseconds{100});
        for (const auto count : std::array{2U, 8U, 32U, 100U}) {
            addScenario(
                target,
                "capture-stress"_el,
                UseCase::FullMatch,
                InputKind::StringUtf8,
                el::StringFormat{"captures-{}"_el}.build(count),
                capturePattern(count),
                "capture-subject"_el,
                CorpusSource::Inline,
                repeatCharacter(U'a', count),
                {},
                ReplacementMode::NotApplicable,
                "{0}"_el,
                {},
                run.regexTimeout);
        }
    }

    /// Add scenarios based on representative real-world expressions.
    static void addRealWorld(std::vector<Scenario> &target, const RunSettings &run) {
        struct RealWorld {
            el::String name;
            el::String corpus;
            UseCase useCase;
            el::re::Flags flags;
        };
        const auto entries = std::vector<RealWorld>{
            {"word"_el, "shakespeare-text"_el, UseCase::FindAll, {}},
            {"capitalized-word"_el, "shakespeare-text"_el, UseCase::CollectAll, {}},
            {"email"_el, "shakespeare-text"_el, UseCase::FindFirst, {}},
            {"url"_el, "shakespeare-text"_el, UseCase::FindAll, {}},
            {"dot-plus"_el,
                "shakespeare-text"_el,
                UseCase::FindAll,
                el::re::Flags{el::re::Flag::DotAll, el::re::Flag::CRLF}},
            {"html-tag"_el, "shakespeare-html"_el, UseCase::FindAll, {}},
            {"toc-capture"_el, "shakespeare-html"_el, UseCase::CollectAll, {}},
            {"toc-possessive"_el, "shakespeare-html"_el, UseCase::FindAll, {}},
            {"markdown-link"_el, "shakespeare-text"_el, UseCase::FindFirst, {}},
        };
        for (const auto &entry : entries) {
            const auto sourceFile = built_in_catalog::file(entry.corpus).value();
            addScenario(
                target,
                "real-world"_el,
                entry.useCase,
                InputKind::StringUtf8,
                entry.name,
                built_in_catalog::pattern(entry.name).value(),
                entry.corpus,
                CorpusSource::File,
                {},
                sourceFile,
                ReplacementMode::NotApplicable,
                "{0}"_el,
                entry.flags,
                run.regexTimeout);
        }
        for (const auto input : std::array{InputKind::FileUtf8, InputKind::FileUtf16, InputKind::FileUtf32}) {
            addScenario(
                target,
                "real-world-file"_el,
                UseCase::FindAll,
                input,
                "toc-capture"_el,
                built_in_catalog::pattern("toc-capture"_el).value(),
                "shakespeare-html"_el,
                CorpusSource::File,
                {},
                built_in_catalog::file("shakespeare-html"_el).value(),
                ReplacementMode::NotApplicable,
                "{0}"_el,
                {},
                run.regexTimeout);
        }
    }

    /// Add scenarios that stress regular-expression replacement.
    static void addReplacementStress(std::vector<Scenario> &target, const RunSettings &run) {
        struct Replacement {
            el::String name;
            el::String pattern;
            el::String subject;
            el::String replacement;
        };
        const auto entries = std::vector<Replacement>{
            {"no-match"_el, "z+"_el, "alpha beta"_el, "x"_el},
            {"deletion"_el, R"(\s+)"_el, "alpha beta gamma"_el, ""_el},
            {"static-expansion"_el, R"(\w+)"_el, "alpha beta gamma"_el, "<static-expanded>"_el},
            {"whole-match"_el, R"(\w+)"_el, "alpha beta gamma"_el, "<{0}>"_el},
            {"group-reorder"_el, "(?<left>[a-z]+)-(?<right>[0-9]+)"_el, "alpha-12 beta-34"_el, "{right}:{left}"_el},
            {"numbered-reorder"_el, "([a-z]+)-([0-9]+)"_el, "alpha-12 beta-34"_el, "{2}:{1}"_el},
        };
        for (const auto &entry : entries) {
            for (const auto mode : std::array{ReplacementMode::Expression, ReplacementMode::Callback}) {
                addScenario(
                    target,
                    "replacement"_el,
                    UseCase::ReplaceAll,
                    InputKind::StringUtf8,
                    entry.name,
                    entry.pattern,
                    "replacement-subject"_el,
                    CorpusSource::Inline,
                    entry.subject,
                    {},
                    mode,
                    entry.replacement,
                    {},
                    run.regexTimeout);
            }
        }
    }
};

}
