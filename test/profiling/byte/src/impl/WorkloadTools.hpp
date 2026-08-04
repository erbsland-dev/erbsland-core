// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Fairness.hpp"
#include "Statistics.hpp"

#include "../Workload.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/mem/impl/UnsafeByteBufferAccess.hpp>
#include <erbsland/system/EnvironmentVariables.hpp>

#include <algorithm>
#include <atomic>
#include <barrier>
#include <cmath>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <numeric>
#include <set>
#include <thread>

namespace app::byte::impl {

using namespace el::text::literals;

using Clock = std::chrono::steady_clock;

/// Report a workload validation error.
[[noreturn]] void workloadError(const el::String &message) {
    throw el::ApplicationError{message};
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

/// Create deterministic source bytes of the requested size.
[[nodiscard]] auto makeBytes(const std::uint64_t size, const std::uint64_t seed) -> el::ByteBuffer {
    auto random = el::FastRandom{seed};
    return random.buildByteBuffer(el::ByteLength{size});
}

/// Calculate the digest of byte data.
[[nodiscard]] auto digest(const el::ConstByteSpan bytes) -> el::ByteBlock {
    auto hasher = el::cryptology::Hasher{el::cryptology::HashAlgorithm::Md5};
    hasher.update(el::ByteBlock::fromSpan(bytes));
    return hasher.finalize();
}

/// Calculate the elapsed time since a start point in nanoseconds.
[[nodiscard]] auto elapsedNanoseconds(const Clock::time_point start) -> std::int64_t {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
}

/// Select the byte index for an iteration.
[[nodiscard]] auto indexFor(const std::uint64_t iteration, const std::size_t size, const bool random) -> el::ByteIndex {
    if (size == 0U) {
        return el::ByteIndex::zero();
    }
    const auto index = random ? mixSeed(iteration, size) % size : iteration % size;
    return el::ByteIndex::fromSizeT(static_cast<std::size_t>(index));
}

/// Select the byte range for a workload variant.
[[nodiscard]] auto rangeFor(
    const std::size_t size, const std::size_t operand, const el::String &variant, const std::uint64_t iteration = 0U)
    -> el::ByteRange {
    const auto count = std::min(size, operand);
    auto start = std::size_t{};
    if (variant == "middle"_el || variant == "equal"_el || variant == "grow"_el || variant == "shrink"_el ||
        variant == "range"_el || variant == "aliased"_el) {
        start = size > count ? (size - count) / 2U : 0U;
    } else if (variant == "back"_el || variant == "end"_el || variant == "suffix"_el) {
        start = size > count ? size - count : 0U;
    } else if (variant == "random"_el && size > count) {
        start = static_cast<std::size_t>(mixSeed(iteration, size) % (size - count + 1U));
    }
    return el::ByteRange{el::ByteIndex::fromSizeT(start), el::ByteLength::fromSizeT(count)};
}

/// Mix a byte into the benchmark result sink.
void consumeByte(std::uint64_t &sink, const el::Byte value) noexcept {
    sink = mixSeed(sink, value.toUInt8());
}

/// Mix a byte index into the benchmark result sink.
void consumeIndex(std::uint64_t &sink, const el::ByteIndex value) noexcept {
    sink = mixSeed(sink, value.isValid() ? value.toRawValue() : std::numeric_limits<std::uint64_t>::max());
}

/// Calculate the number of bytes processed by a workload.
[[nodiscard]] auto operationByteCount(const Scenario &scenario, const std::uint64_t operations) noexcept
    -> std::uint64_t {
    if (scenario.workUnit == WorkUnit::Operations) {
        return 0U;
    }
    const auto perOperation = scenario.useCase == UseCase::ReadIndexed || scenario.useCase == UseCase::WriteIndexed ||
            scenario.useCase == UseCase::IntegerRead || scenario.useCase == UseCase::IntegerWrite ||
            scenario.useCase == UseCase::Append || scenario.useCase == UseCase::Insert ||
            scenario.useCase == UseCase::Replace || scenario.useCase == UseCase::RingTransfer ||
            scenario.useCase == UseCase::RingInteger
        ? std::max<std::uint64_t>(1U, scenario.operandSize)
        : std::max<std::uint64_t>(1U, scenario.size);
    return perOperation > std::numeric_limits<std::uint64_t>::max() / operations
        ? std::numeric_limits<std::uint64_t>::max()
        : perOperation * operations;
}

/// Execute a fixed-size byte-array workload.
template <std::size_t N>
[[nodiscard]] auto executeArray(const Scenario &scenario, const std::uint64_t seed, const std::uint64_t operations)
    -> WorkerResult {
    auto source = makeBytes(N, seed);
    auto firstStorage = std::make_unique<el::ByteArray<N>>();
    auto &first = *firstStorage;
    first.overwrite(source.span());
    auto secondStorage = std::make_unique<el::ByteArray<N>>(first);
    auto &second = *secondStorage;
    if constexpr (N > 0U) {
        second.xorAt(el::ByteIndex::zero(), el::Byte{1U});
    }
    auto sink = seed;
    const auto start = Clock::now();
    for (auto i = std::uint64_t{}; i < operations; ++i) {
        const auto index = indexFor(i, N, scenario.variant == "random"_el);
        switch (scenario.useCase) {
        case UseCase::Create: {
            auto value = el::ByteArray<N>{};
            if constexpr (N > 0U) {
                consumeByte(sink, value.get(el::ByteIndex::zero()));
            }
            break;
        }
        case UseCase::Copy: {
            auto value = first;
            if constexpr (N > 0U) {
                consumeByte(sink, value.get(index));
            }
            break;
        }
        case UseCase::Move: {
            auto copy = first;
            auto value = std::move(copy);
            if constexpr (N > 0U) {
                consumeByte(sink, value.get(index));
            }
            break;
        }
        case UseCase::Slice:
            sink = mixSeed(sink, first.span(rangeFor(N, std::max<std::size_t>(1U, N / 2U), "middle"_el)).size());
            break;
        case UseCase::Convert: {
            const auto value = first.toByteBuffer();
            sink = mixSeed(sink, value.length().toRawValue());
            break;
        }
        case UseCase::ReadIndexed:
            consumeByte(sink, first.get(index));
            break;
        case UseCase::WriteIndexed:
            if (scenario.variant == "xor-at"_el) {
                first.xorAt(index, el::Byte{0x5aU});
            } else {
                first.set(index, el::Byte::fromCroppedUInt64(i));
            }
            break;
        case UseCase::Traverse:
            if (scenario.variant == "for-each"_el) {
                sink = mixSeed(
                    sink,
                    static_cast<std::uint8_t>(first.forEach([&](const el::Byte value) { consumeByte(sink, value); })));
            } else {
                for (const auto value : first.span()) {
                    consumeByte(sink, value);
                }
            }
            break;
        case UseCase::IntegerRead:
            if constexpr (N >= sizeof(std::uint64_t)) {
                sink = mixSeed(
                    sink,
                    first.template getInteger<std::uint64_t>(
                        el::ByteIndex::zero(),
                        scenario.variant == "big"_el ? el::Endianness::Big : el::Endianness::Little));
            } else if constexpr (N > 0U) {
                sink = mixSeed(sink, first.template getInteger<std::uint8_t>(el::ByteIndex::zero()));
            }
            break;
        case UseCase::IntegerWrite:
            if constexpr (N >= sizeof(std::uint64_t)) {
                if (!first.template setInteger<std::uint64_t>(
                        el::ByteIndex::zero(),
                        i,
                        scenario.variant == "big"_el ? el::Endianness::Big : el::Endianness::Little)) {
                    workloadError("Fixed byte integer write unexpectedly failed."_el);
                }
            } else if constexpr (N > 0U) {
                if (!first.template setInteger<std::uint8_t>(el::ByteIndex::zero(), static_cast<std::uint8_t>(i))) {
                    workloadError("Fixed byte integer write unexpectedly failed."_el);
                }
            }
            break;
        case UseCase::Compare:
            sink = mixSeed(sink, (first <=> (scenario.variant == "equal"_el ? first : second)) == 0 ? 1U : 2U);
            break;
        case UseCase::Fill:
            if (scenario.variant == "range"_el) {
                first.fill(rangeFor(N, std::max<std::size_t>(1U, N / 4U), "middle"_el), el::Byte::fromCroppedUInt64(i));
            } else {
                first.fill(el::Byte::fromCroppedUInt64(i));
            }
            break;
        case UseCase::Overwrite:
            first.overwrite(rangeFor(N, source.span().size(), "middle"_el), source.span());
            break;
        case UseCase::Xor:
            if (scenario.variant == "range"_el) {
                first.xorWith(rangeFor(N, source.span().size(), "middle"_el), source.span());
            } else {
                if (!first.xorWith(source.span())) {
                    workloadError("Fixed byte XOR unexpectedly failed."_el);
                }
            }
            break;
        case UseCase::SecureErase: {
            auto value = first;
            value.secureErase();
            if constexpr (N > 0U) {
                consumeByte(sink, value.get(el::ByteIndex::zero()));
            }
            break;
        }
        case UseCase::Bitwise:
            if (scenario.variant == "or"_el) {
                first |= second;
            } else if (scenario.variant == "and"_el) {
                first &= second;
            } else if (scenario.variant == "xor"_el) {
                first ^= second;
            } else {
                first = ~first;
            }
            break;
        case UseCase::ShiftRotate:
            if constexpr (N > 0U) {
                if (scenario.variant == "left"_el) {
                    first.shiftLeft(static_cast<std::size_t>(i % 17U));
                } else if (scenario.variant == "right"_el) {
                    first.shiftRight(static_cast<std::size_t>(i % 17U));
                } else {
                    first.rotateLeft(static_cast<int>(i % 17U));
                }
            }
            break;
        case UseCase::PerByteShiftRotate:
            if constexpr (N > 0U) {
                if (scenario.variant == "shift"_el) {
                    first.shiftEachByteLeft(static_cast<std::size_t>(i % 9U));
                } else {
                    first.rotateEachByteLeft(static_cast<int>(i % 9U));
                }
            }
            break;
        default:
            workloadError("Unsupported ByteArray workload path."_el);
        }
    }
    const auto elapsed = elapsedNanoseconds(start);
    return WorkerResult{
        .operations = operations,
        .logicalBytes = operationByteCount(scenario, operations),
        .nanoseconds = elapsed,
        .sink = sink,
        .digest = digest(first.span())};
}

/// Dispatch a fixed-size byte-array workload.
[[nodiscard]] auto executeArrayDispatch(
    const Scenario &scenario, const std::uint64_t seed, const std::uint64_t operations) -> WorkerResult {
#define ERBSLAND_PROFILE_ARRAY_CASE(N)                                                                                 \
    case N##U:                                                                                                         \
        return executeArray<N>(scenario, seed, operations)
    switch (scenario.size) {
        ERBSLAND_PROFILE_ARRAY_CASE(0);
        ERBSLAND_PROFILE_ARRAY_CASE(1);
        ERBSLAND_PROFILE_ARRAY_CASE(4);
        ERBSLAND_PROFILE_ARRAY_CASE(8);
        ERBSLAND_PROFILE_ARRAY_CASE(15);
        ERBSLAND_PROFILE_ARRAY_CASE(16);
        ERBSLAND_PROFILE_ARRAY_CASE(17);
        ERBSLAND_PROFILE_ARRAY_CASE(20);
        ERBSLAND_PROFILE_ARRAY_CASE(31);
        ERBSLAND_PROFILE_ARRAY_CASE(32);
        ERBSLAND_PROFILE_ARRAY_CASE(33);
        ERBSLAND_PROFILE_ARRAY_CASE(48);
        ERBSLAND_PROFILE_ARRAY_CASE(63);
        ERBSLAND_PROFILE_ARRAY_CASE(64);
        ERBSLAND_PROFILE_ARRAY_CASE(65);
        ERBSLAND_PROFILE_ARRAY_CASE(128);
        ERBSLAND_PROFILE_ARRAY_CASE(255);
        ERBSLAND_PROFILE_ARRAY_CASE(256);
        ERBSLAND_PROFILE_ARRAY_CASE(257);
        ERBSLAND_PROFILE_ARRAY_CASE(4095);
        ERBSLAND_PROFILE_ARRAY_CASE(4096);
        ERBSLAND_PROFILE_ARRAY_CASE(4097);
        ERBSLAND_PROFILE_ARRAY_CASE(65535);
        ERBSLAND_PROFILE_ARRAY_CASE(65536);
    default:
        workloadError("Unsupported ByteArray extent reached the workload dispatcher."_el);
    }
#undef ERBSLAND_PROFILE_ARRAY_CASE
}

/// Select the search needle for a workload scenario.
[[nodiscard]] auto findNeedle(const el::ByteBuffer &source, const Scenario &scenario) -> el::ByteBlock {
    const auto count =
        static_cast<std::size_t>(std::max<std::uint64_t>(1U, std::min(scenario.operandSize, scenario.size)));
    if (scenario.variant == "miss"_el) {
        return el::ByteBlock{el::ByteLength::fromSizeT(count), el::Byte{0xffU}};
    }
    const auto range = rangeFor(source.span().size(), count, scenario.variant);
    return el::ByteBlock::fromSpan(source.span(range));
}

/// Execute a byte-block workload.
[[nodiscard]] auto executeBlock(
    const Scenario &scenario,
    const std::uint64_t seed,
    const std::uint64_t operations,
    const el::ByteBlock *sharedSource) -> WorkerResult {
    auto sourceBuffer = makeBytes(scenario.size, seed);
    if (scenario.variant == "adversarial"_el) {
        sourceBuffer.fill(el::Byte{0xaaU});
        if (!sourceBuffer.isEmpty()) {
            sourceBuffer.set(sourceBuffer.endIndex() - el::ByteLength{1U}, el::Byte{0xabU});
        }
    }
    const auto owned = el::ByteBlock::fromSpan(sourceBuffer.span());
    const auto &source = sharedSource != nullptr ? *sharedSource : owned;
    auto differentBuffer = sourceBuffer;
    if (!differentBuffer.isEmpty()) {
        differentBuffer.xorAt(el::ByteIndex::zero(), el::Byte{1U});
    }
    const auto different = el::ByteBlock::fromSpan(differentBuffer.span());
    const auto needle = findNeedle(sourceBuffer, scenario);
    auto sink = seed;
    auto finalValue = source;
    const auto start = Clock::now();
    for (auto i = std::uint64_t{}; i < operations; ++i) {
        const auto index = indexFor(i, source.span().size(), scenario.variant == "random"_el);
        switch (scenario.useCase) {
        case UseCase::Create:
            finalValue = scenario.variant == "filled"_el
                ? el::ByteBlock{el::ByteLength{scenario.size}, el::Byte::fromCroppedUInt64(i)}
                : el::ByteBlock::fromSpan(sourceBuffer.span());
            break;
        case UseCase::Copy: {
            const auto value = source;
            sink = mixSeed(sink, value.length().toRawValue());
            break;
        }
        case UseCase::Move: {
            auto copy = source;
            auto value = std::move(copy);
            sink = mixSeed(sink, value.length().toRawValue());
            break;
        }
        case UseCase::Slice: {
            const auto value = source.slice(rangeFor(source.span().size(), source.span().size() / 2U, "middle"_el));
            sink = mixSeed(sink, value.length().toRawValue());
            break;
        }
        case UseCase::Convert:
            if (scenario.variant == "buffer"_el) {
                sink = mixSeed(sink, source.toByteBuffer().length().toRawValue());
            } else {
                sink = mixSeed(sink, source.toUInt8Vector().size());
            }
            break;
        case UseCase::ReadIndexed:
            consumeByte(sink, source.get(index));
            break;
        case UseCase::Traverse:
            if (scenario.variant == "for-each"_el) {
                sink = mixSeed(
                    sink,
                    static_cast<std::uint8_t>(source.forEach([&](const el::Byte value) { consumeByte(sink, value); })));
            } else {
                for (const auto value : source.span()) {
                    consumeByte(sink, value);
                }
            }
            break;
        case UseCase::IntegerRead:
            sink = mixSeed(
                sink,
                source.getInteger<std::uint64_t>(
                    el::ByteIndex::zero(),
                    scenario.variant == "big"_el ? el::Endianness::Big : el::Endianness::Little));
            break;
        case UseCase::Compare:
            sink = mixSeed(sink, (source <=> (scenario.variant == "equal"_el ? source : different)) == 0 ? 1U : 2U);
            break;
        case UseCase::PrefixSuffix:
            if (scenario.variant == "prefix"_el) {
                sink = mixSeed(sink, source.startsWith(needle) ? 1U : 0U);
            } else if (scenario.variant == "suffix"_el) {
                sink = mixSeed(sink, source.endsWith(needle) ? 1U : 0U);
            } else {
                sink = mixSeed(sink, source.contains(needle) ? 1U : 0U);
            }
            break;
        case UseCase::Find:
            consumeIndex(sink, scenario.variant == "end"_el ? source.findLast(needle) : source.find(needle));
            break;
        case UseCase::SecureErase: {
            auto value = source;
            if (scenario.variant == "unique"_el) {
                value = el::ByteBlock::fromSpan(source.span());
            } else {
                const auto alias = value.slice(el::ByteRange::all());
                sink = mixSeed(sink, alias.length().toRawValue());
            }
            if (scenario.sensitiveMode == SensitiveMode::Sensitive) {
                value.markAsSensitive();
            }
            value.secureErase();
            sink = mixSeed(sink, value.length().toRawValue());
            finalValue = std::move(value);
            break;
        }
        default:
            workloadError("Unsupported ByteBlock workload path."_el);
        }
    }
    const auto elapsed = elapsedNanoseconds(start);
    return WorkerResult{
        .operations = operations,
        .logicalBytes = operationByteCount(scenario, operations),
        .nanoseconds = elapsed,
        .sink = sink,
        .digest = digest(finalValue.span())};
}

/// Apply the selected sensitivity mode to a byte-block editor.
void setSensitive(el::ByteBlockEditor &value, const SensitiveMode mode) {
    if (mode == SensitiveMode::Sensitive) {
        value.markAsSensitive();
    }
}

/// Apply the selected sensitivity mode to a byte buffer.
void setSensitive(el::ByteBuffer &value, const SensitiveMode mode) {
    value.setSensitive(mode == SensitiveMode::Sensitive);
}

/// Get the current capacity of a dynamic byte value.
template <typename T>
[[nodiscard]] auto currentCapacity(const T &value) -> std::uint64_t {
    return value.capacity().toRawValue();
}

/// Mix the observable state of a dynamic byte value into the result sink.
template <typename T>
void consumeReadable(std::uint64_t &sink, const T &value, const Scenario &scenario, const std::uint64_t iteration) {
    const auto index = indexFor(iteration, value.span().size(), scenario.variant == "random"_el);
    consumeByte(sink, value.get(index));
}

/// Apply the common mutation operations to a dynamic byte value.
template <typename T>
void mutateCommon(T &value, const el::ByteBuffer &source, const Scenario &scenario, const std::uint64_t iteration) {
    const auto range =
        rangeFor(value.span().size(), static_cast<std::size_t>(scenario.operandSize), scenario.variant, iteration);
    if (scenario.useCase == UseCase::WriteIndexed) {
        const auto index = indexFor(iteration, value.span().size(), false);
        if (scenario.variant == "xor-at"_el) {
            value.xorAt(index, el::Byte{0x5aU});
        } else {
            value.set(index, el::Byte::fromCroppedUInt64(iteration));
        }
    } else if (scenario.useCase == UseCase::IntegerWrite) {
        if (!value.template setInteger<std::uint64_t>(
                el::ByteIndex::zero(),
                iteration,
                scenario.variant == "big"_el ? el::Endianness::Big : el::Endianness::Little)) {
            workloadError("Dynamic byte integer write unexpectedly failed."_el);
        }
    } else if (scenario.useCase == UseCase::Fill) {
        if (scenario.variant == "whole"_el) {
            value.fill(el::Byte::fromCroppedUInt64(iteration));
        } else {
            value.fill(range, el::Byte::fromCroppedUInt64(iteration));
        }
    } else if (scenario.useCase == UseCase::Overwrite) {
        if (scenario.variant == "aliased"_el) {
            value.overwrite(range, value.span(range));
        } else {
            value.overwrite(
                range, source.span(el::ByteRange{el::ByteIndex::zero(), el::ByteLength{scenario.operandSize}}));
        }
    } else if (scenario.useCase == UseCase::Xor) {
        if (scenario.variant == "whole"_el) {
            if (!value.xorWith(source.span())) {
                workloadError("Dynamic byte XOR unexpectedly failed."_el);
            }
        } else {
            value.xorWith(
                range, source.span(el::ByteRange{el::ByteIndex::zero(), el::ByteLength{scenario.operandSize}}));
        }
    }
}

/// Apply editor-specific operations to a dynamic byte value.
template <typename T>
void editDynamic(T &value, const el::ByteBuffer &source, const Scenario &scenario, const std::uint64_t iteration) {
    const auto operand = source.span(el::ByteRange{el::ByteIndex::zero(), el::ByteLength{scenario.operandSize}});
    const auto range =
        rangeFor(value.span().size(), static_cast<std::size_t>(scenario.operandSize), scenario.variant, iteration);
    if (scenario.useCase == UseCase::ClearReset) {
        if (scenario.variant == "clear"_el) {
            value.clear();
        } else {
            value.reset();
        }
    } else if (scenario.useCase == UseCase::ReserveShrink) {
        scenario.variant == "reserve"_el ? value.reserve(el::ByteLength{scenario.size + scenario.operandSize})
                                         : value.shrinkToFit();
    } else if (scenario.useCase == UseCase::Resize) {
        value.resize(
            el::ByteLength{scenario.variant == "grow"_el ? scenario.size + scenario.operandSize : scenario.size / 2U});
    } else if (scenario.useCase == UseCase::Append) {
        if (scenario.variant == "byte-growing"_el) {
            value.append(el::Byte::fromCroppedUInt64(iteration));
        } else {
            if (scenario.variant == "span-reserved"_el) {
                value.reserve(el::ByteLength{scenario.size + scenario.operandSize});
            }
            value.append(operand);
        }
    } else if (scenario.useCase == UseCase::Insert) {
        if (scenario.variant == "aliased"_el) {
            value.insert(range.index(), value.span(range));
        } else {
            value.insert(range.index(), operand);
        }
    } else if (scenario.useCase == UseCase::Replace) {
        if (scenario.variant == "aliased"_el) {
            value.replace(range, value.span(range));
        } else if (scenario.variant == "grow"_el) {
            value.replace(range, source.span());
        } else if (scenario.variant == "shrink"_el) {
            value.replace(el::ByteRange::all(), operand);
        } else {
            value.replace(range, operand);
        }
    } else if (scenario.useCase == UseCase::RemoveKeep) {
        if (scenario.variant == "keep"_el) {
            value.keep(range);
        } else {
            value.remove(range);
        }
    } else if (scenario.useCase == UseCase::EditStress) {
        value.append(operand);
        value.insert(el::ByteIndex::zero(), operand);
        value.replace(rangeFor(value.span().size(), operand.size(), "middle"_el), operand);
        value.remove(rangeFor(value.span().size(), operand.size(), "front"_el));
        value.resize(el::ByteLength{scenario.size});
    }
}

/// Execute a dynamic byte workload for a selected container type.
template <typename T>
[[nodiscard]] auto executeDynamic(const Scenario &scenario, const std::uint64_t seed, const std::uint64_t operations)
    -> WorkerResult {
    auto source = makeBytes(scenario.size, seed);
    auto base = [&]() -> T {
        if constexpr (std::same_as<T, el::ByteBlockEditor>) {
            return T::fromSpan(source.span());
        } else {
            return T{source.span()};
        }
    }();
    setSensitive(base, scenario.sensitiveMode);
    auto different = base;
    if (!different.isEmpty()) {
        different.xorAt(el::ByteIndex::zero(), el::Byte{1U});
    }
    auto sink = seed;
    auto capacityChanges = std::uint64_t{};
    auto finalValue = base;
    const auto usesPreparedFixtures =
        scenario.useCase != UseCase::Create && scenario.useCase != UseCase::Copy && scenario.useCase != UseCase::Move;
    auto fixtures = std::vector<T>{};
    if (usesPreparedFixtures) {
        fixtures.reserve(static_cast<std::size_t>(operations));
        for (auto i = std::uint64_t{}; i < operations; ++i) {
            if constexpr (std::same_as<T, el::ByteBlockEditor>) {
                const auto needsSharedStorage = scenario.useCase == UseCase::CowStress ||
                    (scenario.useCase == UseCase::Detach && scenario.variant == "shared"_el) ||
                    (scenario.useCase == UseCase::SecureErase && scenario.variant == "shared"_el);
                fixtures.emplace_back(needsSharedStorage ? base : T::fromSpan(source.span()));
            } else {
                fixtures.emplace_back(base);
            }
            setSensitive(fixtures.back(), scenario.sensitiveMode);
        }
    }
    auto transient = base;
    const auto start = Clock::now();
    for (auto i = std::uint64_t{}; i < operations; ++i) {
        if (!usesPreparedFixtures && scenario.useCase != UseCase::Create) {
            transient = base;
        }
        auto *valuePointer = usesPreparedFixtures ? &fixtures[static_cast<std::size_t>(i)] : &transient;
        auto &value = *valuePointer;
        const auto oldCapacity = currentCapacity(value);
        switch (scenario.useCase) {
        case UseCase::Create:
            if (scenario.variant == "filled"_el) {
                value = T{el::ByteLength{scenario.size}, el::Byte::fromCroppedUInt64(i)};
            } else if constexpr (std::same_as<T, el::ByteBlockEditor>) {
                value = T::fromSpan(source.span());
            } else {
                value = T{source.span()};
            }
            setSensitive(value, scenario.sensitiveMode);
            break;
        case UseCase::Copy:
            sink = mixSeed(sink, value.length().toRawValue());
            break;
        case UseCase::Move: {
            auto moved = std::move(value);
            sink = mixSeed(sink, moved.length().toRawValue());
            value = std::move(moved);
            break;
        }
        case UseCase::Slice:
            sink =
                mixSeed(sink, value.span(rangeFor(value.span().size(), value.span().size() / 2U, "middle"_el)).size());
            break;
        case UseCase::Convert:
            if constexpr (std::same_as<T, el::ByteBlockEditor>) {
                sink = mixSeed(
                    sink,
                    scenario.variant == "buffer"_el ? value.toByteBuffer().length().toRawValue()
                                                    : value.toUInt8Vector().size());
            } else {
                sink = mixSeed(sink, value.toUInt8Vector().size());
            }
            break;
        case UseCase::ReadIndexed:
            consumeReadable(sink, value, scenario, i);
            break;
        case UseCase::Traverse:
            if (scenario.variant == "for-each"_el) {
                sink = mixSeed(
                    sink,
                    static_cast<std::uint8_t>(value.forEach([&](const el::Byte byte) { consumeByte(sink, byte); })));
            } else {
                for (const auto byte : value.span()) {
                    consumeByte(sink, byte);
                }
            }
            break;
        case UseCase::IntegerRead:
            sink = mixSeed(
                sink,
                value.template getInteger<std::uint64_t>(
                    el::ByteIndex::zero(),
                    scenario.variant == "big"_el ? el::Endianness::Big : el::Endianness::Little));
            break;
        case UseCase::Compare:
            sink = mixSeed(sink, (value <=> (scenario.variant == "equal"_el ? base : different)) == 0 ? 1U : 2U);
            break;
        case UseCase::WriteIndexed:
        case UseCase::IntegerWrite:
        case UseCase::Fill:
        case UseCase::Overwrite:
        case UseCase::Xor:
            mutateCommon(value, source, scenario, i);
            break;
        case UseCase::ClearReset:
        case UseCase::ReserveShrink:
        case UseCase::Resize:
        case UseCase::Append:
        case UseCase::Insert:
        case UseCase::Replace:
        case UseCase::RemoveKeep:
        case UseCase::EditStress:
            editDynamic(value, source, scenario, i);
            break;
        case UseCase::SecureErase:
            if constexpr (std::same_as<T, el::ByteBuffer>) {
                if (scenario.variant == "disable-mode"_el && scenario.sensitiveMode == SensitiveMode::Sensitive) {
                    value.setSensitive(false);
                } else {
                    value.secureErase();
                }
            } else {
                value.secureErase();
            }
            break;
        default:
            if constexpr (std::same_as<T, el::ByteBlockEditor>) {
                if (scenario.useCase == UseCase::Detach) {
                    if (scenario.variant == "shared"_el) {
                        const auto alias = value;
                        sink = mixSeed(sink, alias.length().toRawValue());
                    }
                    value.detach();
                } else if (scenario.useCase == UseCase::PrefixSuffix) {
                    const auto needle = el::ByteBlock::fromSpan(
                        source.span(rangeFor(source.span().size(), scenario.operandSize, scenario.variant)));
                    sink = mixSeed(
                        sink,
                        scenario.variant == "prefix"_el       ? value.startsWith(needle)
                            : scenario.variant == "suffix"_el ? value.endsWith(needle)
                                                              : value.contains(needle));
                } else if (scenario.useCase == UseCase::Find) {
                    const auto needle = findNeedle(source, scenario);
                    consumeIndex(sink, scenario.variant == "end"_el ? value.findLast(needle) : value.find(needle));
                } else if (scenario.useCase == UseCase::Join) {
                    const auto block = el::ByteBlock{value};
                    value = value.join({block, block, block});
                } else if (scenario.useCase == UseCase::CowStress) {
                    const auto block = el::ByteBlock{value};
                    const auto slice =
                        block.slice(rangeFor(block.span().size(), block.span().size() / 2U, "middle"_el));
                    auto editor = value;
                    editor.set(el::ByteIndex::zero(), el::Byte::fromCroppedUInt64(i));
                    editor.append(slice);
                    value = std::move(editor);
                } else {
                    workloadError("Unsupported ByteBlockEditor workload path."_el);
                }
            } else {
                workloadError("Unsupported ByteBuffer workload path."_el);
            }
        }
        if (currentCapacity(value) != oldCapacity) {
            ++capacityChanges;
        }
        sink = mixSeed(sink, value.length().toRawValue());
    }
    const auto elapsed = elapsedNanoseconds(start);
    if (!fixtures.empty()) {
        finalValue = std::move(fixtures.back());
    }
    return WorkerResult{
        .operations = operations,
        .logicalBytes = operationByteCount(scenario, operations),
        .capacityChanges = capacityChanges,
        .nanoseconds = elapsed,
        .sink = sink,
        .digest = digest(finalValue.span())};
}

/// Execute a byte-block-editor workload.
[[nodiscard]] auto executeEditor(const Scenario &scenario, const std::uint64_t seed, const std::uint64_t operations)
    -> WorkerResult {
    return executeDynamic<el::ByteBlockEditor>(scenario, seed, operations);
}

/// Execute a byte-buffer workload.
[[nodiscard]] auto executeBuffer(const Scenario &scenario, const std::uint64_t seed, const std::uint64_t operations)
    -> WorkerResult {
    return executeDynamic<el::ByteBuffer>(scenario, seed, operations);
}

/// Execute a byte-ring workload.
[[nodiscard]] auto executeRing(const Scenario &scenario, const std::uint64_t seed, const std::uint64_t operations)
    -> WorkerResult {
    const auto capacity = std::max<std::uint64_t>(1U, scenario.size);
    const auto operandSize = std::max<std::uint64_t>(1U, std::min(scenario.operandSize, capacity));
    auto source = makeBytes(capacity, seed);
    auto destination = el::ByteBuffer{el::ByteLength{capacity}};
    auto writableDestination = el::mem::impl::UnsafeByteBufferAccess{destination}.writableData();
    auto sink = seed;
    auto capacityChanges = std::uint64_t{};
    auto finalBytes = el::ByteBlock{};
    const auto start = Clock::now();
    for (auto i = std::uint64_t{}; i < operations; ++i) {
        const auto growable = scenario.variant == "growable"_el || scenario.variant == "geometric"_el ||
            scenario.variant == "producer-consumer"_el;
        auto ring = growable
            ? el::ByteRingBuffer{el::ByteLength{std::max<std::uint64_t>(1U, capacity / 8U)}, el::ByteLength{capacity * 2U}}
            : el::ByteRingBuffer{el::ByteLength{capacity}};
        ring.setSensitive(scenario.sensitiveMode == SensitiveMode::Sensitive);
        const auto oldCapacity = ring.capacity();
        const auto operand = source.span(el::ByteRange{el::ByteIndex::zero(), el::ByteLength{operandSize}});
        switch (scenario.useCase) {
        case UseCase::Create:
            sink = mixSeed(sink, ring.capacity().toRawValue());
            break;
        case UseCase::RingReserveGrow:
            if (scenario.variant == "failure"_el) {
                sink = mixSeed(sink, ring.reserveAdditional(el::ByteLength{capacity + 1U}).isFailure() ? 1U : 0U);
            } else {
                sink = mixSeed(sink, ring.reserveAdditional(el::ByteLength{capacity}).isSuccessful() ? 1U : 0U);
            }
            break;
        case UseCase::RingTransfer:
            if (scenario.variant == "exact-failure"_el) {
                sink = mixSeed(sink, ring.writeExact(source.span()).isFailure() ? 1U : 0U);
                sink = mixSeed(
                    sink, ring.writeExact(el::ByteBlock{el::ByteLength{capacity + 1U}}.span()).isFailure() ? 1U : 0U);
            } else if (scenario.variant == "exact"_el) {
                sink = mixSeed(sink, ring.writeExact(operand).isSuccessful() ? 1U : 0U);
                sink = mixSeed(sink, ring.read(writableDestination).toRawValue());
            } else {
                sink = mixSeed(sink, ring.write(source.span()).toRawValue());
                sink = mixSeed(
                    sink, ring.read(writableDestination.first(static_cast<std::size_t>(operandSize))).toRawValue());
            }
            break;
        case UseCase::RingOwnedRead:
            if (ring.writeExact(source.span()).isFailure()) {
                workloadError("Ring setup write unexpectedly failed."_el);
            }
            finalBytes = ring.read(el::ByteLength{operandSize});
            sink = mixSeed(sink, finalBytes.length().toRawValue());
            break;
        case UseCase::RingWrappedCycle: {
            const auto first = std::max<std::uint64_t>(1U, capacity - operandSize / 2U);
            if (ring.write(source.span(el::ByteRange{el::ByteIndex::zero(), el::ByteLength{first}})) !=
                el::ByteLength{first}) {
                workloadError("Ring wrap setup write unexpectedly failed."_el);
            }
            if (ring.read(writableDestination.first(static_cast<std::size_t>(first / 2U))) !=
                el::ByteLength{first / 2U}) {
                workloadError("Ring wrap setup read unexpectedly failed."_el);
            }
            if (ring.write(operand) != el::ByteLength::fromSizeT(operand.size())) {
                workloadError("Ring wrapped write unexpectedly failed."_el);
            }
            finalBytes = ring.read(el::ByteLength::infinite());
            sink = mixSeed(sink, finalBytes.length().toRawValue());
            break;
        }
        case UseCase::RingInteger:
            ring.setEndianness(scenario.variant == "big"_el ? el::Endianness::Big : el::Endianness::Little);
            if (ring.writeInteger<std::uint64_t>(seed + i).isFailure()) {
                workloadError("Ring integer write unexpectedly failed."_el);
            }
            sink = mixSeed(sink, ring.readInteger<std::uint64_t>().value_or(0U));
            break;
        case UseCase::RingClearShrinkSwap:
            if (ring.writeExact(operand).isFailure()) {
                workloadError("Ring setup write unexpectedly failed."_el);
            }
            if (scenario.variant == "clear"_el) {
                ring.clear();
            } else if (scenario.variant == "shrink"_el) {
                ring.clear();
                ring.shrinkToInitial();
            } else {
                auto other = el::ByteRingBuffer{el::ByteLength{capacity}};
                ring.swap(other);
                sink = mixSeed(sink, other.length().toRawValue());
            }
            break;
        case UseCase::EditStress:
            for (auto cycle = 0U; cycle < 8U; ++cycle) {
                if (ring.write(operand) != el::ByteLength::fromSizeT(operand.size())) {
                    workloadError("Ring stress write unexpectedly failed."_el);
                }
                const auto readDestination = writableDestination.first(static_cast<std::size_t>(operandSize / 2U + 1U));
                if (ring.read(readDestination) != el::ByteLength::fromSizeT(readDestination.size())) {
                    workloadError("Ring stress read unexpectedly failed."_el);
                }
            }
            finalBytes = ring.read(el::ByteLength::infinite());
            sink = mixSeed(sink, finalBytes.length().toRawValue());
            break;
        default:
            workloadError("Unsupported ByteRingBuffer workload path."_el);
        }
        if (ring.capacity() != oldCapacity) {
            ++capacityChanges;
        }
        if (scenario.sensitiveMode == SensitiveMode::Sensitive && !ring.isSensitive() &&
            !(scenario.useCase == UseCase::RingClearShrinkSwap && scenario.variant == "swap"_el)) {
            workloadError("Sensitive ring workload lost its sensitive mode."_el);
        }
    }
    const auto elapsed = elapsedNanoseconds(start);
    return WorkerResult{
        .operations = operations,
        .logicalBytes = operationByteCount(scenario, operations),
        .capacityChanges = capacityChanges,
        .nanoseconds = elapsed,
        .sink = sink,
        .digest = digest(finalBytes.span())};
}

/// Execute the requested byte workload.
[[nodiscard]] auto executeWorkload(
    const Scenario &scenario,
    const std::uint64_t seed,
    const std::uint64_t operations,
    const el::ByteBlock *sharedSource = nullptr) -> WorkerResult {
    switch (scenario.type) {
    case ByteType::Array:
        return executeArrayDispatch(scenario, seed, operations);
    case ByteType::Block:
        return executeBlock(scenario, seed, operations, sharedSource);
    case ByteType::BlockEditor:
        return executeEditor(scenario, seed, operations);
    case ByteType::Buffer:
        return executeBuffer(scenario, seed, operations);
    case ByteType::RingBuffer:
        return executeRing(scenario, seed, operations);
    }
    workloadError("Unsupported byte profiler type."_el);
}

/// Determine the largest permitted operation count for a scenario.
[[nodiscard]] auto maximumOperations(const Configuration &configuration, const Scenario &scenario) noexcept
    -> std::uint64_t {
    const auto perWorkerMemory = configuration.run.memoryLimit / configuration.run.threadCount;
    const auto footprint = std::max<std::uint64_t>(1U, scenario.size + scenario.operandSize);
    return std::clamp<std::uint64_t>(perWorkerMemory / footprint, 1U, 1'000'000U);
}

/// Calibrate the operation count for one scenario.
[[nodiscard]] auto calibrateOperations(const Configuration &configuration, const Scenario &scenario) -> std::uint64_t {
    const auto maximum = maximumOperations(configuration, scenario);
    auto operations = std::uint64_t{1U};
    for (;;) {
        const auto result = executeWorkload(scenario, workerSeed(configuration, scenario, 0U, 0U), operations);
        if (result.nanoseconds >= configuration.run.minimumSampleTime.count() || operations >= maximum) {
            return operations;
        }
        const auto elapsed = std::max<std::int64_t>(1, result.nanoseconds);
        const auto ratio = std::clamp<std::uint64_t>(
            static_cast<std::uint64_t>(configuration.run.minimumSampleTime.count() / elapsed), 2U, 16U);
        operations = std::min(maximum, operations > maximum / ratio ? maximum : operations * ratio);
    }
}

/// Execute one benchmark sample.
auto runSample(
    const Configuration &configuration,
    const Scenario &scenario,
    const std::uint64_t sampleIndex,
    const std::uint64_t operations) -> SampleResult {
    const auto threadCount = configuration.run.threadCount;
    const auto failureVariable = el::system::EnvironmentVariables{}.get("ERBSLAND_BYTE_PROFILE_TEST_FAIL_WORKER"_el);
    const auto injectWorkerFailure = failureVariable.has_value() && *failureVariable == "1"_el;
    auto sharedSource = el::ByteBlock{};
    if (scenario.type == ByteType::Block) {
        sharedSource =
            el::ByteBlock::fromSpan(makeBytes(scenario.size, workerSeed(configuration, scenario, 0U, 0U)).span());
        if (scenario.sensitiveMode == SensitiveMode::Sensitive) {
            sharedSource.markAsSensitive();
        }
    }
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
                    workloadError("Injected byte profiler worker failure."_el);
                }
                results[worker] = executeWorkload(
                    scenario,
                    workerSeed(configuration, scenario, worker, sampleIndex),
                    operations,
                    scenario.type == ByteType::Block ? &sharedSource : nullptr);
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
            workloadError("A byte profiler worker produced an invalid validation result."_el);
        }
        result.operations += worker.operations;
        result.logicalBytes += worker.logicalBytes;
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
            el::StringFormat{"{} {} {} {} {} {} {} {}\n"_el}.build(
                scenario.id,
                toString(scenario.type),
                toString(scenario.useCase),
                scenario.variant,
                toString(scenario.sizeMode),
                toString(scenario.sensitiveMode),
                scenario.size,
                scenario.operandSize));
    }
    auto hasher = el::cryptology::Hasher{el::cryptology::HashAlgorithm::Md5};
    hasher.update(el::String{text});
    return hasher.finalize();
}

/// Print one benchmark sample.
void printSample(const Scenario &scenario, const SampleResult &sample) {
    const auto nanosecondsPerOperation = static_cast<double>(sample.wallNanoseconds) /
        static_cast<double>(std::max<std::uint64_t>(1U, sample.operations));
    const auto operationsPerSecond = static_cast<double>(sample.operations) * 1.0e9 /
        static_cast<double>(std::max<std::int64_t>(1, sample.wallNanoseconds));
    const auto mibPerSecond = static_cast<double>(sample.logicalBytes) * 1.0e9 /
        static_cast<double>(std::max<std::int64_t>(1, sample.wallNanoseconds)) / (1024.0 * 1024.0);
    el::io::printLine(
        "record=sample scenario="_el,
        scenario.id,
        " type="_el,
        toString(scenario.type),
        " use-case="_el,
        toString(scenario.useCase),
        " variant="_el,
        scenario.variant,
        " sensitive="_el,
        toString(scenario.sensitiveMode),
        " size="_el,
        scenario.size,
        " operand-size="_el,
        scenario.operandSize,
        " operations="_el,
        sample.operations,
        " logical-bytes="_el,
        sample.logicalBytes,
        " capacity-changes="_el,
        sample.capacityChanges,
        " wall-ns="_el,
        sample.wallNanoseconds,
        " ns-per-operation="_el,
        nanosecondsPerOperation,
        " operations-per-second="_el,
        operationsPerSecond,
        " mib-per-second="_el,
        mibPerSecond);
}

/// Print the results of a benchmark scenario.
void printBenchmark(const Scenario &scenario, const std::vector<SampleResult> &samples) {
    auto nsPerOperation = std::vector<double>{};
    auto operationsPerSecond = std::vector<double>{};
    auto mibPerSecond = std::vector<double>{};
    auto operations = std::uint64_t{};
    auto logicalBytes = std::uint64_t{};
    auto capacityChanges = std::uint64_t{};
    for (const auto &sample : samples) {
        const auto safeOperations = std::max<std::uint64_t>(1U, sample.operations);
        const auto safeTime = std::max<std::int64_t>(1, sample.wallNanoseconds);
        nsPerOperation.emplace_back(static_cast<double>(safeTime) / static_cast<double>(safeOperations));
        operationsPerSecond.emplace_back(static_cast<double>(safeOperations) * 1.0e9 / static_cast<double>(safeTime));
        mibPerSecond.emplace_back(
            static_cast<double>(sample.logicalBytes) * 1.0e9 / static_cast<double>(safeTime) / (1024.0 * 1024.0));
        operations += sample.operations;
        logicalBytes += sample.logicalBytes;
        capacityChanges += sample.capacityChanges;
    }
    const auto timing = statistics(std::move(nsPerOperation));
    const auto rate = statistics(std::move(operationsPerSecond));
    const auto throughput = statistics(std::move(mibPerSecond));
    const auto workerFairness = fairness(samples);
    el::io::printLine(
        "record=benchmark scenario="_el,
        scenario.id,
        " type="_el,
        toString(scenario.type),
        " use-case="_el,
        toString(scenario.useCase),
        " variant="_el,
        scenario.variant,
        " sensitive="_el,
        toString(scenario.sensitiveMode),
        " size-mode="_el,
        toString(scenario.sizeMode),
        " size="_el,
        scenario.size,
        " operand-size="_el,
        scenario.operandSize,
        " samples="_el,
        samples.size(),
        " operations="_el,
        operations,
        " logical-bytes="_el,
        logicalBytes,
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
        rate.median,
        " median-mib-per-second="_el,
        throughput.median,
        " fairness-min-operations-per-second="_el,
        workerFairness.minimum,
        " fairness-max-operations-per-second="_el,
        workerFairness.maximum,
        " fairness-cv="_el,
        workerFairness.coefficientOfVariation);
}

}
