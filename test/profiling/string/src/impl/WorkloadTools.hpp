// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Fairness.hpp"
#include "Statistics.hpp"
#include "WidthTraits.hpp"

#include "../Workload.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/system/EnvironmentVariables.hpp>
#include <erbsland/text/impl/UnsafeU16StringBuffer.hpp>
#include <erbsland/text/impl/UnsafeU8StringBuffer.hpp>
#include <erbsland/text/u16/U16StringConstIterator.hpp>
#include <erbsland/text/u32/U32StringConstIterator.hpp>
#include <erbsland/text/u8/U8StringConstIterator.hpp>

#include <algorithm>
#include <atomic>
#include <barrier>
#include <cmath>
#include <exception>
#include <limits>
#include <numeric>
#include <thread>
#include <type_traits>

namespace app::string::impl {

using namespace el::text::literals;

using Clock = std::chrono::steady_clock;

/// Report a workload validation error.
[[noreturn]] void workloadError(const el::String &message) {
    throw el::ApplicationError{message};
}

/// Convert a raw value into a code-point index.
[[nodiscard]] auto cpIndex(const std::uint64_t value) -> el::CpIndex {
    return el::CpIndex::fromSizeT(static_cast<std::size_t>(value));
}

/// Convert a raw value into a code-point length.
[[nodiscard]] auto cpLength(const std::uint64_t value) -> el::CpLength {
    return el::CpLength::fromSizeT(static_cast<std::size_t>(value));
}

/// Mix a value into a deterministic random seed.
[[nodiscard]] auto mixSeed(std::uint64_t seed, const std::uint64_t value) noexcept -> std::uint64_t {
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
    seed ^= seed >> 30U;
    seed *= 0xbf58476d1ce4e5b9ULL;
    seed ^= seed >> 27U;
    seed *= 0x94d049bb133111ebULL;
    return seed ^ (seed >> 31U);
}

/// Create the deterministic seed for a benchmark worker sample.
[[nodiscard]] auto workerSeed(
    const Configuration &configuration,
    const Scenario &scenario,
    const std::uint32_t worker,
    const std::uint64_t sample) noexcept -> std::uint64_t {
    auto result = mixSeed(configuration.run.seed, scenario.id.toHash());
    result = mixSeed(result, worker);
    return mixSeed(result, sample);
}

/// Calculate the elapsed time since a start point in nanoseconds.
[[nodiscard]] auto elapsedNanoseconds(const Clock::time_point start) -> std::int64_t {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
}

/// Calculate the digest of a string.
[[nodiscard]] auto digest(const el::U8String &text) -> el::ByteBlock {
    auto hasher = el::cryptology::Hasher{el::cryptology::HashAlgorithm::Md5};
    hasher.update(text);
    return hasher.finalize();
}

/// Convert a string value to canonical UTF-8 text.
template <typename T>
[[nodiscard]] auto canonicalString(const T &value) -> el::U8String {
    return el::StringConverter{value}.toU8String();
}

/// Apply the selected sensitivity mode to a string value.
template <typename T>
void setSensitive(T &value, const SensitiveMode mode) {
    if constexpr (std::same_as<T, el::U8String> || std::same_as<T, el::U8StringEditor>) {
        if (mode == SensitiveMode::Sensitive) {
            value.markAsSensitive();
        }
    }
}

/// Validate the generated source string for a scenario.
template <typename Traits>
void validateSource(const typename Traits::String &value, const Scenario &scenario) {
    const auto expectedUnits =
        nativeBytesFor(scenario.width, scenario.contentProfile, scenario.size) / Traits::UnitBytes;
    if (Traits::dataLength(value) != expectedUnits ||
        static_cast<std::uint64_t>(value.characterLength().toRawValue()) != scenario.size) {
        workloadError("Generated string corpus has an unexpected native or decoded length."_el);
    }
    const auto malformed = scenario.contentProfile == ContentProfile::MalformedSparse ||
        scenario.contentProfile == ContentProfile::MalformedDense;
    const auto expectedValid = !malformed || scenario.size == 0U;
    if (Traits::isValid(value) != expectedValid) {
        workloadError("Generated string corpus has an unexpected encoding-validity state."_el);
    }
}

/// Create a deterministic source character for a content profile.
[[nodiscard]] auto sourceCharacter(el::FastRandom &random, const ContentProfile profile, const std::uint64_t index)
    -> el::Char {
    if (profile == ContentProfile::Ascii || profile == ContentProfile::MalformedSparse ||
        profile == ContentProfile::MalformedDense) {
        constexpr auto characters = std::array{U'a', U'b', U'c', U'd', U'e', U' ', U'.', U'-'};
        return el::Char{characters[random.getUInt32(0U, static_cast<std::uint32_t>(characters.size() - 1U))]};
    }
    if (profile == ContentProfile::Supplementary) {
        return el::Char{static_cast<char32_t>(0x1f300U + random.getUInt32(0U, 0x34fU))};
    }
    const auto position = index % 100U;
    if (position < 72U) {
        return el::Char{static_cast<char32_t>(U'a' + random.getUInt32(0U, 25U))};
    }
    if (position < 80U) {
        constexpr auto punctuation = std::array{U' ', U'.', U',', U':', U';', U'-', U'!', U'?'};
        return el::Char{punctuation[random.getUInt32(0U, static_cast<std::uint32_t>(punctuation.size() - 1U))]};
    }
    if (position < 92U) {
        return el::Char{static_cast<char32_t>(0x00c0U + random.getUInt32(0U, 0x23fU))};
    }
    if (position < 98U) {
        return el::Char{static_cast<char32_t>(0x4e00U + random.getUInt32(0U, 0x51ffU))};
    }
    return el::Char{static_cast<char32_t>(0x1f300U + random.getUInt32(0U, 0x34fU))};
}

/// Create a valid string for a requested string width.
template <typename Traits>
[[nodiscard]] auto makeValidString(const std::uint64_t count, const ContentProfile profile, const std::uint64_t seed) ->
    typename Traits::String {
    auto random = el::FastRandom{seed};
    auto source = el::U32StringEditor{};
    source.reserve(cpLength(count));
    for (auto index = std::uint64_t{}; index < count; ++index) {
        source.append(sourceCharacter(random, profile, index));
    }
    if constexpr (Traits::Width == StringWidth::U8) {
        return el::StringConverter{el::U32String{source}}.toU8String();
    } else if constexpr (Traits::Width == StringWidth::U16) {
        return el::StringConverter{el::U32String{source}}.toU16String();
    } else {
        return el::U32String{source};
    }
}

/// Create a deliberately malformed string for a requested string width.
template <typename Traits>
[[nodiscard]] auto makeMalformedString(
    const std::uint64_t count, const ContentProfile profile, const std::uint64_t seed) -> typename Traits::String {
    const auto interval = profile == ContentProfile::MalformedDense ? 3U : 97U;
    auto random = el::FastRandom{seed};
    if constexpr (Traits::Width == StringWidth::U8) {
        auto text = el::text::impl::UnsafeU8StringBuffer{el::ByteLength{count}};
        for (auto index = std::uint64_t{}; index < count; ++index) {
            text.data()[index] =
                index % interval == 0U ? static_cast<char>(0x80U) : static_cast<char>('a' + random.getUInt32(0U, 25U));
        }
        return el::U8String{text.take(el::ByteLength{count})};
    } else if constexpr (Traits::Width == StringWidth::U16) {
        const auto length = el::U16DataLength::fromSizeTOrThrow(static_cast<std::size_t>(count));
        auto text = el::text::impl::UnsafeU16StringBuffer{length};
        for (auto index = std::uint64_t{}; index < count; ++index) {
            text.data()[index] = index % interval == 0U ? static_cast<char16_t>(0xd800U)
                                                        : static_cast<char16_t>(u'a' + random.getUInt32(0U, 25U));
        }
        return el::U16String{text.take(length)};
    } else {
        auto text = std::u32string{};
        text.reserve(static_cast<std::size_t>(count));
        for (auto index = std::uint64_t{}; index < count; ++index) {
            text.push_back(
                index % interval == 0U ? static_cast<char32_t>(0x110000U)
                                       : static_cast<char32_t>(U'a' + random.getUInt32(0U, 25U)));
        }
        return el::U32String{std::u32string_view{text}};
    }
}

/// Create the source string for a workload scenario.
template <typename Traits>
[[nodiscard]] auto makeString(const Scenario &scenario, const std::uint64_t seed) -> typename Traits::String {
    if (scenario.contentProfile == ContentProfile::MalformedSparse ||
        scenario.contentProfile == ContentProfile::MalformedDense) {
        return makeMalformedString<Traits>(scenario.size, scenario.contentProfile, seed);
    }
    return makeValidString<Traits>(scenario.size, scenario.contentProfile, seed);
}

/// Create the operand string for a workload scenario.
template <typename Traits>
[[nodiscard]] auto makeOperand(const Scenario &scenario, const std::uint64_t seed) -> typename Traits::String {
    auto operandScenario = scenario;
    operandScenario.size = std::min(scenario.size, scenario.operandSize);
    operandScenario.variant = "operand"_el;
    return makeString<Traits>(operandScenario, seed);
}

/// Select the search needle for a workload scenario.
template <typename Traits>
[[nodiscard]] auto makeNeedle(const typename Traits::String &source, const Scenario &scenario) ->
    typename Traits::String {
    const auto sourceCount = static_cast<std::uint64_t>(source.characterLength().toRawValue());
    if (sourceCount == 0U) {
        return source;
    }
    const auto count = std::max<std::uint64_t>(1U, std::min(scenario.operandSize, sourceCount));
    if (scenario.variant == "find-miss"_el) {
        return Traits::String::fromCharacter(el::Char{U'\u0001'}, cpLength(count));
    }
    if (scenario.variant == "find-adversarial"_el) {
        auto result = typename Traits::Editor{source.slice(el::CpRange{el::CpIndex::zero(), cpLength(count)})};
        result.replace(
            el::CpRange{cpIndex(count - 1U), el::CpLength::one()}, Traits::String::fromCharacter(el::Char{U'\u0001'}));
        return typename Traits::String{result};
    }
    if (scenario.variant == "count-many"_el) {
        return source.slice(el::CpRange{el::CpIndex::zero(), el::CpLength::one()});
    }
    const auto back = scenario.variant == "find-hit-back"_el || scenario.variant == "ends-with-hit"_el ||
        scenario.variant == "contains-hit-back"_el;
    const auto start = back && sourceCount > count ? sourceCount - count : 0U;
    return source.slice(el::CpRange{cpIndex(start), cpLength(count)});
}

/// Return a character without changing it.
[[nodiscard]] auto identityTransform(const el::Char character) noexcept -> el::Char {
    return character;
}

/// Transform the designated character to a different character.
[[nodiscard]] auto changedTransform(const el::Char character) noexcept -> el::Char {
    return character == U'a' ? el::Char{U'A'} : character;
}

/// Mix a character into the benchmark result sink.
void consumeChar(std::uint64_t &sink, const el::Char value) noexcept {
    sink = mixSeed(sink, value.toRawValue());
}

/// Execute read-focused operations for one string width.
template <typename Traits, typename Value>
[[nodiscard]] auto executeReadable(const Scenario &scenario, const std::uint64_t seed, const std::uint64_t operations)
    -> WorkerResult {
    using String = typename Traits::String;
    using Editor = typename Traits::Editor;
    constexpr auto isEditor = std::same_as<Value, Editor>;
    auto sourceString = makeString<Traits>(scenario, seed);
    validateSource<Traits>(sourceString, scenario);
    setSensitive(sourceString, scenario.sensitiveMode);
    auto value = [&]() -> Value {
        if constexpr (isEditor) {
            auto result = Editor{sourceString};
            setSensitive(result, scenario.sensitiveMode);
            return result;
        } else {
            return sourceString;
        }
    }();
    auto differentEditor = Editor{sourceString};
    if (!differentEditor.isEmpty()) {
        const auto differentIndex = scenario.variant == "different-back"_el
            ? cpIndex(static_cast<std::uint64_t>(sourceString.characterLength().toRawValue()) - 1U)
            : el::CpIndex::zero();
        differentEditor.replace(
            el::CpRange{differentIndex, el::CpLength::one()}, String::fromCharacter(el::Char{U'Z'}));
    }
    const auto different = String{differentEditor};
    const auto needle = makeNeedle<Traits>(sourceString, scenario);
    const auto sourceDigest = digest(canonicalString(sourceString));
    auto sink = seed;
    auto finalText = sourceString;
    const auto cpCount = static_cast<std::uint64_t>(sourceString.characterLength().toRawValue());
    const auto dataCount = Traits::dataLength(sourceString);
    auto nativeIndices = std::vector<typename Traits::DataIndex>{};
    if (scenario.useCase == UseCase::ReadIndexed &&
        (scenario.variant == "native-sequential"_el || scenario.variant == "to-char-index"_el)) {
        nativeIndices.reserve(static_cast<std::size_t>(cpCount));
        for (auto index = std::uint64_t{}; index < cpCount; ++index) {
            nativeIndices.emplace_back(Traits::dataIndex(value, index));
        }
    }
    auto moveFixtures = std::vector<Value>{};
    if (scenario.useCase == UseCase::Move) {
        moveFixtures.resize(static_cast<std::size_t>(operations), value);
    }
    const auto start = Clock::now();
    for (auto iteration = std::uint64_t{}; iteration < operations; ++iteration) {
        switch (scenario.useCase) {
        case UseCase::Create:
            if (scenario.variant == "from-character"_el) {
                finalText = String::fromCharacter(el::Char{U'x'}, cpLength(scenario.size));
            } else if constexpr (isEditor) {
                auto created = Editor{sourceString};
                sink = mixSeed(sink, Traits::dataLength(created));
                finalText = String{created};
            } else {
                finalText = sourceString.copy();
            }
            break;
        case UseCase::Copy: {
            auto copy = value;
            if (scenario.variant == "compact"_el) {
                if constexpr (!isEditor) {
                    copy = value.copy();
                }
            }
            sink = mixSeed(sink, Traits::dataLength(copy));
            break;
        }
        case UseCase::Move: {
            auto moved = std::move(moveFixtures[static_cast<std::size_t>(iteration)]);
            sink = mixSeed(sink, Traits::dataLength(moved));
            break;
        }
        case UseCase::TypeConvert:
            if constexpr (isEditor) {
                finalText = String{value};
            } else {
                auto editor = Editor{value};
                sink = mixSeed(sink, Traits::dataLength(editor));
                finalText = String{editor};
            }
            break;
        case UseCase::WidthConvert:
            if (scenario.variant == "to-u8"_el) {
                sink = mixSeed(sink, el::StringConverter{value}.toU8String().length().toRawValue());
            } else if (scenario.variant == "to-u16"_el) {
                sink = mixSeed(sink, el::StringConverter{value}.toU16String().length().toRawValue());
            } else {
                sink = mixSeed(sink, el::StringConverter{value}.toU32String().length().toRawValue());
            }
            break;
        case UseCase::SliceSplit: {
            const auto count = cpCount / 2U;
            if (scenario.variant == "native-middle"_el) {
                const auto slice = value.slice(Traits::dataRange(value, (cpCount - count) / 2U, count));
                sink = mixSeed(sink, Traits::dataLength(slice));
            } else if (scenario.variant == "code-point-back"_el) {
                const auto slice = value.slice(el::StringSide::Back, cpLength(count));
                sink = mixSeed(sink, Traits::dataLength(slice));
            } else if (scenario.variant == "split-middle"_el) {
                const auto [first, second] = value.splitAt(cpIndex(cpCount / 2U));
                sink = mixSeed(sink, Traits::dataLength(first) + Traits::dataLength(second));
            } else {
                const auto slice = value.slice(el::CpRange{cpIndex((cpCount - count) / 2U), cpLength(count)});
                sink = mixSeed(sink, Traits::dataLength(slice));
            }
            break;
        }
        case UseCase::Inspect:
            if (scenario.variant == "character-length"_el) {
                sink = mixSeed(sink, value.characterLength().toRawValue());
            } else if (scenario.variant == "display-width"_el) {
                sink = mixSeed(sink, static_cast<std::uint64_t>(value.displayWidth()));
            } else {
                sink = mixSeed(sink, Traits::isValid(value) ? 1U : 2U);
            }
            break;
        case UseCase::ReadIndexed: {
            if (cpCount == 0U) {
                consumeChar(sink, value.charAt(el::StringSide::Front));
                break;
            }
            const auto characterIndex =
                scenario.variant == "code-point-random"_el ? mixSeed(iteration, seed) % cpCount : iteration % cpCount;
            if (scenario.variant == "native-sequential"_el) {
                consumeChar(sink, value.charAt(nativeIndices[static_cast<std::size_t>(characterIndex)]));
            } else if (scenario.variant == "index-at"_el) {
                sink = mixSeed(sink, Traits::dataIndex(value, characterIndex).toRawValue());
            } else if (scenario.variant == "to-char-index"_el) {
                sink = mixSeed(
                    sink, Traits::characterIndex(value, nativeIndices[static_cast<std::size_t>(characterIndex)]));
            } else {
                consumeChar(sink, value.charAt(cpIndex(characterIndex)));
            }
            break;
        }
        case UseCase::Traverse:
            if (scenario.variant == "reader-forward"_el) {
                auto index = Traits::dataIndex(value, 0U);
                const auto end = Traits::dataIndex(value, cpCount);
                while (index != end) {
                    consumeChar(sink, value.readCharAndAdvance(index));
                }
            } else if (scenario.variant == "reader-backward"_el) {
                auto index = Traits::dataIndex(value, cpCount);
                const auto begin = Traits::dataIndex(value, 0U);
                while (index != begin) {
                    consumeChar(sink, value.readCharAndRetreat(index));
                }
            } else if (scenario.variant == "iterator"_el) {
                for (const auto character : value) {
                    consumeChar(sink, character);
                }
            } else {
                const auto loopResult = value.forEach([&](const el::Char character) {
                    consumeChar(sink, character);
                    return el::LoopStatus::Continue;
                });
                sink = mixSeed(sink, static_cast<std::uint8_t>(loopResult));
            }
            break;
        case UseCase::Compare:
            if (scenario.variant == "case-folded"_el) {
                sink = mixSeed(sink, value.compare(sourceString, el::Char::compareCaseFolded) == 0 ? 1U : 2U);
            } else {
                sink = mixSeed(
                    sink, value.compare(scenario.variant == "equal"_el ? sourceString : different) == 0 ? 1U : 2U);
            }
            break;
        case UseCase::Hash:
            sink = mixSeed(sink, scenario.variant == "case-folded"_el ? value.toHashCI() : value.toHash());
            break;
        case UseCase::Search:
            if (scenario.variant == "find-first-of"_el) {
                const auto characters = el::CharSet{"aeiou"_el};
                sink = mixSeed(sink, value.findFirstOf(characters).toRawValue());
            } else if (scenario.variant == "find-last-of"_el) {
                sink = mixSeed(sink, value.findLastOf(el::CharSet{"aeiou"_el}).toRawValue());
            } else if (scenario.variant == "starts-with-hit"_el) {
                sink = mixSeed(sink, value.startsWith(needle) ? 1U : 2U);
            } else if (scenario.variant == "ends-with-hit"_el) {
                sink = mixSeed(sink, value.endsWith(needle) ? 1U : 2U);
            } else if (scenario.variant == "contains-hit-back"_el) {
                sink = mixSeed(sink, value.contains(needle) ? 1U : 2U);
            } else if (scenario.variant == "count-many"_el) {
                sink = mixSeed(sink, value.count(needle).toRawValue());
            } else {
                const auto found = value.find(needle);
                sink = mixSeed(sink, found.isValid() ? found.toRawValue() : std::numeric_limits<std::uint64_t>::max());
            }
            break;
        case UseCase::Transform:
            if (scenario.variant == "unchanged"_el) {
                finalText = String{value.transformed(identityTransform)};
            } else if (scenario.variant == "changed"_el) {
                finalText = String{value.transformed(changedTransform)};
            } else if (scenario.variant == "truncate"_el) {
                finalText = String{value.truncated(cpLength(cpCount / 2U), el::TruncateMode::Middle)};
            } else {
                finalText = String{value.aligned(cpLength(cpCount + 16U), el::geometry::Alignment::HCenter)};
            }
            sink = mixSeed(sink, Traits::dataLength(finalText));
            break;
        case UseCase::EscapeSafe:
            if (scenario.variant == "escaped-size"_el) {
                sink = mixSeed(sink, value.escapedSize(el::EscapeFormat::Json).toRawValue());
            } else if (scenario.variant == "to-escaped"_el) {
                finalText = String{value.toEscaped(el::EscapeFormat::Json)};
            } else {
                finalText = String{value.toSafeString(cpLength(std::max<std::uint64_t>(1U, cpCount)))};
            }
            if (scenario.variant != "escaped-size"_el) {
                sink = mixSeed(sink, Traits::dataLength(finalText));
            }
            break;
        case UseCase::Join:
            if constexpr (isEditor) {
                finalText = String{Editor::fromJoined({sourceString, sourceString, sourceString, sourceString})};
            } else {
                finalText = String::fromJoined({sourceString, sourceString, sourceString, sourceString});
            }
            sink = mixSeed(sink, Traits::dataLength(finalText));
            break;
        case UseCase::SensitiveStorage:
            if constexpr (Traits::Width == StringWidth::U8) {
                auto marked = value;
                marked.markAsSensitive();
                if (scenario.variant == "copy-release"_el) {
                    auto copy = marked;
                    sink = mixSeed(sink, copy.isSensitive() ? 1U : 2U);
                } else {
                    sink = mixSeed(sink, marked.isSensitive() ? 1U : 2U);
                }
            }
            break;
        default:
            workloadError("Unsupported read-only string workload path."_el);
        }
    }
    const auto elapsed = elapsedNanoseconds(start);
    if (digest(canonicalString(sourceString)) != sourceDigest) {
        workloadError("A string workload modified its immutable baseline."_el);
    }
    const auto perOperationCodePoints =
        scenario.workUnit == WorkUnit::Operations ? 0U : std::max<std::uint64_t>(1U, cpCount);
    const auto perOperationBytes = scenario.workUnit == WorkUnit::Operations ? 0U : dataCount * Traits::UnitBytes;
    return WorkerResult{
        .operations = operations,
        .logicalCodePoints = perOperationCodePoints * operations,
        .logicalNativeBytes = perOperationBytes * operations,
        .nanoseconds = elapsed,
        .sink = sink,
        .digest = digest(canonicalString(finalText))};
}

/// Apply mutation operations to a string editor.
template <typename Traits>
void mutateEditor(
    typename Traits::Editor &value,
    const typename Traits::String &operand,
    const Scenario &scenario,
    const std::uint64_t iteration,
    std::uint64_t &sink) {
    using String = typename Traits::String;
    const auto cpCount = static_cast<std::uint64_t>(value.characterLength().toRawValue());
    const auto operandCount = static_cast<std::uint64_t>(operand.characterLength().toRawValue());
    const auto rangeCount = std::min(cpCount, std::max<std::uint64_t>(1U, operandCount));
    const auto middle = cpCount > rangeCount ? (cpCount - rangeCount) / 2U : 0U;
    if (scenario.useCase == UseCase::Storage) {
        if (scenario.variant == "clear"_el) {
            value.clear();
        } else if (scenario.variant == "reset"_el) {
            value.reset();
        } else if (scenario.variant == "reserve"_el) {
            value.reserve(
                Traits::DataLength::fromSizeT(
                    static_cast<std::size_t>(Traits::dataLength(value) + Traits::dataLength(operand))));
        } else if (scenario.variant == "shrink"_el) {
            value.shrinkToFit();
        } else if (scenario.variant == "detach-shared"_el) {
            const auto alias = value;
            sink = mixSeed(sink, Traits::dataLength(alias));
            value.detach();
        } else {
            value = value.slice(el::CpRange{cpIndex(middle), cpLength(rangeCount)});
            value.shrinkToFit();
        }
    } else if (scenario.useCase == UseCase::Append) {
        if (scenario.variant == "character-growing"_el) {
            value.append(el::Char{U'x'});
        } else if (scenario.variant == "text-reserved"_el) {
            value.append(operand);
        } else if (scenario.variant == "aliased"_el) {
            const auto alias = String{value}.slice(el::CpRange{cpIndex(middle), cpLength(rangeCount)});
            value.append(alias);
        } else {
            value.append(operand);
        }
    } else if (scenario.useCase == UseCase::Insert) {
        if (scenario.variant == "native-front"_el) {
            value.insert(Traits::dataIndex(value, 0U), operand);
        } else if (scenario.variant == "native-back"_el) {
            value.insert(Traits::dataIndex(value, cpCount), operand);
        } else if (scenario.variant == "code-point-middle"_el) {
            value.insert(cpIndex(middle), operand);
        } else if (scenario.variant == "aliased"_el) {
            const auto alias = String{value}.slice(el::CpRange{cpIndex(middle), cpLength(rangeCount)});
            value.insert(Traits::dataIndex(value, middle), alias);
        } else {
            value.insert(Traits::dataIndex(value, middle), operand);
        }
    } else if (scenario.useCase == UseCase::Replace) {
        if (scenario.variant == "all-dense"_el) {
            value.replaceAll(el::CharSet{"aeiou"_el}, el::Char{U'x'});
        } else if (scenario.variant == "code-point-middle"_el) {
            value.replace(el::CpRange{cpIndex(middle), cpLength(rangeCount)}, operand);
        } else if (scenario.variant == "aliased"_el) {
            const auto alias = String{value}.slice(el::CpRange{cpIndex(middle), cpLength(rangeCount)});
            value.replace(Traits::dataRange(value, middle, rangeCount), alias);
        } else if (scenario.variant == "native-grow"_el) {
            value.replace(Traits::dataRange(value, middle, rangeCount / 2U), operand);
        } else if (scenario.variant == "native-shrink"_el) {
            value.replace(Traits::dataRange(value, middle, rangeCount), String::fromCharacter(U'x'));
        } else {
            value.replace(Traits::dataRange(value, middle, rangeCount), operand);
        }
    } else if (scenario.useCase == UseCase::RemoveKeep) {
        if (scenario.variant == "all-dense"_el) {
            value.removeAll(el::CharSet{"aeiou"_el});
        } else if (scenario.variant == "keep-middle"_el) {
            value.keep(el::CpRange{cpIndex(middle), cpLength(rangeCount)});
        } else {
            const auto start = scenario.variant == "remove-front"_el ? 0U : middle;
            value.remove(Traits::dataRange(value, start, rangeCount));
        }
    } else if (scenario.useCase == UseCase::Trim) {
        if (scenario.variant == "character-set"_el) {
            value.trim(el::CharSet{"aeiou"_el});
        } else {
            value.trim();
        }
    } else if (scenario.useCase == UseCase::Truncate) {
        const auto target = cpLength(cpCount / 2U);
        if (scenario.variant == "middle"_el) {
            value.truncate(target, el::TruncateMode::Middle);
        } else if (scenario.variant == "ellipsis"_el) {
            value.truncate(target, el::TruncateMode::Middle, String::fromCharacter(U'\u2026'));
        } else {
            value.truncate(target, el::TruncateMode::End);
        }
    } else if (scenario.useCase == UseCase::CowStress) {
        auto aliases = std::array{value, value, value, value, value, value, value, value};
        if (scenario.variant == "sliced-fanout"_el) {
            for (auto &alias : aliases) {
                alias = alias.slice(el::CpRange{cpIndex(middle), cpLength(rangeCount)});
            }
        }
        aliases[iteration % aliases.size()].append(operand);
        value = std::move(aliases[iteration % aliases.size()]);
    } else if (scenario.useCase == UseCase::EditStress) {
        value.append(operand);
        value.insert(el::CpIndex::zero(), operand);
        const auto current = static_cast<std::uint64_t>(value.characterLength().toRawValue());
        const auto replaceCount = std::min<std::uint64_t>(operandCount, current / 4U);
        value.replace(el::CpRange{cpIndex(current / 3U), cpLength(replaceCount)}, operand);
        value.remove(
            el::CpRange{
                el::CpIndex::zero(),
                cpLength(std::min(operandCount, static_cast<std::uint64_t>(value.characterLength().toRawValue())))});
        value.truncate(cpLength(scenario.size), el::TruncateMode::Middle);
    } else if (scenario.useCase == UseCase::SensitiveStorage) {
        if constexpr (Traits::Width == StringWidth::U8) {
            value.markAsSensitive();
            if (scenario.variant == "detach"_el) {
                const auto alias = value;
                sink = mixSeed(sink, alias.isSensitive() ? 1U : 2U);
                value.detach();
            } else if (scenario.variant == "reset-release"_el) {
                value.reset();
            }
        }
    } else {
        workloadError("Unsupported StringEditor workload path."_el);
    }
}

/// Execute mutation operations for a string editor.
template <typename Traits>
[[nodiscard]] auto executeEditorMutation(
    const Scenario &scenario, const std::uint64_t seed, const std::uint64_t operations) -> WorkerResult {
    using Editor = typename Traits::Editor;
    auto source = makeString<Traits>(scenario, seed);
    validateSource<Traits>(source, scenario);
    setSensitive(source, scenario.sensitiveMode);
    const auto operand = makeOperand<Traits>(scenario, mixSeed(seed, 1U));
    const auto sourceDigest = digest(canonicalString(source));
    auto fixtures = std::vector<Editor>{};
    fixtures.reserve(static_cast<std::size_t>(operations));
    for (auto iteration = std::uint64_t{}; iteration < operations; ++iteration) {
        fixtures.emplace_back(source);
        setSensitive(fixtures.back(), scenario.sensitiveMode);
        const auto needsSharedStorage = scenario.useCase == UseCase::CowStress ||
            (scenario.useCase == UseCase::Storage && scenario.variant == "detach-shared"_el) ||
            (scenario.useCase == UseCase::SensitiveStorage && scenario.variant == "detach"_el);
        if (!needsSharedStorage) {
            fixtures.back().detach();
        }
        if (scenario.useCase == UseCase::Append && scenario.variant == "text-reserved"_el) {
            fixtures.back().reserve(
                Traits::DataLength::fromSizeT(
                    static_cast<std::size_t>(Traits::dataLength(source) + Traits::dataLength(operand))));
        }
    }
    auto sink = seed;
    auto capacityChanges = std::uint64_t{};
    const auto start = Clock::now();
    for (auto iteration = std::uint64_t{}; iteration < operations; ++iteration) {
        auto &value = fixtures[static_cast<std::size_t>(iteration)];
        const auto oldCapacity = value.capacity().toRawValue();
        mutateEditor<Traits>(value, operand, scenario, iteration, sink);
        capacityChanges += value.capacity().toRawValue() != oldCapacity ? 1U : 0U;
        sink = mixSeed(sink, Traits::dataLength(value));
    }
    const auto elapsed = elapsedNanoseconds(start);
    if (digest(canonicalString(source)) != sourceDigest) {
        workloadError("A StringEditor workload modified its source baseline."_el);
    }
    const auto finalText = fixtures.empty() ? source : typename Traits::String{fixtures.back()};
    const auto perOperationCodePoints = scenario.workUnit == WorkUnit::Operations
        ? 0U
        : std::max<std::uint64_t>(
              1U,
              scenario.useCase == UseCase::Append || scenario.useCase == UseCase::Insert ||
                      scenario.useCase == UseCase::Replace
                  ? scenario.operandSize
                  : scenario.size);
    const auto perOperationBytes = scenario.workUnit == WorkUnit::Operations
        ? 0U
        : (scenario.useCase == UseCase::Append || scenario.useCase == UseCase::Insert ||
                      scenario.useCase == UseCase::Replace
                  ? Traits::dataLength(operand)
                  : Traits::dataLength(source)) *
            Traits::UnitBytes;
    return WorkerResult{
        .operations = operations,
        .logicalCodePoints = perOperationCodePoints * operations,
        .logicalNativeBytes = perOperationBytes * operations,
        .capacityChanges = capacityChanges,
        .nanoseconds = elapsed,
        .sink = sink,
        .digest = digest(canonicalString(finalText))};
}

/// Test whether a use case mutates a string editor.
[[nodiscard]] auto isEditorMutation(const UseCase useCase) noexcept -> bool {
    return useCase == UseCase::Storage || useCase == UseCase::Append || useCase == UseCase::Insert ||
        useCase == UseCase::Replace || useCase == UseCase::RemoveKeep || useCase == UseCase::Trim ||
        useCase == UseCase::Truncate || useCase == UseCase::CowStress || useCase == UseCase::EditStress;
}

/// Execute the workload with the selected string width.
template <typename Traits>
[[nodiscard]] auto executeForWidth(const Scenario &scenario, const std::uint64_t seed, const std::uint64_t operations)
    -> WorkerResult {
    if (scenario.type == StringType::StringEditor &&
        (isEditorMutation(scenario.useCase) || scenario.useCase == UseCase::SensitiveStorage)) {
        return executeEditorMutation<Traits>(scenario, seed, operations);
    }
    if (scenario.type == StringType::StringEditor) {
        return executeReadable<Traits, typename Traits::Editor>(scenario, seed, operations);
    }
    return executeReadable<Traits, typename Traits::String>(scenario, seed, operations);
}

/// Execute the requested string workload.
[[nodiscard]] auto executeWorkload(const Scenario &scenario, const std::uint64_t seed, const std::uint64_t operations)
    -> WorkerResult {
    switch (scenario.width) {
    case StringWidth::U8:
        return executeForWidth<WidthTraits<StringWidth::U8>>(scenario, seed, operations);
    case StringWidth::U16:
        return executeForWidth<WidthTraits<StringWidth::U16>>(scenario, seed, operations);
    case StringWidth::U32:
        return executeForWidth<WidthTraits<StringWidth::U32>>(scenario, seed, operations);
    }
    workloadError("Unsupported string width."_el);
}

/// Determine the largest permitted operation count for a scenario.
[[nodiscard]] auto maximumOperations(const Configuration &configuration, const Scenario &scenario) -> std::uint64_t {
    constexpr auto absoluteMaximum = std::uint64_t{4096U};
    if (scenario.type == StringType::String || !isEditorMutation(scenario.useCase)) {
        return absoluteMaximum;
    }
    const auto fixtureBytes =
        std::max<std::uint64_t>(64U, nativeBytesFor(scenario.width, scenario.contentProfile, scenario.size) + 128U);
    const auto perWorkerLimit =
        configuration.run.memoryLimit / std::max<std::uint64_t>(1U, configuration.run.threadCount);
    return std::max<std::uint64_t>(1U, std::min(absoluteMaximum, perWorkerLimit / fixtureBytes));
}

/// Calibrate the operation count for one scenario.
[[nodiscard]] auto calibrateOperations(const Configuration &configuration, const Scenario &scenario) -> std::uint64_t {
    const auto maximum = maximumOperations(configuration, scenario);
    auto operations = std::uint64_t{1U};
    while (operations < maximum) {
        const auto result = executeWorkload(scenario, workerSeed(configuration, scenario, 0U, 0U), operations);
        if (result.nanoseconds >= configuration.run.minimumSampleTime.count()) {
            break;
        }
        const auto elapsed = std::max<std::int64_t>(1, result.nanoseconds);
        const auto ratio = std::clamp<std::uint64_t>(
            static_cast<std::uint64_t>(configuration.run.minimumSampleTime.count() / elapsed), 2U, 16U);
        operations = std::min(maximum, operations > maximum / ratio ? maximum : operations * ratio);
    }
    return operations;
}

/// Execute one benchmark sample.
auto runSample(
    const Configuration &configuration,
    const Scenario &scenario,
    const std::uint64_t sampleIndex,
    const std::uint64_t operations) -> SampleResult {
    const auto threadCount = configuration.run.threadCount;
    const auto failureVariable = el::system::EnvironmentVariables{}.get("ERBSLAND_STRING_PROFILE_TEST_FAIL_WORKER"_el);
    const auto injectWorkerFailure = failureVariable.has_value() && *failureVariable == "1"_el;
    auto results = std::vector<WorkerResult>(threadCount);
    auto failures = std::vector<std::exception_ptr>(threadCount);
    auto startBarrier = std::barrier{static_cast<std::ptrdiff_t>(threadCount + 1U)};
    auto finishBarrier = std::barrier{static_cast<std::ptrdiff_t>(threadCount + 1U)};
    auto threads = std::vector<std::jthread>{};
    threads.reserve(threadCount);
    for (auto worker = std::uint32_t{}; worker < threadCount; ++worker) {
        threads.emplace_back([&, worker]() {
            try {
                startBarrier.arrive_and_wait();
                if (injectWorkerFailure && worker == threadCount - 1U) {
                    workloadError("Injected string profiler worker failure."_el);
                }
                results[worker] =
                    executeWorkload(scenario, workerSeed(configuration, scenario, worker, sampleIndex), operations);
            } catch (...) {
                failures[worker] = std::current_exception();
            }
            finishBarrier.arrive_and_wait();
        });
    }
    startBarrier.arrive_and_wait();
    const auto wallStart = Clock::now();
    finishBarrier.arrive_and_wait();
    const auto wallNanoseconds = elapsedNanoseconds(wallStart);
    for (const auto &failure : failures) {
        if (failure) {
            std::rethrow_exception(failure);
        }
    }
    auto result = SampleResult{.workers = std::move(results), .wallNanoseconds = wallNanoseconds};
    for (const auto &worker : result.workers) {
        if (worker.operations != operations || worker.digest.length().isZero()) {
            workloadError("A string profiler worker produced an invalid validation result."_el);
        }
        result.operations += worker.operations;
        result.logicalCodePoints += worker.logicalCodePoints;
        result.logicalNativeBytes += worker.logicalNativeBytes;
        result.capacityChanges += worker.capacityChanges;
    }
    return result;
}

/// Calculate statistics for recorded sample values.
[[nodiscard]] auto statistics(std::vector<double> values) -> Statistics {
    std::ranges::sort(values);
    const auto mean = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
    return Statistics{
        .minimum = values.front(),
        .median = values[values.size() / 2U],
        .mean = mean,
        .p95 = values[std::min(values.size() - 1U, (values.size() * 95U) / 100U)],
        .maximum = values.back()};
}

/// Calculate the fairness information for benchmark samples.
[[nodiscard]] auto fairness(const std::vector<SampleResult> &samples) -> Fairness {
    auto rates = std::vector<double>{};
    for (const auto &sample : samples) {
        for (const auto &worker : sample.workers) {
            rates.emplace_back(
                static_cast<double>(worker.operations) * 1.0e9 /
                static_cast<double>(std::max<std::int64_t>(1, worker.nanoseconds)));
        }
    }
    const auto mean = std::accumulate(rates.begin(), rates.end(), 0.0) / static_cast<double>(rates.size());
    auto variance = 0.0;
    for (const auto value : rates) {
        const auto delta = value - mean;
        variance += delta * delta;
    }
    variance /= static_cast<double>(rates.size());
    return Fairness{
        .minimum = *std::ranges::min_element(rates),
        .maximum = *std::ranges::max_element(rates),
        .coefficientOfVariation = mean == 0.0 ? 0.0 : std::sqrt(variance) / mean};
}

/// Calculate a digest that identifies a benchmark configuration.
[[nodiscard]] auto configurationDigest(const Configuration &configuration) -> el::ByteBlock {
    auto text = el::StringEditor{};
    text.append(
        el::StringFormat{"{} {} {} {} {} {} {} {} {}\n"_el}.build(
            toString(configuration.run.mode),
            configuration.run.suite,
            configuration.run.duration.count(),
            configuration.run.threadCount,
            configuration.run.seed,
            configuration.run.warmupSamples,
            configuration.run.samples,
            configuration.run.minimumSampleTime.count(),
            toString(configuration.run.sensitiveSelection)));
    for (const auto &scenario : configuration.scenarios) {
        text.append(
            el::StringFormat{"{} {} {} {} {} {} {} {} {} {}\n"_el}.build(
                scenario.id,
                toString(scenario.width),
                toString(scenario.type),
                toString(scenario.useCase),
                scenario.variant,
                toString(scenario.contentProfile),
                toString(scenario.sizeMode),
                toString(scenario.sensitiveMode),
                scenario.size,
                scenario.operandSize));
    }
    return digest(el::String{text});
}

/// Print one benchmark sample.
void printSample(const Scenario &scenario, const SampleResult &sample) {
    const auto safeTime = static_cast<double>(std::max<std::int64_t>(1, sample.wallNanoseconds));
    const auto safeOperations = static_cast<double>(std::max<std::uint64_t>(1U, sample.operations));
    el::io::printLine(
        "record=sample scenario="_el,
        scenario.id,
        " width="_el,
        toString(scenario.width),
        " type="_el,
        toString(scenario.type),
        " use-case="_el,
        toString(scenario.useCase),
        " variant="_el,
        scenario.variant,
        " content="_el,
        toString(scenario.contentProfile),
        " sensitive="_el,
        toString(scenario.sensitiveMode),
        " size-cp="_el,
        scenario.size,
        " native-size="_el,
        nativeBytesFor(scenario.width, scenario.contentProfile, scenario.size) /
            (scenario.width == StringWidth::U8           ? 1U
                    : scenario.width == StringWidth::U16 ? 2U
                                                         : 4U),
        " operations="_el,
        sample.operations,
        " logical-code-points="_el,
        sample.logicalCodePoints,
        " logical-native-bytes="_el,
        sample.logicalNativeBytes,
        " capacity-changes="_el,
        sample.capacityChanges,
        " wall-ns="_el,
        sample.wallNanoseconds,
        " ns-per-operation="_el,
        safeTime / safeOperations,
        " operations-per-second="_el,
        safeOperations * 1.0e9 / safeTime,
        " code-points-per-second="_el,
        static_cast<double>(sample.logicalCodePoints) * 1.0e9 / safeTime,
        " mib-per-second="_el,
        static_cast<double>(sample.logicalNativeBytes) * 1.0e9 / safeTime / (1024.0 * 1024.0));
}

/// Print the results of a benchmark scenario.
void printBenchmark(const Scenario &scenario, const std::vector<SampleResult> &samples) {
    auto nsPerOperation = std::vector<double>{};
    auto operationsPerSecond = std::vector<double>{};
    auto codePointsPerSecond = std::vector<double>{};
    auto mibPerSecond = std::vector<double>{};
    auto operations = std::uint64_t{};
    auto logicalCodePoints = std::uint64_t{};
    auto logicalNativeBytes = std::uint64_t{};
    auto capacityChanges = std::uint64_t{};
    for (const auto &sample : samples) {
        const auto safeOperations = std::max<std::uint64_t>(1U, sample.operations);
        const auto safeTime = std::max<std::int64_t>(1, sample.wallNanoseconds);
        nsPerOperation.emplace_back(static_cast<double>(safeTime) / static_cast<double>(safeOperations));
        operationsPerSecond.emplace_back(static_cast<double>(safeOperations) * 1.0e9 / static_cast<double>(safeTime));
        codePointsPerSecond.emplace_back(
            static_cast<double>(sample.logicalCodePoints) * 1.0e9 / static_cast<double>(safeTime));
        mibPerSecond.emplace_back(
            static_cast<double>(sample.logicalNativeBytes) * 1.0e9 / static_cast<double>(safeTime) / (1024.0 * 1024.0));
        operations += sample.operations;
        logicalCodePoints += sample.logicalCodePoints;
        logicalNativeBytes += sample.logicalNativeBytes;
        capacityChanges += sample.capacityChanges;
    }
    const auto timing = statistics(std::move(nsPerOperation));
    const auto operationRate = statistics(std::move(operationsPerSecond));
    const auto codePointRate = statistics(std::move(codePointsPerSecond));
    const auto byteRate = statistics(std::move(mibPerSecond));
    const auto workerFairness = fairness(samples);
    el::io::printLine(
        "record=benchmark scenario="_el,
        scenario.id,
        " width="_el,
        toString(scenario.width),
        " type="_el,
        toString(scenario.type),
        " use-case="_el,
        toString(scenario.useCase),
        " variant="_el,
        scenario.variant,
        " content="_el,
        toString(scenario.contentProfile),
        " sensitive="_el,
        toString(scenario.sensitiveMode),
        " size-mode="_el,
        toString(scenario.sizeMode),
        " size-cp="_el,
        scenario.size,
        " operand-cp="_el,
        scenario.operandSize,
        " native-bytes="_el,
        nativeBytesFor(scenario.width, scenario.contentProfile, scenario.size),
        " samples="_el,
        samples.size(),
        " operations="_el,
        operations,
        " logical-code-points="_el,
        logicalCodePoints,
        " logical-native-bytes="_el,
        logicalNativeBytes,
        " capacity-changes="_el,
        capacityChanges,
        " min-ns-per-operation="_el,
        timing.minimum,
        " median-ns-per-operation="_el,
        timing.median,
        " mean-ns-per-operation="_el,
        timing.mean,
        " p95-ns-per-operation="_el,
        timing.p95,
        " max-ns-per-operation="_el,
        timing.maximum,
        " median-operations-per-second="_el,
        operationRate.median,
        " median-code-points-per-second="_el,
        codePointRate.median,
        " median-mib-per-second="_el,
        byteRate.median,
        " fairness-min-operations-per-second="_el,
        workerFairness.minimum,
        " fairness-max-operations-per-second="_el,
        workerFairness.maximum,
        " fairness-cv="_el,
        workerFairness.coefficientOfVariation);
}

}
