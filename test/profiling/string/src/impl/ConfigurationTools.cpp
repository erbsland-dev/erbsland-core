// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConfigurationTools.hpp"

#include <erbsland/conf/Parser.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <set>

namespace app::string::impl {

using namespace el::text::literals;

using ValuePtr = el::conf::ValuePtr;

constexpr auto cLogicalBoundaries = std::array<std::uint64_t, 20>{
    0U, 1U, 2U, 7U, 8U, 15U, 16U, 17U, 31U, 32U, 33U, 63U, 64U, 65U, 127U, 128U, 129U, 255U, 256U, 257U};
constexpr auto cNativeByteBoundaries = std::array<std::uint64_t, 8>{16U, 32U, 64U, 128U, 256U, 4096U, 65536U, 1048576U};

[[noreturn]] void configError(const el::String &message) {
    throw el::ApplicationError{message};
}

[[noreturn]] void configError(const el::String &message, const ValuePtr &value) {
    throw el::conf::ConfError{el::conf::ConfErrorCategory::Validation, message, value->namePath(), value->location()};
}

void requireKnownKeys(const ValuePtr &value, const std::span<const el::StringLiteral> known) {
    for (const auto &child : *value) {
        const auto childName = child->name().asText();
        if (std::ranges::none_of(known, [&](const auto name) { return childName == el::String{name}; })) {
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
        "widths"_el,
        "types"_el,
        "use_cases"_el,
        "variants"_el,
        "content_profiles"_el,
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
    result.widths = parseNames<StringWidth>(
        texts(value, "widths"_el, {"all"_el}), "string width"_el, parseStringWidth, result.allWidths);
    result.types = parseNames<StringType>(
        texts(value, "types"_el, {"all"_el}), "string type"_el, parseStringType, result.allTypes);
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
    result.contentProfiles = parseNames<ContentProfile>(
        texts(value, "content_profiles"_el, {"mixed"_el}),
        "content profile"_el,
        parseContentProfile,
        result.allContentProfiles);
    if (result.allContentProfiles) {
        result.contentProfiles = {
            ContentProfile::Ascii,
            ContentProfile::Mixed,
            ContentProfile::Supplementary,
            ContentProfile::MalformedSparse,
            ContentProfile::MalformedDense};
    }
    auto allSizeModes = false;
    result.sizeModes =
        parseNames<SizeMode>(texts(value, "size_modes"_el, {"fixed"_el}), "size mode"_el, parseSizeMode, allSizeModes);
    if (allSizeModes) {
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

void appendBoundaryNeighbor(
    std::vector<std::uint64_t> &target,
    const StringWidth width,
    const ContentProfile profile,
    const std::uint64_t byteBoundary,
    const std::uint64_t maximum) {
    auto low = std::uint64_t{};
    auto high = std::max<std::uint64_t>(1U, maximum);
    while (low < high) {
        const auto middle = low + (high - low) / 2U;
        if (nativeBytesFor(width, profile, middle) < byteBoundary) {
            low = middle + 1U;
        } else {
            high = middle;
        }
    }
    for (const auto candidate : {low == 0U ? 0U : low - 1U, low, low == maximum ? maximum : low + 1U}) {
        target.emplace_back(candidate);
    }
}

[[nodiscard]] auto expandedSizes(
    const ScenarioTemplate &source,
    const SizeMode mode,
    const StringWidth width,
    const ContentProfile profile,
    const std::uint64_t seed) -> std::vector<std::uint64_t> {
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
        if (result.empty() || result.back() != maximum) {
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
        for (auto index = 0U; index < 8U && minimum != maximum; ++index) {
            result.emplace_back(random.getUInt64(minimum, maximum));
        }
        result.emplace_back(maximum);
    } else {
        for (const auto boundary : cLogicalBoundaries) {
            if (boundary >= minimum && boundary <= maximum) {
                result.emplace_back(boundary);
            }
        }
        for (const auto boundary : cNativeByteBoundaries) {
            appendBoundaryNeighbor(result, width, profile, boundary, maximum);
        }
        result.emplace_back(minimum);
        result.emplace_back(maximum);
    }
    std::erase_if(result, [&](const auto size) { return size < minimum || size > maximum; });
    std::ranges::sort(result);
    const auto uniqueEnd = std::ranges::unique(result).begin();
    result.erase(uniqueEnd, result.end());
    return result;
}

[[nodiscard]] auto scenarioId(
    const el::String &group,
    const StringWidth width,
    const UseCaseDescriptor &descriptor,
    const ContentProfile profile,
    const SizeMode sizeMode,
    const SensitiveMode sensitiveMode,
    const std::uint64_t size,
    const std::uint64_t operandSize) -> el::String {
    return el::StringFormat{"{}:{}:{}:{}:{}:{}:{}:{}:size-{}:operand-{}"_el}.build(
        group,
        toString(width),
        toString(descriptor.type),
        toString(descriptor.useCase),
        descriptor.variant,
        toString(profile),
        toString(sizeMode),
        toString(sensitiveMode),
        size,
        operandSize);
}

void appendScenario(
    std::vector<Scenario> &target,
    const el::String &group,
    const StringWidth width,
    const UseCaseDescriptor &descriptor,
    const ContentProfile profile,
    const SizeMode sizeMode,
    const SensitiveMode sensitiveMode,
    const std::uint64_t size,
    const std::uint64_t operandSize,
    const std::uint32_t weight) {
    if (descriptor.u8Only && width != StringWidth::U8) {
        return;
    }
    const auto effectiveSensitive = width == StringWidth::U8 ? sensitiveMode : SensitiveMode::NotApplicable;
    target.emplace_back(
        Scenario{
            .id = scenarioId(group, width, descriptor, profile, sizeMode, effectiveSensitive, size, operandSize),
            .group = group,
            .width = width,
            .type = descriptor.type,
            .useCase = descriptor.useCase,
            .variant = descriptor.variant,
            .contentProfile = profile,
            .sizeMode = sizeMode,
            .sensitiveMode = effectiveSensitive,
            .size = size,
            .operandSize = operandSize,
            .weight = weight,
            .workUnit = descriptor.workUnit});
}

template <typename T>
[[nodiscard]] auto selected(const bool all, const std::vector<T> &values, const T value) -> bool {
    return all || std::ranges::find(values, value) != values.end();
}

void expandTemplate(
    const ScenarioTemplate &source, const RunSettings &run, std::vector<Scenario> &target, const ValuePtr &value) {
    auto matched = false;
    for (const auto &descriptor : coverageRegistry()) {
        if (!selected(source.allTypes, source.types, descriptor.type) ||
            !selected(source.allUseCases, source.useCases, descriptor.useCase) ||
            (!source.allVariants && std::ranges::find(source.variants, descriptor.variant) == source.variants.end())) {
            continue;
        }
        for (
            auto width = StringWidth::U8; width <= StringWidth::U32;
            width = static_cast<StringWidth>(static_cast<int>(width) + 1)) {
            if (!selected(source.allWidths, source.widths, width) || (descriptor.u8Only && width != StringWidth::U8)) {
                continue;
            }
            matched = true;
            for (const auto profile : source.contentProfiles) {
                for (const auto sizeMode : source.sizeModes) {
                    const auto sizes = expandedSizes(source, sizeMode, width, profile, run.seed ^ source.name.toHash());
                    for (const auto size : sizes) {
                        const auto operand = std::min(
                            source.operandSizeMaximum,
                            std::max(source.operandSizeMinimum, size == 0U ? 0U : size / 4U));
                        auto selections = source.sensitiveModes;
                        if (selections.empty()) {
                            selections = run.sensitiveSelection == SensitiveSelection::All
                                ? std::vector{SensitiveSelection::Normal, SensitiveSelection::Sensitive}
                                : std::vector{run.sensitiveSelection};
                        }
                        if (width != StringWidth::U8) {
                            appendScenario(
                                target,
                                source.name,
                                width,
                                descriptor,
                                profile,
                                sizeMode,
                                SensitiveMode::NotApplicable,
                                size,
                                operand,
                                source.weight);
                            continue;
                        }
                        for (const auto selection : selections) {
                            appendScenario(
                                target,
                                source.name,
                                width,
                                descriptor,
                                profile,
                                sizeMode,
                                selection == SensitiveSelection::Sensitive ? SensitiveMode::Sensitive
                                                                           : SensitiveMode::Normal,
                                size,
                                operand,
                                source.weight);
                        }
                    }
                }
            }
        }
    }
    if (!matched) {
        const auto message =
            el::StringFormat{"Scenario '{}' has no compatible width/type/use-case/variant path."_el}.build(source.name);
        if (value) {
            configError(message, value);
        }
        configError(message);
    }
}

[[nodiscard]] auto decodeSensitive(const UseCase useCase) noexcept -> bool {
    return useCase == UseCase::WidthConvert || useCase == UseCase::Inspect || useCase == UseCase::ReadIndexed ||
        useCase == UseCase::Traverse || useCase == UseCase::Compare || useCase == UseCase::Hash ||
        useCase == UseCase::Search || useCase == UseCase::Transform || useCase == UseCase::EscapeSafe;
}

[[nodiscard]] auto snapshotSizes(const UseCaseDescriptor &descriptor) -> std::vector<std::uint64_t> {
    if (descriptor.workUnit == WorkUnit::Operations) {
        return {64U, 4096U};
    }
    if (descriptor.useCase == UseCase::Create || descriptor.useCase == UseCase::Copy ||
        descriptor.useCase == UseCase::WidthConvert || descriptor.useCase == UseCase::Traverse ||
        descriptor.useCase == UseCase::Compare || descriptor.useCase == UseCase::Hash ||
        descriptor.useCase == UseCase::Search) {
        return {64U, 4096U, 65536U};
    }
    return {4096U};
}

[[nodiscard]] auto builtInSuite(const RunSettings &run) -> std::vector<Scenario> {
    auto result = std::vector<Scenario>{};
    if (run.suite == "smoke"_el) {
        struct SmokePath {
            StringWidth width;
            StringType type;
            UseCase useCase;
            el::StringLiteral variant;
        };
        constexpr auto paths = std::array{
            SmokePath{StringWidth::U8, StringType::String, UseCase::Traverse, "reader-forward"_el},
            SmokePath{StringWidth::U16, StringType::StringEditor, UseCase::Append, "text-growing"_el},
            SmokePath{StringWidth::U32, StringType::String, UseCase::Search, "find-adversarial"_el},
            SmokePath{StringWidth::U8, StringType::StringEditor, UseCase::CowStress, "fanout"_el}};
        for (const auto &path : paths) {
            const auto *descriptor = findDescriptor(path.type, path.useCase, el::String{path.variant});
            if (path.width != StringWidth::U8) {
                appendScenario(
                    result,
                    "smoke"_el,
                    path.width,
                    *descriptor,
                    ContentProfile::Mixed,
                    SizeMode::Fixed,
                    SensitiveMode::NotApplicable,
                    256U,
                    32U,
                    1U);
            } else {
                const auto add = [&](const SensitiveMode mode) {
                    appendScenario(
                        result,
                        "smoke"_el,
                        path.width,
                        *descriptor,
                        ContentProfile::Mixed,
                        SizeMode::Fixed,
                        mode,
                        256U,
                        32U,
                        1U);
                };
                if (run.sensitiveSelection != SensitiveSelection::Sensitive) {
                    add(SensitiveMode::Normal);
                }
                if (run.sensitiveSelection != SensitiveSelection::Normal) {
                    add(SensitiveMode::Sensitive);
                }
            }
        }
        return result;
    }
    if (run.suite != "snapshot"_el) {
        configError(el::StringFormat{"Unsupported built-in suite '{}'."_el}.build(run.suite));
    }
    for (const auto &descriptor : coverageRegistry()) {
        for (
            auto width = StringWidth::U8; width <= StringWidth::U32;
            width = static_cast<StringWidth>(static_cast<int>(width) + 1)) {
            if (descriptor.u8Only && width != StringWidth::U8) {
                continue;
            }
            auto contentSizes = std::vector<std::pair<ContentProfile, std::uint64_t>>{};
            const auto sizes = snapshotSizes(descriptor);
            for (const auto size : sizes) {
                contentSizes.emplace_back(ContentProfile::Mixed, size);
            }
            if (decodeSensitive(descriptor.useCase)) {
                const auto representativeSize = sizes.size() > 1U ? sizes[sizes.size() / 2U] : sizes.front();
                contentSizes.emplace_back(ContentProfile::Ascii, representativeSize);
                contentSizes.emplace_back(ContentProfile::Supplementary, representativeSize);
            }
            for (const auto &[profile, size] : contentSizes) {
                const auto operand = std::max<std::uint64_t>(1U, std::min<std::uint64_t>(1024U, size / 4U));
                if (width != StringWidth::U8) {
                    appendScenario(
                        result,
                        "snapshot"_el,
                        width,
                        descriptor,
                        profile,
                        SizeMode::Fixed,
                        SensitiveMode::NotApplicable,
                        size,
                        operand,
                        1U);
                } else if (run.sensitiveSelection == SensitiveSelection::All) {
                    appendScenario(
                        result,
                        "snapshot"_el,
                        width,
                        descriptor,
                        profile,
                        SizeMode::Fixed,
                        SensitiveMode::Normal,
                        size,
                        operand,
                        1U);
                    appendScenario(
                        result,
                        "snapshot"_el,
                        width,
                        descriptor,
                        profile,
                        SizeMode::Fixed,
                        SensitiveMode::Sensitive,
                        size,
                        operand,
                        1U);
                } else {
                    appendScenario(
                        result,
                        "snapshot"_el,
                        width,
                        descriptor,
                        profile,
                        SizeMode::Fixed,
                        run.sensitiveSelection == SensitiveSelection::Sensitive ? SensitiveMode::Sensitive
                                                                                : SensitiveMode::Normal,
                        size,
                        operand,
                        1U);
                }
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
    result.seed = static_cast<std::uint64_t>(run->getInteger("seed"_el, static_cast<el::conf::Integer>(result.seed)));
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
    const auto sensitivity =
        parseSensitiveSelection(run->getText("sensitive_mode"_el, toString(result.sensitiveSelection)));
    if (!sensitivity) {
        configError("Unsupported sensitive mode."_el, run->valueOrThrow("sensitive_mode"_el));
    }
    result.sensitiveSelection = *sensitivity;
    if (result.duration.count() <= 0 || result.minimumSampleTime.count() <= 0 || result.progressInterval.count() <= 0 ||
        result.memoryLimit == 0U) {
        configError("Run durations and memory limit must be positive."_el, run);
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
