// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Execution.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/EncodingMode.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathReadTextOptions.hpp>
#include <erbsland/re/Feature.hpp>
#include <erbsland/re/Flag.hpp>
#include <erbsland/StringBomMode.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/TextInputStream.hpp>
#include <erbsland/TimeDelta.hpp>

#include <vector>

namespace app::regex::impl {

void Execution::workloadError(const el::String &message) {
    throw el::ApplicationError{message};
}

auto Execution::elapsedNanoseconds(const Clock::time_point start) -> std::int64_t {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
}

auto Execution::mixSeed(std::uint64_t seed, const std::uint64_t value) noexcept -> std::uint64_t {
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
    seed ^= seed >> 30U;
    seed *= 0xbf58476d1ce4e5b9ULL;
    seed ^= seed >> 27U;
    seed *= 0x94d049bb133111ebULL;
    return seed ^ (seed >> 31U);
}

auto Execution::isCompilationCase(const UseCase useCase) noexcept -> bool {
    return useCase == UseCase::Compile || useCase == UseCase::LazyCompile || useCase == UseCase::LazyFirstUse ||
        useCase == UseCase::LazyContendedFirstUse;
}

auto Execution::makePattern(const Scenario &scenario) -> el::AnyString {
    switch (scenario.inputKind) {
    case InputKind::StringUtf16:
        return el::AnyString{el::StringConverter{scenario.pattern}.toU16String()};
    case InputKind::StringUtf32:
        return el::AnyString{el::StringConverter{scenario.pattern}.toU32String()};
    default:
        return el::AnyString{scenario.pattern};
    }
}

auto Execution::preparedPattern() const -> const el::AnyString & {
    return _prepared.pattern;
}

auto Execution::settings(const Scenario &scenario) -> el::re::Settings {
    auto result = el::re::Settings{};
    result.setTimeout(scenario.timeout);
    if (scenario.pattern.isEmpty()) {
        result.enableFeature(el::re::Feature::EmptyGroups);
    }
    return result;
}

auto Execution::compile(const el::AnyString &pattern, const Scenario &scenario) -> el::re::RegExPtr {
    return el::re::RegEx::compile(pattern, scenario.flags, settings(scenario));
}

auto Execution::lazyCompile(const el::AnyString &pattern, const Scenario &scenario) -> el::re::RegExPtr {
    return el::re::RegEx::lazyCompile(pattern, scenario.flags, settings(scenario));
}

auto Execution::standardFlags(const Scenario &scenario) -> std::regex_constants::syntax_option_type {
    auto result = std::regex_constants::ECMAScript;
    if (scenario.flags.isSet(el::re::Flag::IgnoreCase)) {
        result |= std::regex_constants::icase;
    }
    return result;
}

auto Execution::compileStandard(const std::string &pattern, const Scenario &scenario)
    -> std::shared_ptr<const std::regex> {
    return std::make_shared<const std::regex>(pattern, standardFlags(scenario));
}

void Execution::executeStandard(std::uint64_t &sink, std::uint64_t &matches) const {
    const auto &subject = _prepared.standardSubject;
    const auto &expression = *_prepared.standardExpression;
    auto match = std::smatch{};
    switch (_prepared.scenario.useCase) {
    case UseCase::Match:
        if (std::regex_search(subject, match, expression, std::regex_constants::match_continuous)) {
            consumeStandardMatch(&match, subject, sink, matches);
        } else {
            consumeStandardMatch<std::smatch>(nullptr, subject, sink, matches);
        }
        break;
    case UseCase::FullMatch:
        if (std::regex_match(subject, match, expression)) {
            consumeStandardMatch(&match, subject, sink, matches);
        } else {
            consumeStandardMatch<std::smatch>(nullptr, subject, sink, matches);
        }
        break;
    case UseCase::FindFirst:
        if (std::regex_search(subject, match, expression)) {
            consumeStandardMatch(&match, subject, sink, matches);
        } else {
            consumeStandardMatch<std::smatch>(nullptr, subject, sink, matches);
        }
        break;
    case UseCase::FindAll:
    case UseCase::CollectAll:
        for (
            auto iterator = std::sregex_iterator{subject.begin(), subject.end(), expression};
            iterator != std::sregex_iterator{};
            ++iterator) {
            consumeStandardMatch(&*iterator, subject, sink, matches);
        }
        break;
    default:
        workloadError("Unsupported standard-library regex workload path."_el);
    }
}

void Execution::executeOnStream(
    const el::stream::TextInputStreamPtr &stream,
    const el::re::RegExPtr &expression,
    std::uint64_t &sink,
    std::uint64_t &matches) const {
    const auto includePositions = !_prepared.scenario.comparisonName.isEmpty();
    switch (_prepared.scenario.useCase) {
    case UseCase::Match:
        consumeMatch(expression->match(stream), includePositions, sink, matches);
        break;
    case UseCase::FullMatch:
        consumeMatch(expression->fullMatch(stream), includePositions, sink, matches);
        break;
    case UseCase::FindFirst:
        consumeMatch(expression->findFirst(stream), includePositions, sink, matches);
        break;
    case UseCase::FindAll:
        for (const auto &match : expression->findAll(stream)) {
            consumeMatch(match, includePositions, sink, matches);
        }
        break;
    case UseCase::CollectAll:
        for (const auto &match : expression->collectAll(stream)) {
            consumeMatch(match, includePositions, sink, matches);
        }
        break;
    default:
        workloadError("Unsupported stream regex workload path."_el);
    }
}

void Execution::executeReplacement(const el::re::RegExPtr &expression, std::uint64_t &sink) const {
    auto result = el::String{};
    if (_prepared.scenario.replacementMode == ReplacementMode::Expression) {
        result = expression->replaceAll(_prepared.subject8, _prepared.scenario.replacement);
    } else {
        result = expression->replaceAll(
            _prepared.subject8, [](const el::re::MatchPtr &match) -> el::String { return match->content(); });
    }
    sink = mixSeed(sink, result.length().toRawValue());
    sink = mixSeed(sink, result.toHash());
}

void Execution::executeLazyFirstUse(
    const el::re::RegExPtr &expression, std::uint64_t &sink, std::uint64_t &matches) const {
    const auto includePositions = !_prepared.scenario.comparisonName.isEmpty();
    if (_prepared.scenario.inputKind == InputKind::StringUtf16) {
        consumeMatch(expression->fullMatch(_prepared.subject16), includePositions, sink, matches);
    } else if (_prepared.scenario.inputKind == InputKind::StringUtf32) {
        consumeMatch(expression->fullMatch(_prepared.subject32), includePositions, sink, matches);
    } else {
        consumeMatch(expression->fullMatch(_prepared.subject8), includePositions, sink, matches);
    }
}

auto Execution::executeWorker(const std::uint64_t operations, const el::re::RegExPtr &sharedLazy) const
    -> WorkerResult {
    auto sink = std::uint64_t{0x455242534c414e44ULL};
    auto matches = std::uint64_t{};
    auto stream = el::stream::TextInputStreamPtr{};
    if (isFileInput(_prepared.scenario.inputKind)) {
        auto options = el::PathReadTextOptions{encodingFor(_prepared.scenario.inputKind)};
        options.setBomMode(el::StringBomMode::Automatic)
            .setEncodingMode(el::EncodingMode::Strict)
            .setTimeout(el::TimeDelta::seconds(30));
        stream = _prepared.filePath.content().openTextInputStream(options);
    }
    auto lazyExpressions = std::vector<el::re::RegExPtr>{};
    if (_prepared.scenario.useCase == UseCase::LazyFirstUse) {
        lazyExpressions.reserve(static_cast<std::size_t>(operations));
        for (auto operation = std::uint64_t{}; operation < operations; ++operation) {
            lazyExpressions.emplace_back(lazyCompile(preparedPattern(), _prepared.scenario));
        }
    }
    const auto start = Clock::now();
    for (auto operation = std::uint64_t{}; operation < operations; ++operation) {
        if (_prepared.scenario.backend == Backend::Standard) {
            if (_prepared.scenario.useCase == UseCase::Compile) {
                const auto expression = compileStandard(_prepared.standardPattern, _prepared.scenario);
                sink = mixSeed(sink, _prepared.scenario.pattern.characterLength().toRawValue());
            } else {
                executeStandard(sink, matches);
            }
            continue;
        }
        switch (_prepared.scenario.useCase) {
        case UseCase::Compile: {
            const auto expression = compile(preparedPattern(), _prepared.scenario);
            sink = mixSeed(sink, expression->pattern().characterLength().toRawValue());
            break;
        }
        case UseCase::LazyCompile: {
            const auto expression = lazyCompile(preparedPattern(), _prepared.scenario);
            sink = mixSeed(sink, expression->isCompiled() ? 1U : 0U);
            break;
        }
        case UseCase::LazyFirstUse:
            executeLazyFirstUse(lazyExpressions[operation], sink, matches);
            break;
        case UseCase::LazyContendedFirstUse:
            executeLazyFirstUse(sharedLazy, sink, matches);
            break;
        case UseCase::ReplaceAll:
            executeReplacement(_prepared.expression, sink);
            break;
        default:
            if (stream) {
                if (operation != 0U) {
                    if (!stream->setPosition(el::ByteIndex::zero()).isSuccess()) {
                        workloadError("Failed to rewind the profiling input stream."_el);
                    }
                }
                executeOnStream(stream, _prepared.expression, sink, matches);
            } else if (_prepared.scenario.inputKind == InputKind::StringUtf16) {
                executeOnString(_prepared.subject16, _prepared.expression, sink, matches);
            } else if (_prepared.scenario.inputKind == InputKind::StringUtf32) {
                executeOnString(_prepared.subject32, _prepared.expression, sink, matches);
            } else {
                executeOnString(_prepared.subject8, _prepared.expression, sink, matches);
            }
            break;
        }
    }
    const auto elapsed = elapsedNanoseconds(start);
    if (stream) {
        if (!stream->close().isClosed()) {
            workloadError("Failed to close the profiling input stream."_el);
        }
    }
    return WorkerResult{
        .operations = operations,
        .logicalBytes =
            (isCompilationCase(_prepared.scenario.useCase) ? _prepared.scenario.pattern.length().toRawValue()
                                                           : _prepared.subject8.length().toRawValue()) *
            operations,
        .logicalCodePoints =
            (isCompilationCase(_prepared.scenario.useCase) ? _prepared.scenario.pattern.characterLength().toRawValue()
                                                           : _prepared.subject8.characterLength().toRawValue()) *
            operations,
        .matches = matches,
        .nanoseconds = elapsed,
        .sink = sink};
}

}
