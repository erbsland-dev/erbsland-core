// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConfigurationTools.hpp"

#include <erbsland/conf/Parser.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <set>

namespace app::byte::impl {

using namespace el::text::literals;

using ValuePtr = el::conf::ValuePtr;

constexpr auto cArrayExtents = std::array<std::uint64_t, 24>{
    0U,
    1U,
    4U,
    8U,
    15U,
    16U,
    17U,
    20U,
    31U,
    32U,
    33U,
    48U,
    63U,
    64U,
    65U,
    128U,
    255U,
    256U,
    257U,
    4095U,
    4096U,
    4097U,
    65535U,
    65536U};
constexpr auto cBoundarySizes = std::array<std::uint64_t, 31>{
    0U,
    1U,
    2U,
    4U,
    7U,
    8U,
    15U,
    16U,
    17U,
    31U,
    32U,
    33U,
    63U,
    64U,
    65U,
    127U,
    128U,
    129U,
    255U,
    256U,
    257U,
    4095U,
    4096U,
    4097U,
    65535U,
    65536U,
    65537U,
    1024U * 1024U,
    8U * 1024U * 1024U,
    16U * 1024U * 1024U,
    64U * 1024U * 1024U};

[[noreturn]] void configError(const el::String &message) {
    throw el::ApplicationError{message};
}

[[noreturn]] void configError(const el::String &message, const ValuePtr &value) {
    throw el::conf::ConfError{el::conf::ConfErrorCategory::Validation, message, value->namePath(), value->location()};
}

void requireKnownKeys(const ValuePtr &value, const std::span<const el::StringLiteral> known) {
    for (const auto &child : *value) {
        const auto childName = child->name().asText();
        if (std::ranges::none_of(known, [&](const auto name) -> bool { return childName == el::String{name}; })) {
            configError(el::StringFormat{"Unknown configuration key '{}'."_el}.build(childName), child);
        }
    }
}

[[nodiscard]] auto texts(const ValuePtr &value, const el::String &name, std::vector<el::String> defaults)
    -> std::vector<el::String> {
    return value->hasValue(name) ? value->valueOrThrow(name)->asListOrThrow<el::String>() : std::move(defaults);
}

[[nodiscard]] auto positiveInteger(
    const ValuePtr &value, const el::String &name, const std::uint64_t defaultValue, const bool allowZero = false)
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

[[nodiscard]] auto parseMode(const el::String &value) -> RunMode {
    if (value == "profile"_el) {
        return RunMode::Profile;
    }
    if (value == "benchmark"_el) {
        return RunMode::Benchmark;
    }
    configError(el::StringFormat{"Unsupported run mode '{}'."_el}.build(value));
}

template <typename T, typename Parser>
[[nodiscard]] auto parseNames(
    const std::vector<el::String> &values, const el::StringLiteral kind, Parser parser, bool &all) -> std::vector<T> {
    auto result = std::vector<T>{};
    all = false;
    for (const auto &value : values) {
        if (value == "all"_el) {
            if (values.size() != 1U) {
                configError(el::StringFormat{"'all' cannot be combined with explicit {} names."_el}.build(kind));
            }
            all = true;
            return {};
        }
        const auto parsed = parser(value);
        if (!parsed) {
            configError(el::StringFormat{"Unsupported {} '{}'."_el}.build(kind, value));
        }
        result.emplace_back(*parsed);
    }
    return result;
}

[[nodiscard]] auto parseSensitiveNames(const std::vector<el::String> &values) -> std::vector<SensitiveSelection> {
    auto result = std::vector<SensitiveSelection>{};
    for (const auto &value : values) {
        const auto parsed = parseSensitiveSelection(value);
        if (!parsed) {
            configError(el::StringFormat{"Unsupported sensitive mode '{}'."_el}.build(value));
        }
        if (*parsed == SensitiveSelection::All) {
            result = {SensitiveSelection::Normal, SensitiveSelection::Sensitive};
        } else {
            result.emplace_back(*parsed);
        }
    }
    return result;
}

[[nodiscard]] auto parseScenario(const ValuePtr &value) -> ScenarioTemplate {
    constexpr auto known = std::array{
        "name"_el,
        "types"_el,
        "use_cases"_el,
        "variants"_el,
        "size_modes"_el,
        "size_min"_el,
        "size_max"_el,
        "size_step"_el,
        "operand_size_min"_el,
        "operand_size_max"_el,
        "sensitive_modes"_el,
        "weight"_el};
    requireKnownKeys(value, known);
    auto result = ScenarioTemplate{};
    result.name = value->getTextOrThrow("name"_el);
    if (result.name.isEmpty()) {
        configError("Scenario names must not be empty."_el, value->valueOrThrow("name"_el));
    }
    result.types =
        parseNames<ByteType>(texts(value, "types"_el, {"all"_el}), "byte type"_el, parseByteType, result.allTypes);
    result.useCases =
        parseNames<UseCase>(texts(value, "use_cases"_el, {"all"_el}), "use case"_el, parseUseCase, result.allUseCases);
    const auto variantNames = texts(value, "variants"_el, {"all"_el});
    result.allVariants = variantNames.size() == 1U && variantNames.front() == "all"_el;
    if (!result.allVariants) {
        if (std::ranges::find(variantNames, el::String{"all"_el}) != variantNames.end()) {
            configError("'all' cannot be combined with explicit variant names."_el, value->valueOrThrow("variants"_el));
        }
        result.variants = variantNames;
    }
    auto unusedAll = false;
    result.sizeModes =
        parseNames<SizeMode>(texts(value, "size_modes"_el, {"fixed"_el}), "size mode"_el, parseSizeMode, unusedAll);
    if (unusedAll) {
        result.sizeModes = {
            SizeMode::Fixed, SizeMode::Incrementing, SizeMode::Exponential, SizeMode::Random, SizeMode::Boundaries};
    }
    result.sizeMinimum = positiveInteger(value, "size_min"_el, result.sizeMinimum, true);
    result.sizeMaximum = positiveInteger(value, "size_max"_el, result.sizeMaximum, true);
    result.sizeStep = positiveInteger(value, "size_step"_el, result.sizeStep);
    result.operandSizeMinimum = positiveInteger(value, "operand_size_min"_el, result.operandSizeMinimum, true);
    result.operandSizeMaximum = positiveInteger(value, "operand_size_max"_el, result.operandSizeMaximum, true);
    if (value->hasValue("sensitive_modes"_el)) {
        result.sensitiveModes = parseSensitiveNames(texts(value, "sensitive_modes"_el, {}));
    }
    const auto weight = positiveInteger(value, "weight"_el, result.weight);
    if (weight > std::numeric_limits<std::uint32_t>::max()) {
        configError("Scenario weight exceeds the supported range."_el, value->valueOrThrow("weight"_el));
    }
    result.weight = static_cast<std::uint32_t>(weight);
    if (result.sizeMinimum > result.sizeMaximum || result.operandSizeMinimum > result.operandSizeMaximum) {
        configError("A scenario minimum must not exceed its maximum."_el, value);
    }
    return result;
}

[[nodiscard]] auto supportsArrayExtent(const std::uint64_t value) noexcept -> bool {
    return std::ranges::find(cArrayExtents, value) != cArrayExtents.end();
}

[[nodiscard]] auto expandedSizes(const ScenarioTemplate &source, const SizeMode mode, const std::uint64_t seed)
    -> std::vector<std::uint64_t> {
    auto result = std::vector<std::uint64_t>{};
    const auto minimum = source.sizeMinimum;
    const auto maximum = source.sizeMaximum;
    if (mode == SizeMode::Fixed) {
        result.emplace_back(minimum);
        if (maximum != minimum) {
            result.emplace_back(maximum);
        }
    } else if (mode == SizeMode::Incrementing) {
        for (auto size = minimum; size <= maximum && result.size() < 64U;) {
            result.emplace_back(size);
            if (maximum - size < source.sizeStep) {
                break;
            }
            size += source.sizeStep;
        }
        if (result.back() != maximum) {
            result.emplace_back(maximum);
        }
    } else if (mode == SizeMode::Exponential) {
        auto size = minimum;
        result.emplace_back(size);
        while (size < maximum && result.size() < 64U) {
            size = size == 0U ? 1U : std::min(maximum, size > maximum / 2U ? maximum : size * 2U);
            if (size != result.back()) {
                result.emplace_back(size);
            }
        }
    } else if (mode == SizeMode::Random) {
        auto random = el::FastRandom{seed};
        result.emplace_back(minimum);
        for (auto i = 0U; i < 8U && minimum != maximum; ++i) {
            result.emplace_back(random.getUInt64(minimum, maximum));
        }
        if (maximum != minimum) {
            result.emplace_back(maximum);
        }
    } else {
        for (const auto size : cBoundarySizes) {
            if (size >= minimum && size <= maximum) {
                result.emplace_back(size);
            }
        }
        if (result.empty() || result.front() != minimum) {
            result.insert(result.begin(), minimum);
        }
        if (result.back() != maximum) {
            result.emplace_back(maximum);
        }
    }
    std::ranges::sort(result);
    const auto uniqueEnd = std::ranges::unique(result).begin();
    result.erase(uniqueEnd, result.end());
    return result;
}

[[nodiscard]] auto scenarioId(
    const el::String &group,
    const UseCaseDescriptor &descriptor,
    const SizeMode sizeMode,
    const SensitiveMode sensitiveMode,
    const std::uint64_t size,
    const std::uint64_t operandSize) -> el::String {
    return el::StringFormat{"{}:{}:{}:{}:{}:{}:size-{}:operand-{}"_el}.build(
        group,
        toString(descriptor.type),
        toString(descriptor.useCase),
        descriptor.variant,
        toString(sizeMode),
        toString(sensitiveMode),
        size,
        operandSize);
}

void appendScenario(
    std::vector<Scenario> &target,
    const el::String &group,
    const UseCaseDescriptor &descriptor,
    const SizeMode sizeMode,
    const SensitiveMode sensitiveMode,
    const std::uint64_t size,
    const std::uint64_t operandSize,
    const std::uint32_t weight) {
    if (descriptor.type == ByteType::Array && !supportsArrayExtent(size)) {
        configError(el::StringFormat{"ByteArray size {} is not compiled into the profiler."_el}.build(size));
    }
    if (descriptor.type == ByteType::RingBuffer && size == 0U) {
        configError("ByteRingBuffer scenarios require a non-zero primary size."_el);
    }
    const auto effectiveSensitive = descriptor.type == ByteType::Array ? SensitiveMode::NotApplicable : sensitiveMode;
    const auto id = scenarioId(group, descriptor, sizeMode, effectiveSensitive, size, operandSize);
    target.emplace_back(
        Scenario{
            .id = id,
            .group = group,
            .type = descriptor.type,
            .useCase = descriptor.useCase,
            .variant = descriptor.variant,
            .sizeMode = sizeMode,
            .sensitiveMode = effectiveSensitive,
            .size = size,
            .operandSize = operandSize,
            .weight = weight,
            .workUnit = descriptor.workUnit});
}

[[nodiscard]] auto typeSelected(const ScenarioTemplate &source, const ByteType type) -> bool {
    return source.allTypes || std::ranges::find(source.types, type) != source.types.end();
}

[[nodiscard]] auto useCaseSelected(const ScenarioTemplate &source, const UseCase useCase) -> bool {
    return source.allUseCases || std::ranges::find(source.useCases, useCase) != source.useCases.end();
}

[[nodiscard]] auto variantSelected(const ScenarioTemplate &source, const el::String &variant) -> bool {
    return source.allVariants || std::ranges::find(source.variants, variant) != source.variants.end();
}

void expandTemplate(
    const ScenarioTemplate &source, const RunSettings &run, std::vector<Scenario> &target, const ValuePtr &value) {
    auto matchedDescriptor = false;
    for (const auto &descriptor : coverageRegistry()) {
        if (!typeSelected(source, descriptor.type) || !useCaseSelected(source, descriptor.useCase) ||
            !variantSelected(source, descriptor.variant)) {
            continue;
        }
        matchedDescriptor = true;
        for (const auto sizeMode : source.sizeModes) {
            const auto sizes = expandedSizes(source, sizeMode, run.seed ^ source.name.toHash());
            for (const auto size : sizes) {
                const auto operand =
                    std::min(source.operandSizeMaximum, std::max(source.operandSizeMinimum, size / 4U));
                if (!supportsSensitiveMode(descriptor.type)) {
                    appendScenario(
                        target,
                        source.name,
                        descriptor,
                        sizeMode,
                        SensitiveMode::NotApplicable,
                        size,
                        operand,
                        source.weight);
                    continue;
                }
                auto sensitiveModes = source.sensitiveModes;
                if (sensitiveModes.empty()) {
                    sensitiveModes = run.sensitiveSelection == SensitiveSelection::All
                        ? std::vector{SensitiveSelection::Normal, SensitiveSelection::Sensitive}
                        : std::vector{run.sensitiveSelection};
                }
                for (const auto selection : sensitiveModes) {
                    const auto mode =
                        selection == SensitiveSelection::Sensitive ? SensitiveMode::Sensitive : SensitiveMode::Normal;
                    appendScenario(target, source.name, descriptor, sizeMode, mode, size, operand, source.weight);
                }
            }
        }
    }
    if (!matchedDescriptor) {
        const auto message =
            el::StringFormat{"Scenario '{}' has no compatible type/use-case/variant path."_el}.build(source.name);
        if (value) {
            configError(message, value);
        }
        configError(message);
    }
}

[[nodiscard]] auto snapshotSizes(const UseCaseDescriptor &descriptor) -> std::vector<std::uint64_t> {
    if (descriptor.type == ByteType::Array) {
        if (descriptor.useCase == UseCase::Create || descriptor.useCase == UseCase::SecureErase ||
            descriptor.useCase == UseCase::Bitwise || descriptor.useCase == UseCase::ShiftRotate ||
            descriptor.useCase == UseCase::PerByteShiftRotate) {
            return {cArrayExtents.begin(), cArrayExtents.end()};
        }
        return {64U, 4096U};
    }
    if (descriptor.type == ByteType::RingBuffer) {
        return {4096U, 65536U};
    }
    if (descriptor.useCase == UseCase::Create || descriptor.useCase == UseCase::Copy ||
        descriptor.useCase == UseCase::Convert || descriptor.useCase == UseCase::Traverse ||
        descriptor.useCase == UseCase::Compare || descriptor.useCase == UseCase::Find ||
        descriptor.useCase == UseCase::Fill || descriptor.useCase == UseCase::Xor ||
        descriptor.useCase == UseCase::SecureErase) {
        return {64U, 4096U, 65536U, 1024U * 1024U};
    }
    return {4096U, 65536U};
}

[[nodiscard]] auto builtInSuite(const RunSettings &run) -> std::vector<Scenario> {
    auto result = std::vector<Scenario>{};
    if (run.suite == "smoke"_el) {
        constexpr auto smokeCases = std::array{
            std::pair{ByteType::Array, UseCase::Bitwise},
            std::pair{ByteType::Block, UseCase::Find},
            std::pair{ByteType::BlockEditor, UseCase::CowStress},
            std::pair{ByteType::Buffer, UseCase::EditStress},
            std::pair{ByteType::RingBuffer, UseCase::RingWrappedCycle}};
        for (const auto &[type, useCase] : smokeCases) {
            const auto descriptor = std::ranges::find_if(
                coverageRegistry(), [&](const auto &entry) { return entry.type == type && entry.useCase == useCase; });
            const auto size = type == ByteType::Array ? 64U : 1024U;
            if (type == ByteType::Array) {
                appendScenario(
                    result, "smoke"_el, *descriptor, SizeMode::Fixed, SensitiveMode::NotApplicable, size, 16U, 1U);
            } else {
                appendScenario(result, "smoke"_el, *descriptor, SizeMode::Fixed, SensitiveMode::Normal, size, 64U, 1U);
                appendScenario(
                    result, "smoke"_el, *descriptor, SizeMode::Fixed, SensitiveMode::Sensitive, size, 64U, 1U);
            }
        }
        return result;
    }
    if (run.suite != "snapshot"_el) {
        configError(el::StringFormat{"Unsupported built-in suite '{}'."_el}.build(run.suite));
    }
    for (const auto &descriptor : coverageRegistry()) {
        for (const auto size : snapshotSizes(descriptor)) {
            const auto operand = std::max<std::uint64_t>(1U, std::min<std::uint64_t>(4096U, size / 4U));
            if (!supportsSensitiveMode(descriptor.type)) {
                appendScenario(
                    result,
                    "snapshot"_el,
                    descriptor,
                    SizeMode::Fixed,
                    SensitiveMode::NotApplicable,
                    size,
                    operand,
                    1U);
            } else if (run.sensitiveSelection == SensitiveSelection::Normal) {
                appendScenario(
                    result, "snapshot"_el, descriptor, SizeMode::Fixed, SensitiveMode::Normal, size, operand, 1U);
            } else if (run.sensitiveSelection == SensitiveSelection::Sensitive) {
                appendScenario(
                    result, "snapshot"_el, descriptor, SizeMode::Fixed, SensitiveMode::Sensitive, size, operand, 1U);
            } else {
                appendScenario(
                    result, "snapshot"_el, descriptor, SizeMode::Fixed, SensitiveMode::Normal, size, operand, 1U);
                appendScenario(
                    result, "snapshot"_el, descriptor, SizeMode::Fixed, SensitiveMode::Sensitive, size, operand, 1U);
            }
        }
    }
    return result;
}

void parseRun(const ValuePtr &run, RunSettings &result) {
    constexpr auto keys = std::array{
        "mode"_el,
        "suite"_el,
        "duration"_el,
        "threads"_el,
        "seed"_el,
        "warmup_samples"_el,
        "samples"_el,
        "minimum_sample_time"_el,
        "memory_limit"_el,
        "progress_interval"_el,
        "sensitive_mode"_el};
    requireKnownKeys(run, keys);
    result.mode = parseMode(run->getText("mode"_el, toString(result.mode)));
    result.suite = run->getText("suite"_el, result.suite);
    if (run->hasValue("duration"_el)) {
        result.duration = run->getCalendarDeltaOrThrow("duration"_el).toTimeDeltaOrThrow().toStdNanoseconds();
    }
    const auto threads = positiveInteger(run, "threads"_el, result.threadCount);
    if (threads > 256U) {
        configError("The workload thread count must be in the range 1-256."_el, run->valueOrThrow("threads"_el));
    }
    result.threadCount = static_cast<std::uint32_t>(threads);
    const auto seed = run->getInteger("seed"_el, static_cast<el::conf::Integer>(result.seed));
    if (seed < 0) {
        configError("The global seed must not be negative."_el, run->valueOrThrow("seed"_el));
    }
    result.seed = static_cast<std::uint64_t>(seed);
    result.warmupSamples =
        static_cast<std::uint32_t>(positiveInteger(run, "warmup_samples"_el, result.warmupSamples, true));
    result.samples = static_cast<std::uint32_t>(positiveInteger(run, "samples"_el, result.samples));
    if (run->hasValue("minimum_sample_time"_el)) {
        result.minimumSampleTime =
            run->getCalendarDeltaOrThrow("minimum_sample_time"_el).toTimeDeltaOrThrow().toStdNanoseconds();
    }
    result.memoryLimit = positiveInteger(run, "memory_limit"_el, result.memoryLimit);
    if (run->hasValue("progress_interval"_el)) {
        result.progressInterval =
            run->getCalendarDeltaOrThrow("progress_interval"_el).toTimeDeltaOrThrow().toStdNanoseconds();
    }
    const auto sensitive =
        parseSensitiveSelection(run->getText("sensitive_mode"_el, toString(result.sensitiveSelection)));
    if (!sensitive) {
        configError("Unsupported run sensitive mode."_el, run->valueOrThrow("sensitive_mode"_el));
    }
    result.sensitiveSelection = *sensitive;
    if (result.duration <= std::chrono::nanoseconds::zero() ||
        result.minimumSampleTime <= std::chrono::nanoseconds::zero() ||
        result.progressInterval <= std::chrono::nanoseconds::zero()) {
        configError("Run durations and intervals must be positive."_el);
    }
}

void parseDocument(
    const ValuePtr &document, RunSettings &run, std::vector<ScenarioTemplate> &templates, const bool replaceTemplates) {
    constexpr auto rootKeys = std::array{"run"_el, "scenario"_el};
    requireKnownKeys(document, rootKeys);
    if (document->hasValue("run"_el)) {
        parseRun(document->valueOrThrow("run"_el), run);
    }
    if (document->hasValue("scenario"_el)) {
        if (replaceTemplates) {
            templates.clear();
        }
        for (const auto &entry : *document->valueOrThrow("scenario"_el)) {
            templates.emplace_back(parseScenario(entry));
        }
    }
}

}
