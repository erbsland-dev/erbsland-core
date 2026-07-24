// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConfigurationTools.hpp"

#include <erbsland/conf/Parser.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <set>
#include <tuple>

namespace app::stream::impl {

using namespace el::text::literals;

using ValuePtr = el::conf::ValuePtr;

[[noreturn]] void configError(const el::String &message) {
    throw el::ApplicationError{message};
}

[[noreturn]] void configError(const el::String &message, const ValuePtr &value) {
    throw el::conf::ConfError{el::conf::ConfErrorCategory::Validation, message, value->namePath(), value->location()};
}

[[nodiscard]] auto textList(const ValuePtr &value, const el::String &name, std::vector<el::String> defaults)
    -> std::vector<el::String> {
    if (!value->hasValue(name)) {
        return defaults;
    }
    return value->valueOrThrow(name)->asListOrThrow<el::String>();
}

[[nodiscard]] auto integerValue(const ValuePtr &value, const el::String &name, const std::uint64_t defaultValue)
    -> std::uint64_t {
    if (!value->hasValue(name)) {
        return defaultValue;
    }
    const auto result = value->getIntegerOrThrow(name);
    if (result <= 0) {
        configError(el::StringFormat{"Configuration value '{}' must be positive."_el}.build(name));
    }
    return static_cast<std::uint64_t>(result);
}

void requireKnownKeys(const ValuePtr &value, const std::span<const el::StringLiteral> known) {
    for (const auto &child : *value) {
        const auto childName = child->name().asText();
        const auto found = std::ranges::any_of(
            known, [&](const auto knownName) -> bool { return childName == el::String{knownName}; });
        if (!found) {
            configError(el::StringFormat{"Unknown configuration key '{}'."_el}.build(childName), child);
        }
    }
}

[[nodiscard]] auto parseMode(const el::String &text) -> RunMode {
    if (text == "profile"_el) {
        return RunMode::Profile;
    }
    if (text == "benchmark"_el) {
        return RunMode::Benchmark;
    }
    configError(el::StringFormat{"Unsupported run mode '{}'."_el}.build(text));
}

[[nodiscard]] auto parseDirections(const std::vector<el::String> &values) -> std::vector<Direction> {
    auto result = std::vector<Direction>{};
    for (const auto &value : values) {
        if (value == "all"_el) {
            result = {Direction::Read, Direction::Write};
        } else if (value == "read"_el) {
            result.emplace_back(Direction::Read);
        } else if (value == "write"_el) {
            result.emplace_back(Direction::Write);
        } else {
            configError(el::StringFormat{"Unsupported direction '{}'."_el}.build(value));
        }
    }
    return result;
}

[[nodiscard]] auto parseFileTypes(const std::vector<el::String> &values) -> std::vector<FileType> {
    auto result = std::vector<FileType>{};
    for (const auto &value : values) {
        if (value == "all"_el) {
            result = {FileType::Binary, FileType::Utf8, FileType::Utf16, FileType::Utf32};
        } else if (value == "binary"_el) {
            result.emplace_back(FileType::Binary);
        } else if (value == "utf8"_el) {
            result.emplace_back(FileType::Utf8);
        } else if (value == "utf16"_el) {
            result.emplace_back(FileType::Utf16);
        } else if (value == "utf32"_el) {
            result.emplace_back(FileType::Utf32);
        } else {
            configError(el::StringFormat{"Unsupported file type '{}'."_el}.build(value));
        }
    }
    return result;
}

[[nodiscard]] auto parseMethod(const el::String &value) -> std::optional<Method> {
    constexpr auto entries = std::array{
        std::pair{"read-byte"_el, Method::ReadByte},
        std::pair{"read-span"_el, Method::ReadSpan},
        std::pair{"read-block"_el, Method::ReadBlock},
        std::pair{"read-exact"_el, Method::ReadExact},
        std::pair{"read-all"_el, Method::ReadAll},
        std::pair{"write-byte"_el, Method::WriteByte},
        std::pair{"write-span"_el, Method::WriteSpan},
        std::pair{"write-block"_el, Method::WriteBlock},
        std::pair{"read-char"_el, Method::ReadChar},
        std::pair{"read-text"_el, Method::ReadText},
        std::pair{"read-line"_el, Method::ReadLine},
        std::pair{"read-all-text"_el, Method::ReadAllText},
        std::pair{"write-char"_el, Method::WriteChar},
        std::pair{"write-text"_el, Method::WriteText},
        std::pair{"write-line"_el, Method::WriteLine},
    };
    for (const auto &[name, method] : entries) {
        if (value == name) {
            return method;
        }
    }
    return std::nullopt;
}

[[nodiscard]] auto parseChunkModes(const std::vector<el::String> &values) -> std::vector<ChunkMode> {
    auto result = std::vector<ChunkMode>{};
    for (const auto &value : values) {
        if (value == "all"_el) {
            result = {ChunkMode::Fixed, ChunkMode::Incrementing, ChunkMode::Random};
        } else if (value == "fixed"_el) {
            result.emplace_back(ChunkMode::Fixed);
        } else if (value == "incrementing"_el) {
            result.emplace_back(ChunkMode::Incrementing);
        } else if (value == "random"_el) {
            result.emplace_back(ChunkMode::Random);
        } else {
            configError(el::StringFormat{"Unsupported chunk mode '{}'."_el}.build(value));
        }
    }
    return result;
}

[[nodiscard]] auto parseLocalities(const std::vector<el::String> &values) -> std::vector<Locality> {
    auto result = std::vector<Locality>{};
    for (const auto &value : values) {
        if (value == "all"_el) {
            result = {Locality::Hot, Locality::Rotating};
        } else if (value == "hot"_el) {
            result.emplace_back(Locality::Hot);
        } else if (value == "rotating"_el) {
            result.emplace_back(Locality::Rotating);
        } else {
            configError(el::StringFormat{"Unsupported locality '{}'."_el}.build(value));
        }
    }
    return result;
}

[[nodiscard]] auto parseBuffering(const std::vector<el::String> &values) -> std::vector<el::StreamBuffering> {
    auto result = std::vector<el::StreamBuffering>{};
    for (const auto &value : values) {
        if (value == "all"_el) {
            result = {
                el::StreamBuffering::MinimalMemory,
                el::StreamBuffering::Interactive,
                el::StreamBuffering::Balanced,
                el::StreamBuffering::Throughput,
                el::StreamBuffering::Bulk};
        } else if (value == "minimal-memory"_el) {
            result.emplace_back(el::StreamBuffering::MinimalMemory);
        } else if (value == "interactive"_el) {
            result.emplace_back(el::StreamBuffering::Interactive);
        } else if (value == "balanced"_el) {
            result.emplace_back(el::StreamBuffering::Balanced);
        } else if (value == "throughput"_el) {
            result.emplace_back(el::StreamBuffering::Throughput);
        } else if (value == "bulk"_el) {
            result.emplace_back(el::StreamBuffering::Bulk);
        } else {
            configError(el::StringFormat{"Unsupported stream buffering preset '{}'."_el}.build(value));
        }
    }
    return result;
}

[[nodiscard]] auto compatibleMethods(const Direction direction, const FileType fileType) -> std::vector<Method> {
    if (fileType == FileType::Binary && direction == Direction::Read) {
        return {Method::ReadByte, Method::ReadSpan, Method::ReadBlock, Method::ReadExact, Method::ReadAll};
    }
    if (fileType == FileType::Binary) {
        return {Method::WriteByte, Method::WriteSpan, Method::WriteBlock};
    }
    if (direction == Direction::Read) {
        return {Method::ReadChar, Method::ReadText, Method::ReadLine, Method::ReadAllText};
    }
    return {Method::WriteChar, Method::WriteText, Method::WriteLine};
}

[[nodiscard]] auto isCompatible(const Method method, const Direction direction, const FileType fileType) noexcept
    -> bool {
    return methodDirection(method) == direction && isTextMethod(method) == isTextFile(fileType);
}

[[nodiscard]] auto buildId(
    const el::String &group,
    const Method method,
    const FileType fileType,
    const ChunkMode chunkMode,
    const Locality locality,
    const el::StreamBuffering buffering) -> el::String {
    return el::StringFormat{"{}:{}:{}:{}:{}:buffering-{}"_el}.build(
        group, toString(method), toString(fileType), toString(chunkMode), toString(locality), toString(buffering));
}

void appendExpanded(const ScenarioTemplate &source, std::vector<Scenario> &target, const ValuePtr &value) {
    for (const auto direction : source.directions) {
        for (const auto fileType : source.fileTypes) {
            const auto methods = source.allMethods ? compatibleMethods(direction, fileType) : source.methods;
            auto hasCompatibleMethod = false;
            for (const auto method : methods) {
                if (!isCompatible(method, direction, fileType)) {
                    continue;
                }
                hasCompatibleMethod = true;
                const auto usesChunks = method != Method::ReadByte && method != Method::ReadAll &&
                    method != Method::WriteByte && method != Method::ReadChar && method != Method::ReadAllText &&
                    method != Method::WriteChar;
                const auto modes = usesChunks ? source.chunkModes : std::vector{ChunkMode::Fixed};
                for (const auto mode : modes) {
                    for (const auto locality : source.localities) {
                        for (const auto buffering : source.buffering) {
                            target.emplace_back(
                                Scenario{
                                    .id = buildId(source.name, method, fileType, mode, locality, buffering),
                                    .group = source.name,
                                    .direction = direction,
                                    .fileType = fileType,
                                    .method = method,
                                    .locality = locality,
                                    .chunkMode = mode,
                                    .fileSizeMinimum = source.fileSizeMinimum,
                                    .fileSizeMaximum = source.fileSizeMaximum,
                                    .chunkSizeMinimum = source.chunkSizeMinimum,
                                    .chunkSizeMaximum = source.chunkSizeMaximum,
                                    .buffering = buffering,
                                    .backBufferLimit = source.backBufferLimit,
                                    .weight = source.weight,
                                });
                        }
                    }
                }
            }
            if (!hasCompatibleMethod) {
                configError(
                    el::StringFormat{"No explicit method is compatible with direction '{}' and file type '{}'."_el}
                        .build(toString(direction), toString(fileType)),
                    value);
            }
        }
    }
}

[[nodiscard]] auto parseScenario(const ValuePtr &value) -> ScenarioTemplate {
    constexpr auto known = std::array{
        "name"_el,
        "direction"_el,
        "file_type"_el,
        "methods"_el,
        "file_size_min"_el,
        "file_size_max"_el,
        "chunk_modes"_el,
        "chunk_size_min"_el,
        "chunk_size_max"_el,
        "buffering"_el,
        "back_buffer_limit"_el,
        "localities"_el,
        "weight"_el,
    };
    requireKnownKeys(value, known);
    auto result = ScenarioTemplate{};
    result.name = value->getTextOrThrow("name"_el);
    if (result.name.isEmpty()) {
        configError("Scenario names must not be empty."_el, value->valueOrThrow("name"_el));
    }
    result.directions = parseDirections(textList(value, "direction"_el, {"read"_el}));
    result.fileTypes = parseFileTypes(textList(value, "file_type"_el, {"binary"_el}));
    const auto methodTexts = textList(value, "methods"_el, {"all"_el});
    result.allMethods =
        std::ranges::any_of(methodTexts, [](const el::String &value) -> bool { return value == "all"_el; });
    if (result.allMethods && methodTexts.size() != 1U) {
        configError(
            "The method 'all' cannot be combined with explicit method names."_el, value->valueOrThrow("methods"_el));
    }
    if (!result.allMethods) {
        for (const auto &methodText : methodTexts) {
            if (methodText == "read-all"_el) {
                if (std::ranges::find(result.fileTypes, FileType::Binary) != result.fileTypes.end()) {
                    result.methods.emplace_back(Method::ReadAll);
                }
                if (std::ranges::any_of(
                        result.fileTypes, [](const FileType type) -> bool { return type != FileType::Binary; })) {
                    result.methods.emplace_back(Method::ReadAllText);
                }
                continue;
            }
            const auto method = parseMethod(methodText);
            if (!method) {
                configError(el::StringFormat{"Unsupported stream method '{}'."_el}.build(methodText), value);
            }
            result.methods.emplace_back(*method);
        }
    }
    result.fileSizeMinimum = integerValue(value, "file_size_min"_el, result.fileSizeMinimum);
    result.fileSizeMaximum = integerValue(value, "file_size_max"_el, result.fileSizeMaximum);
    result.chunkModes = parseChunkModes(textList(value, "chunk_modes"_el, {"fixed"_el}));
    result.chunkSizeMinimum = integerValue(value, "chunk_size_min"_el, result.chunkSizeMinimum);
    result.chunkSizeMaximum = integerValue(value, "chunk_size_max"_el, result.chunkSizeMaximum);
    result.buffering = parseBuffering(textList(value, "buffering"_el, {"balanced"_el}));
    if (value->hasValue("back_buffer_limit"_el)) {
        result.backBufferLimit = integerValue(value, "back_buffer_limit"_el, result.backBufferLimit);
    }
    const auto weight = integerValue(value, "weight"_el, result.weight);
    if (weight > std::numeric_limits<std::uint32_t>::max()) {
        configError("Scenario weight is too large."_el);
    }
    result.weight = static_cast<std::uint32_t>(weight);
    result.localities = parseLocalities(textList(value, "localities"_el, {"hot"_el}));
    if (result.fileSizeMinimum > result.fileSizeMaximum || result.chunkSizeMinimum > result.chunkSizeMaximum) {
        configError(
            el::StringFormat{"Scenario '{}' has a minimum larger than its maximum."_el}.build(result.name), value);
    }
    if (result.backBufferLimit != 0U && result.backBufferLimit < result.chunkSizeMaximum) {
        configError(
            el::StringFormat{"Scenario '{}' has a back-buffer limit smaller than its largest chunk."_el}.build(
                result.name),
            value);
    }
    return result;
}

[[nodiscard]] auto curatedSuite(const bool smoke) -> std::vector<Scenario> {
    const auto smallSize = smoke ? 1024ULL : 256ULL * 1024ULL;
    const auto characterWriteSize = smoke ? 1024ULL : 4ULL * 1024ULL;
    const auto bulkSize = smoke ? 4096ULL : 8ULL * 1024ULL * 1024ULL;
    const auto textWriteSize = smoke ? 4096ULL : 1024ULL * 1024ULL;
    const auto largeSize = smoke ? 8192ULL : 64ULL * 1024ULL * 1024ULL;
    const auto chunkMaximum = smoke ? 257ULL : 1024ULL * 1024ULL;
    auto result = std::vector<Scenario>{};
    auto add = [&](const el::String &group,
                   const Method method,
                   const FileType type,
                   const std::uint64_t size,
                   const ChunkMode mode,
                   const Locality locality,
                   const el::StreamBuffering buffering) -> void {
        const auto direction = methodDirection(method);
        auto backBufferLimit = std::uint64_t{};
        if (method == Method::WriteBlock) {
            backBufferLimit = chunkMaximum;
        } else if (method == Method::WriteText || method == Method::WriteLine) {
            // Allow a four-byte BOM, worst-case encoded characters, and a line ending.
            backBufferLimit = (chunkMaximum + 2U) * 4U;
        }
        result.emplace_back(
            Scenario{
                .id = buildId(group, method, type, mode, locality, buffering),
                .group = group,
                .direction = direction,
                .fileType = type,
                .method = method,
                .locality = locality,
                .chunkMode = mode,
                .fileSizeMinimum = size,
                .fileSizeMaximum = size,
                .chunkSizeMinimum = 1ULL,
                .chunkSizeMaximum = chunkMaximum,
                .buffering = buffering,
                .backBufferLimit = backBufferLimit,
                .weight = locality == Locality::Hot ? 3U : 1U,
            });
    };
    constexpr auto defaultBuffering = el::StreamBuffering::Balanced;
    constexpr auto stressBuffering = el::StreamBuffering::MinimalMemory;
    add("binary"_el, Method::ReadByte, FileType::Binary, smallSize, ChunkMode::Fixed, Locality::Hot, stressBuffering);
    add("binary"_el,
        Method::ReadSpan,
        FileType::Binary,
        bulkSize,
        ChunkMode::Incrementing,
        Locality::Hot,
        defaultBuffering);
    add("binary"_el,
        Method::ReadBlock,
        FileType::Binary,
        bulkSize,
        ChunkMode::Random,
        Locality::Rotating,
        stressBuffering);
    add("binary"_el, Method::ReadExact, FileType::Binary, bulkSize, ChunkMode::Random, Locality::Hot, defaultBuffering);
    add("binary"_el,
        Method::ReadAll,
        FileType::Binary,
        largeSize,
        ChunkMode::Fixed,
        Locality::Rotating,
        defaultBuffering);
    add("binary"_el, Method::WriteByte, FileType::Binary, smallSize, ChunkMode::Fixed, Locality::Hot, stressBuffering);
    add("binary"_el,
        Method::WriteSpan,
        FileType::Binary,
        bulkSize,
        ChunkMode::Incrementing,
        Locality::Hot,
        defaultBuffering);
    add("binary"_el,
        Method::WriteBlock,
        FileType::Binary,
        largeSize,
        ChunkMode::Random,
        Locality::Rotating,
        stressBuffering);
    constexpr auto textTypes = std::array{FileType::Utf8, FileType::Utf16, FileType::Utf32};
    constexpr auto textMethods = std::array{
        Method::ReadChar,
        Method::ReadText,
        Method::ReadLine,
        Method::ReadAllText,
        Method::WriteChar,
        Method::WriteText,
        Method::WriteLine,
    };
    auto index = std::size_t{};
    for (const auto type : textTypes) {
        for (const auto method : textMethods) {
            const auto perCharacter = method == Method::ReadChar || method == Method::WriteChar;
            const auto aggregateRead = method == Method::ReadAllText;
            add("text"_el,
                method,
                type,
                method == Method::WriteChar                                      ? characterWriteSize
                    : method == Method::WriteText || method == Method::WriteLine ? textWriteSize
                    : perCharacter || aggregateRead                              ? smallSize
                                                                                 : bulkSize,
                perCharacter || aggregateRead ? ChunkMode::Fixed
                                              : (index % 2U == 0U ? ChunkMode::Incrementing : ChunkMode::Random),
                index % 4U == 0U ? Locality::Rotating : Locality::Hot,
                index % 3U == 0U ? stressBuffering : defaultBuffering);
            ++index;
        }
    }
    return result;
}

auto parseConfigurationDocument(const ValuePtr &document, const Configuration *defaults) -> Configuration {
    auto result = defaults != nullptr ? *defaults : Configuration{};
    constexpr auto rootKeys = std::array{"run"_el, "scenario"_el};
    requireKnownKeys(document, rootKeys);
    if (document->hasValue("run"_el)) {
        const auto run = document->valueOrThrow("run"_el);
        constexpr auto runKeys = std::array{
            "mode"_el,
            "suite"_el,
            "duration"_el,
            "threads"_el,
            "seed"_el,
            "corpus_limit"_el,
            "progress_interval"_el,
            "workspace"_el,
            "keep_files"_el,
        };
        requireKnownKeys(run, runKeys);
        result.run.mode = parseMode(run->getText("mode"_el, toString(result.run.mode)));
        result.run.suite = run->getText("suite"_el, result.run.suite);
        if (run->hasValue("duration"_el)) {
            result.run.duration = run->getCalendarDeltaOrThrow("duration"_el).toTimeDeltaOrThrow().toStdNanoseconds();
        }
        const auto threads = run->getInteger("threads"_el, result.run.threadCount);
        if (threads < 1 || threads > 256) {
            configError("The workload thread count must be in the range 1-256."_el, run->valueOrThrow("threads"_el));
        }
        result.run.threadCount = static_cast<std::uint32_t>(threads);
        const auto seed = run->getInteger("seed"_el, static_cast<el::conf::Integer>(result.run.seed));
        if (seed < 0) {
            configError("The global seed must not be negative."_el, run->valueOrThrow("seed"_el));
        }
        result.run.seed = static_cast<std::uint64_t>(seed);
        result.run.corpusLimit = integerValue(run, "corpus_limit"_el, result.run.corpusLimit);
        if (run->hasValue("progress_interval"_el)) {
            result.run.progressInterval =
                run->getCalendarDeltaOrThrow("progress_interval"_el).toTimeDeltaOrThrow().toStdNanoseconds();
            if (result.run.progressInterval <= std::chrono::nanoseconds::zero()) {
                configError("The progress interval must be positive."_el, run->valueOrThrow("progress_interval"_el));
            }
        }
        result.run.workspace = run->getText("workspace"_el, result.run.workspace);
        result.run.keepFiles = run->getBoolean("keep_files"_el, result.run.keepFiles);
    }
    if (result.run.duration <= std::chrono::nanoseconds::zero()) {
        configError("The run duration must be positive."_el);
    }
    if (document->hasValue("scenario"_el)) {
        result.scenarios.clear();
        const auto scenarioList = document->valueOrThrow("scenario"_el);
        for (const auto &entry : *scenarioList) {
            appendExpanded(parseScenario(entry), result.scenarios, entry);
        }
    } else if (result.run.suite == "cover-all"_el) {
        result.scenarios = curatedSuite(false);
    } else if (result.run.suite == "smoke"_el) {
        result.scenarios = curatedSuite(true);
    } else {
        configError(el::StringFormat{"Unsupported suite '{}'."_el}.build(result.run.suite));
    }
    if (result.scenarios.empty()) {
        configError("The configuration expands to no scenarios."_el);
    }
    auto ids = std::set<el::String>{};
    for (const auto &scenario : result.scenarios) {
        if (!ids.emplace(scenario.id).second) {
            configError(el::StringFormat{"Duplicate expanded scenario ID '{}'."_el}.build(scenario.id));
        }
    }
    return result;
}

}
