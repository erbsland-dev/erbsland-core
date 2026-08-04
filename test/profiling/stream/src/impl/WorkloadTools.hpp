// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ChunkSchedule.hpp"
#include "Fixture.hpp"
#include "PreparedScenario.hpp"
#include "Statistics.hpp"
#include "ThroughputStatistics.hpp"
#include "WorkerFairness.hpp"

#include "../Workload.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/stream/impl/StreamBufferSizes.hpp>
#include <erbsland/system/EnvironmentVariables.hpp>
#include <erbsland/text/StringCharReader.hpp>

#include <algorithm>
#include <atomic>
#include <barrier>
#include <cmath>
#include <exception>
#include <limits>
#include <mutex>
#include <numeric>
#include <set>
#include <thread>

namespace app::stream::impl {

using namespace el::text::literals;

using Clock = std::chrono::steady_clock;

/// Throw the workload failure with the specified message.
[[noreturn]] void workloadError(const el::String &message) {
    throw el::ApplicationError{message};
}

/// Stop the current worker if another worker has failed.
void requireActive(const std::atomic_bool &cancelled) {
    if (cancelled.load(std::memory_order_relaxed)) {
        workloadError("Workload cancelled after another worker failed."_el);
    }
}

/// Get the elapsed nanoseconds since the specified start time.
[[nodiscard]] auto elapsedNanoseconds(const Clock::time_point start) -> std::int64_t {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
}

/// Mix a value into a deterministic pseudo-random seed.
[[nodiscard]] auto mixSeed(std::uint64_t seed, const std::uint64_t value) noexcept -> std::uint64_t {
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
    seed ^= seed >> 30U;
    seed *= 0xbf58476d1ce4e5b9ULL;
    seed ^= seed >> 27U;
    seed *= 0x94d049bb133111ebULL;
    return seed ^ (seed >> 31U);
}

/// Derive the deterministic seed for a scenario worker slot.
[[nodiscard]] auto scenarioSeed(
    const std::uint64_t seed, const Scenario &scenario, const std::uint32_t worker, const std::uint32_t slot)
    -> std::uint64_t {
    auto result = mixSeed(seed, scenario.id.toHash());
    result = mixSeed(result, worker);
    return mixSeed(result, slot);
}

/// Calculate the MD5 digest for a byte block.
[[nodiscard]] auto digest(const el::ByteBlock &bytes) -> el::ByteBlock {
    auto hasher = el::cryptology::Hasher{el::cryptology::HashAlgorithm::Md5};
    hasher.update(bytes);
    return hasher.finalize();
}

/// Calculate the MD5 digest for text.
[[nodiscard]] auto digest(const el::String &text) -> el::ByteBlock {
    auto hasher = el::cryptology::Hasher{el::cryptology::HashAlgorithm::Md5};
    hasher.update(text);
    return hasher.finalize();
}

/// Calculate the MD5 digest of no more than the expected file data.
[[nodiscard]] auto rawFileDigest(const el::Path &path, const std::uint64_t maximum) -> el::ByteBlock {
    auto options = el::PathReadDataOptions{};
    options.setMaximumByteLength(el::ByteLength{maximum + 16U});
    return digest(path.content().readDataOrThrow(options));
}

/// Create deterministic text with approximately the requested byte length.
[[nodiscard]] auto buildText(const std::uint64_t seed, const std::uint64_t targetBytes) -> el::String {
    auto random = el::FastRandom{seed};
    auto result = el::StringEditor{};
    result.reserve(el::ByteLength{targetBytes + 16U});
    auto lineLength = std::uint64_t{};
    auto nextLineLength = random.getUInt64(24U, 120U);
    while (result.length().toRawValue() < targetBytes) {
        if (lineLength >= nextLineLength) {
            result.append(U'\n');
            lineLength = 0U;
            nextLineLength =
                random.getUInt64(0U, 99U) == 0U ? random.getUInt64(1024U, 4096U) : random.getUInt64(24U, 120U);
            continue;
        }
        const auto selection = random.getUInt32(0U, 99U);
        if (selection < 62U) {
            result.append(el::Char{static_cast<char32_t>(U'a' + random.getUInt32(0U, 25U))});
        } else if (selection < 72U) {
            result.append(U' ');
        } else if (selection < 80U) {
            constexpr auto punctuation = std::array{U'.', U',', U':', U';', U'-', U'!', U'?', U'('};
            result.append(
                el::Char{punctuation[random.getUInt32(0U, static_cast<std::uint32_t>(punctuation.size() - 1U))]});
        } else if (selection < 92U) {
            result.append(el::Char{static_cast<char32_t>(0x00c0U + random.getUInt32(0U, 0x02ffU - 0x00c0U))});
        } else if (selection < 98U) {
            result.append(el::Char{static_cast<char32_t>(0x4e00U + random.getUInt32(0U, 0x9fffU - 0x4e00U))});
        } else {
            result.append(el::Char{static_cast<char32_t>(0x1f300U + random.getUInt32(0U, 0x1f64fU - 0x1f300U))});
        }
        ++lineLength;
    }
    if (result.charAt(el::StringSide::Back) != U'\n') {
        result.append(U'\n');
    }
    return result;
}

/// Return the text expected after the scenario write method.
[[nodiscard]] auto expectedWrittenText(const Scenario &scenario, const el::String &source, const std::uint64_t seed)
    -> el::String {
    if (scenario.method != Method::WriteLine) {
        return source;
    }
    auto schedule = ChunkSchedule{scenario, seed};
    auto result = el::StringEditor{};
    auto offset = std::uint64_t{};
    const auto length = source.characterLength().toRawValue();
    while (offset < length) {
        const auto count = schedule.next(length - offset);
        result.append(source.slice(
            el::CpRange{
                el::CpIndex::fromSizeTOrThrow(static_cast<std::size_t>(offset)),
                el::CpLength::fromSizeTOrThrow(static_cast<std::size_t>(count))}));
        result.append(U'\n');
        offset += count;
    }
    return result;
}

/// Create a fixture and, if requested, its input file.
[[nodiscard]] auto buildFixture(
    const Scenario &scenario,
    const el::Path &path,
    const std::uint64_t seed,
    const std::uint64_t size,
    const bool createInput) -> Fixture {
    auto result = Fixture{};
    result.path = path;
    if (scenario.fileType == FileType::Binary) {
        auto random = el::FastRandom{seed};
        result.binary = random.buildByteBlock(el::ByteLength{size});
        result.expectedRawDigest = digest(result.binary);
        result.rawLength = result.binary.length().toRawValue();
        if (createInput) {
            auto options = el::PathWriteDataOptions{};
            options.setCreateParents(true).setStreamSettings(
                el::OutputStreamSettings{}.setBackBufferLimit(el::ByteLength{result.rawLength}));
            path.content().writeDataOrThrow(result.binary, options);
            if (rawFileDigest(path, result.rawLength) != result.expectedRawDigest) {
                workloadError("Generated binary fixture failed MD5 validation."_el);
            }
        }
    } else {
        result.text = buildText(seed, size);
        result.expectedText = expectedWrittenText(scenario, result.text, seed);
        const auto encoded = el::StringEncoder{createInput ? result.text : result.expectedText}.encode(
            encodingFor(scenario.fileType), el::StringBomMode::Automatic);
        result.expectedRawDigest = digest(encoded);
        result.expectedTextDigest = digest(createInput ? result.text : result.expectedText);
        result.rawLength = encoded.length().toRawValue();
        if (createInput) {
            auto options = el::PathWriteDataOptions{};
            options.setCreateParents(true).setStreamSettings(
                el::OutputStreamSettings{}.setBackBufferLimit(el::ByteLength{result.rawLength}));
            path.content().writeDataOrThrow(encoded, options);
            if (rawFileDigest(path, result.rawLength) != result.expectedRawDigest) {
                workloadError("Generated text fixture failed raw MD5 validation."_el);
            }
        }
    }
    return result;
}

/// Select the deterministic file size for a scenario.
[[nodiscard]] auto selectedFileSize(const Configuration &configuration, const Scenario &scenario) -> std::uint64_t {
    auto random = el::FastRandom{scenarioSeed(configuration.run.seed, scenario, 0U, 0U)};
    return random.getUInt64(scenario.fileSizeMinimum, scenario.fileSizeMaximum);
}

/// Estimate the encoded physical size required by a scenario file.
[[nodiscard]] auto estimatedPhysicalFileSize(const Scenario &scenario, const std::uint64_t fileSize) noexcept
    -> std::uint64_t {
    if (scenario.fileType == FileType::Binary) {
        return fileSize;
    }
    if (fileSize > (std::numeric_limits<std::uint64_t>::max() - 64U) / 4U) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return fileSize * 4U + 64U;
}

/// Calculate the number of file slots that fit in the configured corpus.
[[nodiscard]] auto slotCount(const Configuration &configuration, const Scenario &scenario, const std::uint64_t fileSize)
    -> std::uint32_t {
    const auto threadCount = configuration.run.threadCount;
    const auto physicalSize = estimatedPhysicalFileSize(scenario, fileSize);
    const auto minimumCopies = scenario.direction == Direction::Read && scenario.locality == Locality::Hot
        ? std::uint64_t{1U}
        : static_cast<std::uint64_t>(threadCount);
    if (physicalSize > configuration.run.corpusLimit / minimumCopies) {
        workloadError(
            el::StringFormat{"Scenario '{}' cannot fit one file per required owner into the corpus limit."_el}.build(
                scenario.id));
    }
    if (scenario.locality == Locality::Hot) {
        return 1U;
    }
    const auto denominator = std::max<std::uint64_t>(1U, physicalSize * threadCount);
    return static_cast<std::uint32_t>(std::clamp<std::uint64_t>(configuration.run.corpusLimit / denominator, 1U, 4U));
}

/// Prepare fixtures and their workspace for one benchmark scenario.
[[nodiscard]] auto prepareScenario(
    const Configuration &configuration, const Scenario &scenario, const el::TempDirectoryPtr &workspace)
    -> PreparedScenario {
    auto directoryOptions = el::PathTempDirectoryOptions{};
    directoryOptions.setPrefix(el::StringFormat{"scenario-{}-"_el}.build(scenario.id.toHash()))
        .setRandomLength(el::CpLength{8U});
    auto scenarioWorkspace = workspace->path().operations().createTempDirectoryOrThrow(directoryOptions);
    if (configuration.run.keepFiles) {
        scenarioWorkspace->setRemoveOnDestroy(false);
    }
    auto result = PreparedScenario{.scenario = scenario, .fixtures = {}, .workspace = scenarioWorkspace};
    const auto threadCount = configuration.run.threadCount;
    const auto fileSize = selectedFileSize(configuration, scenario);
    const auto slots = slotCount(configuration, scenario, fileSize);
    result.fixtures.resize(threadCount);
    if (scenario.direction == Direction::Read && scenario.locality == Locality::Hot) {
        const auto path = scenarioWorkspace->path() / el::StringFormat{"{}-shared.dat"_el}.build(scenario.id.toHash());
        const auto fixture =
            buildFixture(scenario, path, scenarioSeed(configuration.run.seed, scenario, 0U, 0U), fileSize, true);
        for (auto &workerFixtures : result.fixtures) {
            workerFixtures.emplace_back(fixture);
        }
        return result;
    }
    for (auto worker = std::uint32_t{}; worker < threadCount; ++worker) {
        for (auto slot = std::uint32_t{}; slot < slots; ++slot) {
            const auto path = scenarioWorkspace->path() /
                el::StringFormat{"{}-worker-{}-slot-{}.dat"_el}.build(scenario.id.toHash(), worker, slot);
            const auto seed = scenarioSeed(configuration.run.seed, scenario, worker, slot);
            result.fixtures[worker].emplace_back(
                buildFixture(scenario, path, seed, fileSize, scenario.direction == Direction::Read));
        }
    }
    return result;
}

/// Add a byte range to the worker's counters and optional digest.
void addByteResult(WorkerResult &worker, el::cryptology::Hasher *hasher, const el::ConstByteSpan bytes) {
    worker.bytes += bytes.size();
    if (!bytes.empty()) {
        worker.sink ^= bytes.front().toUInt8();
    }
    if (hasher != nullptr) {
        hasher->update(bytes);
    }
}

/// Add text to the worker's counters and optional digest.
void addTextResult(WorkerResult &worker, el::cryptology::Hasher *hasher, const el::String &text) {
    worker.bytes += text.length().toRawValue();
    worker.codePoints += text.characterLength().toRawValue();
    if (!text.isEmpty()) {
        worker.sink ^= text.charAt(el::StringSide::Front).toRawValue();
    }
    if (hasher != nullptr) {
        hasher->update(text);
    }
}

/// Retry a timed output operation until it succeeds or the worker is cancelled.
template <typename Function>
void retryWrite(
    Function function, const el::OutputStreamPtr &stream, WorkerResult &result, const std::atomic_bool &cancelled) {
    while (true) {
        requireActive(cancelled);
        ++result.calls;
        if (function().isSuccess()) {
            return;
        }
        ++result.timeouts;
        const auto waitStatus = stream->waitForReady();
        if (waitStatus.isTimeout()) {
            continue;
        }
    }
}

/// Close an output stream while retrying bounded close attempts.
void closeOutput(const el::OutputStreamPtr &stream, WorkerResult &result) {
    for (auto attempt = 0U; attempt < 16U; ++attempt) {
        if (stream->close().isClosed()) {
            return;
        }
        ++result.timeouts;
        const auto waitStatus = stream->waitForReady();
        if (waitStatus.isTimeout()) {
            continue;
        }
    }
    workloadError("Output stream did not close after repeated timeout retries."_el);
}

/// Read a binary fixture according to the scenario and collect its measurements.
void readBinary(
    const Scenario &scenario,
    const Fixture &fixture,
    const std::uint64_t seed,
    const bool validate,
    WorkerResult &result,
    const std::atomic_bool &cancelled) {
    auto options = el::PathReadDataOptions{};
    options.setStreamSettings(
        el::InputStreamSettings{}.setBuffering(scenario.buffering).setTimeout(el::TimeDelta::seconds(30)));
    const auto openStart = Clock::now();
    const auto input = fixture.path.content().openByteInputStream(options);
    result.openNanoseconds = elapsedNanoseconds(openStart);
    auto hasher = el::cryptology::Hasher{el::cryptology::HashAlgorithm::Md5};
    auto *hasherPtr = validate ? &hasher : nullptr;
    auto schedule = ChunkSchedule{scenario, seed};
    auto transferStart = Clock::now();
    auto remaining = fixture.binary.length().toRawValue();
    if (scenario.method == Method::ReadAll) {
        while (true) {
            requireActive(cancelled);
            ++result.calls;
            const auto read = input->readAll(el::ByteLength{remaining + 1U});
            if (read.isTimeout()) {
                ++result.timeouts;
                continue;
            }
            if (!read.hasData()) {
                workloadError("readAll did not return data for a non-empty file."_el);
            }
            addByteResult(result, hasherPtr, read.data().span());
            break;
        }
    } else {
        auto buffer = std::vector<el::Byte>(static_cast<std::size_t>(scenario.chunkSizeMaximum));
        while (remaining > 0U) {
            requireActive(cancelled);
            if (scenario.method == Method::ReadByte) {
                ++result.calls;
                const auto read = input->readByte();
                if (read.isTimeout()) {
                    ++result.timeouts;
                    continue;
                }
                if (!read.hasData()) {
                    workloadError("Byte stream ended before the expected length."_el);
                }
                const auto one = el::ByteArray<1>{read.data()};
                addByteResult(result, hasherPtr, one.span());
                --remaining;
                continue;
            }
            const auto count = schedule.next(remaining);
            ++result.calls;
            if (scenario.method == Method::ReadSpan) {
                const auto read = input->read(el::ByteSpan{buffer}.first(static_cast<std::size_t>(count)));
                if (read.isTimeout()) {
                    ++result.timeouts;
                    continue;
                }
                if (!read.hasData()) {
                    workloadError("Byte stream ended before the expected length."_el);
                }
                addByteResult(result, hasherPtr, el::ConstByteSpan{buffer}.first(read.data().toSizeT()));
                remaining -= read.data().toRawValue();
            } else {
                const auto read = scenario.method == Method::ReadExact ? input->readExact(el::ByteLength{count})
                                                                       : input->read(el::ByteLength{count});
                if (read.isTimeout()) {
                    ++result.timeouts;
                    continue;
                }
                if (!read.hasData()) {
                    workloadError("Byte stream ended before the expected length."_el);
                }
                addByteResult(result, hasherPtr, read.data().span());
                remaining -= read.data().length().toRawValue();
            }
        }
    }
    if (result.bytes != fixture.rawLength) {
        workloadError("Binary stream returned an unexpected byte count."_el);
    }
    while (true) {
        requireActive(cancelled);
        ++result.calls;
        const auto end = input->readByte();
        if (end.isTimeout()) {
            ++result.timeouts;
            continue;
        }
        if (!end.isFinished()) {
            workloadError("Binary stream did not report end-of-stream after the expected length."_el);
        }
        break;
    }
    result.transferNanoseconds = elapsedNanoseconds(transferStart);
    const auto closeStart = Clock::now();
    if (!input->close().isClosed()) {
        workloadError("Binary input stream did not close successfully."_el);
    }
    result.closeNanoseconds = elapsedNanoseconds(closeStart);
    if (validate) {
        result.digest = hasher.finalize();
    }
    result.bytes = fixture.rawLength;
}

/// Write a binary fixture according to the scenario and collect its measurements.
void writeBinary(
    const Scenario &scenario,
    const Fixture &fixture,
    const std::uint64_t seed,
    WorkerResult &result,
    const std::atomic_bool &cancelled) {
    auto settings = el::OutputStreamSettings{};
    settings.setBuffering(scenario.buffering).setTimeout(el::TimeDelta::seconds(30));
    if (scenario.backBufferLimit != 0U) {
        settings.setBackBufferLimit(el::ByteLength{scenario.backBufferLimit});
    }
    auto options = el::PathWriteDataOptions{};
    options.setCreateParents(true).setCreationMode(el::PathCreateMode::CreateOrOverwrite).setStreamSettings(settings);
    const auto openStart = Clock::now();
    const auto output = fixture.path.content().openByteOutputStream(options);
    result.openNanoseconds = elapsedNanoseconds(openStart);
    auto schedule = ChunkSchedule{scenario, seed};
    const auto transferStart = Clock::now();
    auto offset = std::uint64_t{};
    const auto length = fixture.binary.length().toRawValue();
    while (offset < length) {
        requireActive(cancelled);
        if (scenario.method == Method::WriteByte) {
            retryWrite(
                [&]() { return output->write(fixture.binary.get(el::ByteIndex{offset})); }, output, result, cancelled);
            ++offset;
            ++result.bytes;
            continue;
        }
        const auto count = schedule.next(length - offset);
        const auto bytes = fixture.binary.slice(el::ByteIndex{offset}, el::ByteLength{count});
        if (scenario.method == Method::WriteSpan) {
            retryWrite([&]() { return output->write(bytes.span()); }, output, result, cancelled);
        } else {
            retryWrite([&]() { return output->write(bytes); }, output, result, cancelled);
        }
        offset += count;
        result.bytes += count;
    }
    result.transferNanoseconds = elapsedNanoseconds(transferStart);
    const auto closeStart = Clock::now();
    closeOutput(output, result);
    result.closeNanoseconds = elapsedNanoseconds(closeStart);
    result.bytes = fixture.rawLength;
}

/// Read a text fixture according to the scenario and collect its measurements.
void readText(
    const Scenario &scenario,
    const Fixture &fixture,
    const std::uint64_t seed,
    const bool validate,
    WorkerResult &result,
    const std::atomic_bool &cancelled) {
    auto options = el::PathReadTextOptions{encodingFor(scenario.fileType)};
    options.setBuffering(scenario.buffering)
        .setTimeout(el::TimeDelta::seconds(30))
        .setBomMode(el::StringBomMode::Automatic)
        .setEncodingMode(el::EncodingMode::Strict);
    const auto openStart = Clock::now();
    const auto input = fixture.path.content().openTextInputStream(options);
    result.openNanoseconds = elapsedNanoseconds(openStart);
    auto hasher = el::cryptology::Hasher{el::cryptology::HashAlgorithm::Md5};
    auto *hasherPtr = validate ? &hasher : nullptr;
    auto schedule = ChunkSchedule{scenario, seed};
    const auto expectedCodePoints = fixture.text.characterLength().toRawValue();
    const auto transferStart = Clock::now();
    while (true) {
        requireActive(cancelled);
        ++result.calls;
        if (scenario.method == Method::ReadChar) {
            const auto read = input->readChar();
            if (read.isTimeout()) {
                ++result.timeouts;
                continue;
            }
            if (read.isFinished()) {
                break;
            }
            const auto text = el::String::fromCharacter(read.data());
            addTextResult(result, hasherPtr, text);
            continue;
        }
        const auto maximumRaw = scenario.method == Method::ReadAllText
            ? static_cast<std::uint64_t>(expectedCodePoints) + 1U
            : schedule.next();
        const auto maximum = el::CpLength::fromSizeTOrThrow(static_cast<std::size_t>(maximumRaw));
        const auto read = scenario.method == Method::ReadText ? input->read(maximum)
            : scenario.method == Method::ReadLine             ? input->readLine(maximum)
                                                              : input->readAll(maximum);
        if (read.isTimeout()) {
            ++result.timeouts;
            continue;
        }
        if (read.isFinished()) {
            break;
        }
        addTextResult(result, hasherPtr, read.data());
    }
    if (result.codePoints != expectedCodePoints) {
        workloadError(
            el::StringFormat{"Text stream returned {} characters, expected {}."_el}.build(
                result.codePoints, expectedCodePoints));
    }
    result.transferNanoseconds = elapsedNanoseconds(transferStart);
    const auto closeStart = Clock::now();
    if (!input->close().isClosed()) {
        workloadError("Text input stream did not close successfully."_el);
    }
    result.closeNanoseconds = elapsedNanoseconds(closeStart);
    if (validate) {
        result.digest = hasher.finalize();
    }
    result.bytes = fixture.rawLength;
}

/// Write a text fixture according to the scenario and collect its measurements.
void writeText(
    const Scenario &scenario,
    const Fixture &fixture,
    const std::uint64_t seed,
    WorkerResult &result,
    const std::atomic_bool &cancelled) {
    auto settings = el::OutputStreamSettings{};
    settings.setBuffering(scenario.buffering).setTimeout(el::TimeDelta::seconds(30));
    if (scenario.backBufferLimit != 0U) {
        settings.setBackBufferLimit(el::ByteLength{scenario.backBufferLimit});
    }
    auto options = el::PathWriteTextOptions{encodingFor(scenario.fileType)};
    options.setCreateParents(true)
        .setCreationMode(el::PathCreateMode::CreateOrOverwrite)
        .setStreamSettings(settings)
        .setBomMode(el::StringBomMode::Automatic);
    const auto openStart = Clock::now();
    const auto output = fixture.path.content().openTextOutputStream(options);
    result.openNanoseconds = elapsedNanoseconds(openStart);
    auto schedule = ChunkSchedule{scenario, seed};
    const auto transferStart = Clock::now();
    auto reader = el::StringCharReader{fixture.text};
    auto offset = std::uint64_t{};
    const auto length = fixture.text.characterLength().toRawValue();
    while (offset < length) {
        requireActive(cancelled);
        if (scenario.method == Method::WriteChar) {
            const auto character = reader.read();
            retryWrite([&]() { return output->write(character); }, output, result, cancelled);
            ++offset;
            ++result.codePoints;
            continue;
        }
        const auto count = schedule.next(length - offset);
        reader.startCapture();
        reader.advanceOrThrow(el::CpLength::fromSizeTOrThrow(static_cast<std::size_t>(count)));
        const auto text = reader.takeCapture().toString();
        if (scenario.method == Method::WriteLine) {
            retryWrite([&]() { return output->writeLine(text); }, output, result, cancelled);
        } else {
            retryWrite([&]() { return output->write(text); }, output, result, cancelled);
        }
        offset += count;
        result.codePoints += count;
        result.bytes += text.length().toRawValue();
    }
    result.transferNanoseconds = elapsedNanoseconds(transferStart);
    const auto closeStart = Clock::now();
    closeOutput(output, result);
    result.closeNanoseconds = elapsedNanoseconds(closeStart);
    result.bytes = fixture.rawLength;
}

/// Execute one worker for a prepared scenario sample.
[[nodiscard]] auto executeWorker(
    const PreparedScenario &prepared,
    const Configuration &configuration,
    const std::uint32_t workerIndex,
    const std::uint64_t sampleIndex,
    const bool validate,
    const std::atomic_bool &cancelled) -> WorkerResult {
    const auto &workerFixtures = prepared.fixtures[workerIndex];
    const auto fixtureIndex = static_cast<std::size_t>(sampleIndex % workerFixtures.size());
    const auto &fixture = workerFixtures[fixtureIndex];
    const auto seed =
        scenarioSeed(configuration.run.seed, prepared.scenario, workerIndex, static_cast<std::uint32_t>(fixtureIndex));
    auto result = WorkerResult{};
    if (prepared.scenario.fileType == FileType::Binary) {
        if (prepared.scenario.direction == Direction::Read) {
            readBinary(prepared.scenario, fixture, seed, validate, result, cancelled);
        } else {
            writeBinary(prepared.scenario, fixture, seed, result, cancelled);
        }
    } else if (prepared.scenario.direction == Direction::Read) {
        readText(prepared.scenario, fixture, seed, validate, result, cancelled);
    } else {
        writeText(prepared.scenario, fixture, seed, result, cancelled);
    }
    return result;
}

/// Verify the output digest for one worker fixture.
void validateFixtureOutput(const PreparedScenario &prepared, const std::uint32_t worker, const std::uint64_t sample) {
    const auto &fixtures = prepared.fixtures[worker];
    const auto &fixture = fixtures[static_cast<std::size_t>(sample % fixtures.size())];
    const auto actual = rawFileDigest(fixture.path, prepared.scenario.fileSizeMaximum * 5U + 1024U);
    if (actual != fixture.expectedRawDigest) {
        workloadError(
            el::StringFormat{"MD5 mismatch for scenario '{}', worker {}, file '{}'."_el}.build(
                prepared.scenario.id, worker, fixture.path.toString()));
    }
}

/// Verify the output digests of all prepared fixtures.
void validateAllOutputs(const PreparedScenario &prepared) {
    for (auto worker = std::uint32_t{}; worker < prepared.fixtures.size(); ++worker) {
        for (auto slot = std::size_t{}; slot < prepared.fixtures[worker].size(); ++slot) {
            validateFixtureOutput(prepared, worker, slot);
        }
    }
}

/// Run one parallel sample and return the aggregated measurements.
auto runSample(
    const PreparedScenario &prepared,
    const Configuration &configuration,
    const std::uint64_t sampleIndex,
    const bool validate) -> SampleResult {
    const auto threadCount = configuration.run.threadCount;
    const auto failureVariable = el::system::EnvironmentVariables{}.get("ERBSLAND_STREAM_PROFILE_TEST_FAIL_WORKER"_el);
    const auto injectWorkerFailure = failureVariable.has_value() && *failureVariable == "1"_el;
    auto result = SampleResult{};
    result.workers.resize(threadCount);
    auto cancelled = std::atomic_bool{false};
    auto firstError = std::exception_ptr{};
    auto firstErrorWorker = std::uint32_t{};
    auto errorMutex = std::mutex{};
    auto startBarrier = std::barrier{static_cast<std::ptrdiff_t>(threadCount + 1U)};
    auto threads = std::vector<std::thread>{};
    threads.reserve(threadCount);
    for (auto worker = std::uint32_t{}; worker < threadCount; ++worker) {
        threads.emplace_back([&, worker]() -> void {
            startBarrier.arrive_and_wait();
            try {
                if (injectWorkerFailure && worker == 0U) {
                    workloadError("Injected worker failure for propagation testing."_el);
                }
                result.workers[worker] =
                    executeWorker(prepared, configuration, worker, sampleIndex, validate, cancelled);
            } catch (...) {
                const auto lock = std::scoped_lock{errorMutex};
                if (!firstError) {
                    firstError = std::current_exception();
                    firstErrorWorker = worker;
                }
                cancelled.store(true, std::memory_order_relaxed);
            }
        });
    }
    const auto start = Clock::now();
    startBarrier.arrive_and_wait();
    for (auto &thread : threads) {
        thread.join();
    }
    result.wallNanoseconds = elapsedNanoseconds(start);
    if (firstError) {
        el::io::printLine(
            "record=summary status=worker-error scenario="_el,
            prepared.scenario.id,
            " worker="_el,
            firstErrorWorker,
            " sample="_el,
            sampleIndex);
        std::rethrow_exception(firstError);
    }
    for (const auto &worker : result.workers) {
        result.bytes += worker.bytes;
        result.calls += worker.calls;
        result.timeouts += worker.timeouts;
    }
    if (validate && prepared.scenario.direction == Direction::Read) {
        for (auto worker = std::uint32_t{}; worker < threadCount; ++worker) {
            const auto &fixtures = prepared.fixtures[worker];
            const auto &fixture = fixtures[static_cast<std::size_t>(sampleIndex % fixtures.size())];
            validateFixtureOutput(prepared, worker, sampleIndex);
            if (result.workers[worker].digest !=
                (prepared.scenario.fileType == FileType::Binary ? fixture.expectedRawDigest
                                                                : fixture.expectedTextDigest)) {
                workloadError(
                    el::StringFormat{"Decoded MD5 mismatch for scenario '{}', worker {}."_el}.build(
                        prepared.scenario.id, worker));
            }
        }
    }
    if (validate && prepared.scenario.direction == Direction::Write) {
        for (auto worker = std::uint32_t{}; worker < threadCount; ++worker) {
            validateFixtureOutput(prepared, worker, sampleIndex);
        }
    }
    return result;
}

/// Calculate latency statistics from the supplied values.
[[nodiscard]] auto calculateStatistics(std::vector<std::int64_t> values) -> Statistics {
    std::ranges::sort(values);
    const auto sum = std::accumulate(values.begin(), values.end(), std::int64_t{});
    return {
        .minimum = values.front(),
        .median = values[values.size() / 2U],
        .p95 = values[std::min(values.size() - 1U, (values.size() * 95U) / 100U)],
        .maximum = values.back(),
        .mean = static_cast<double>(sum) / static_cast<double>(values.size()),
    };
}

/// Convert transferred bytes and elapsed time to MiB per second.
[[nodiscard]] auto throughput(const std::uint64_t bytes, const std::int64_t nanoseconds) noexcept -> double {
    return nanoseconds > 0
        ? static_cast<double>(bytes) * 1'000'000'000.0 / (static_cast<double>(nanoseconds) * 1024.0 * 1024.0)
        : 0.0;
}

/// Calculate per-worker throughput fairness across samples.
[[nodiscard]] auto calculateFairness(const std::vector<SampleResult> &samples, const std::uint32_t threadCount)
    -> WorkerFairness {
    auto workerBytes = std::vector<std::uint64_t>(threadCount);
    auto workerNanoseconds = std::vector<std::int64_t>(threadCount);
    for (const auto &sample : samples) {
        for (auto worker = std::uint32_t{}; worker < threadCount; ++worker) {
            workerBytes[worker] += sample.workers[worker].bytes;
            workerNanoseconds[worker] += sample.workers[worker].transferNanoseconds;
        }
    }
    auto rates = std::vector<double>{};
    rates.reserve(threadCount);
    for (auto worker = std::uint32_t{}; worker < threadCount; ++worker) {
        rates.emplace_back(throughput(workerBytes[worker], workerNanoseconds[worker]));
    }
    const auto [minimum, maximum] = std::ranges::minmax_element(rates);
    const auto mean = std::accumulate(rates.begin(), rates.end(), 0.0) / static_cast<double>(rates.size());
    auto squaredDifference = 0.0;
    for (const auto rate : rates) {
        const auto difference = rate - mean;
        squaredDifference += difference * difference;
    }
    const auto deviation = std::sqrt(squaredDifference / static_cast<double>(rates.size()));
    return {
        .minimum = *minimum,
        .maximum = *maximum,
        .coefficientOfVariation = mean > 0.0 ? deviation / mean : 0.0,
    };
}

/// Calculate aggregate throughput statistics across samples.
[[nodiscard]] auto calculateThroughputStatistics(const std::vector<SampleResult> &samples) -> ThroughputStatistics {
    auto values = std::vector<double>{};
    values.reserve(samples.size());
    for (const auto &sample : samples) {
        values.emplace_back(throughput(sample.bytes, sample.wallNanoseconds));
    }
    std::ranges::sort(values);
    return {
        .minimum = values.front(),
        .median = values[values.size() / 2U],
        .mean = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size()),
        .p95 = values[std::min(values.size() - 1U, (values.size() * 95U) / 100U)],
        .maximum = values.back(),
    };
}

/// Print the measurements for one scenario sample.
void printSample(const Configuration &configuration, const Scenario &scenario, const SampleResult &sample) {
    const auto aggregateThroughput = throughput(sample.bytes, sample.wallNanoseconds);
    const auto bufferSizes = el::stream::impl::streamBufferSizes(scenario.buffering);
    const auto effectiveBackLimit =
        scenario.backBufferLimit != 0U ? scenario.backBufferLimit : bufferSizes.outputBackLimit.toRawValue();
    auto openNanoseconds = std::int64_t{};
    auto transferNanoseconds = std::int64_t{};
    auto closeNanoseconds = std::int64_t{};
    for (const auto &worker : sample.workers) {
        openNanoseconds += worker.openNanoseconds;
        transferNanoseconds += worker.transferNanoseconds;
        closeNanoseconds += worker.closeNanoseconds;
    }
    el::io::printLine(
        "record=scenario id="_el,
        scenario.id,
        " mode="_el,
        toString(configuration.run.mode),
        " workload-threads="_el,
        configuration.run.threadCount,
        " file-type="_el,
        toString(scenario.fileType),
        " method="_el,
        toString(scenario.method),
        " locality="_el,
        toString(scenario.locality),
        " file-min="_el,
        scenario.fileSizeMinimum,
        " file-max="_el,
        scenario.fileSizeMaximum,
        " chunk-min="_el,
        scenario.chunkSizeMinimum,
        " chunk-max="_el,
        scenario.chunkSizeMaximum,
        " buffering="_el,
        toString(scenario.buffering),
        " ring="_el,
        bufferSizes.ioRing.toRawValue(),
        " aggregate="_el,
        bufferSizes.aggregateChunk.toRawValue(),
        " decoder="_el,
        bufferSizes.decoder.toRawValue(),
        " output-retained-initial="_el,
        bufferSizes.outputRetainedInitial.toRawValue(),
        " back-limit="_el,
        effectiveBackLimit,
        " bytes="_el,
        sample.bytes,
        " calls="_el,
        sample.calls,
        " timeouts="_el,
        sample.timeouts,
        " wall-ns="_el,
        sample.wallNanoseconds,
        " open-worker-ns="_el,
        openNanoseconds,
        " transfer-worker-ns="_el,
        transferNanoseconds,
        " close-worker-ns="_el,
        closeNanoseconds,
        " mib-per-second="_el,
        aggregateThroughput);
}

/// Print validation coverage for one scenario sample.
void printCoverage(const Scenario &scenario, const SampleResult &sample) {
    el::io::printLine(
        "record=coverage scenario="_el,
        scenario.id,
        " file-type="_el,
        toString(scenario.fileType),
        " method="_el,
        toString(scenario.method),
        " bytes="_el,
        sample.bytes,
        " calls="_el,
        sample.calls,
        " timeouts="_el,
        sample.timeouts,
        " validation=md5-ok"_el);
}

/// Print the aggregate benchmark measurements for a scenario.
void printBenchmark(
    const Configuration &configuration, const Scenario &scenario, const std::vector<SampleResult> &samples) {
    const auto bufferSizes = el::stream::impl::streamBufferSizes(scenario.buffering);
    const auto effectiveBackLimit =
        scenario.backBufferLimit != 0U ? scenario.backBufferLimit : bufferSizes.outputBackLimit.toRawValue();
    auto times = std::vector<std::int64_t>{};
    times.reserve(samples.size());
    auto bytes = std::uint64_t{};
    auto calls = std::uint64_t{};
    auto timeouts = std::uint64_t{};
    auto openNanoseconds = std::int64_t{};
    auto transferNanoseconds = std::int64_t{};
    auto closeNanoseconds = std::int64_t{};
    for (const auto &sample : samples) {
        times.emplace_back(sample.wallNanoseconds);
        bytes += sample.bytes;
        calls += sample.calls;
        timeouts += sample.timeouts;
        for (const auto &worker : sample.workers) {
            openNanoseconds += worker.openNanoseconds;
            transferNanoseconds += worker.transferNanoseconds;
            closeNanoseconds += worker.closeNanoseconds;
        }
    }
    const auto statistics = calculateStatistics(std::move(times));
    const auto throughputStatistics = calculateThroughputStatistics(samples);
    const auto fairness = calculateFairness(samples, configuration.run.threadCount);
    const auto workerObservations = static_cast<double>(samples.size() * configuration.run.threadCount);
    el::io::printLine(
        "record=benchmark scenario="_el,
        scenario.id,
        " workload-threads="_el,
        configuration.run.threadCount,
        " file-type="_el,
        toString(scenario.fileType),
        " method="_el,
        toString(scenario.method),
        " locality="_el,
        toString(scenario.locality),
        " file-min="_el,
        scenario.fileSizeMinimum,
        " file-max="_el,
        scenario.fileSizeMaximum,
        " chunk-min="_el,
        scenario.chunkSizeMinimum,
        " chunk-max="_el,
        scenario.chunkSizeMaximum,
        " buffering="_el,
        toString(scenario.buffering),
        " ring="_el,
        bufferSizes.ioRing.toRawValue(),
        " aggregate="_el,
        bufferSizes.aggregateChunk.toRawValue(),
        " decoder="_el,
        bufferSizes.decoder.toRawValue(),
        " output-retained-initial="_el,
        bufferSizes.outputRetainedInitial.toRawValue(),
        " back-limit="_el,
        effectiveBackLimit,
        " samples="_el,
        samples.size(),
        " bytes="_el,
        bytes,
        " calls="_el,
        calls,
        " timeouts="_el,
        timeouts,
        " min-ns="_el,
        statistics.minimum,
        " median-ns="_el,
        statistics.median,
        " mean-ns="_el,
        statistics.mean,
        " p95-ns="_el,
        statistics.p95,
        " max-ns="_el,
        statistics.maximum,
        " min-mib-per-second="_el,
        throughputStatistics.minimum,
        " median-mib-per-second="_el,
        throughputStatistics.median,
        " mean-mib-per-second="_el,
        throughputStatistics.mean,
        " p95-mib-per-second="_el,
        throughputStatistics.p95,
        " max-mib-per-second="_el,
        throughputStatistics.maximum,
        " open-mean-worker-ns="_el,
        static_cast<double>(openNanoseconds) / workerObservations,
        " transfer-mean-worker-ns="_el,
        static_cast<double>(transferNanoseconds) / workerObservations,
        " close-mean-worker-ns="_el,
        static_cast<double>(closeNanoseconds) / workerObservations,
        " fairness-min-mib-per-second="_el,
        fairness.minimum,
        " fairness-max-mib-per-second="_el,
        fairness.maximum,
        " fairness-cv="_el,
        fairness.coefficientOfVariation);
}

/// Calculate the digest that identifies the effective benchmark configuration.
[[nodiscard]] auto configurationDigest(const Configuration &configuration) -> el::ByteBlock {
    auto effective = el::StringEditor{};
    effective.append(
        el::StringFormat{
            "\n# effective mode={} suite={} duration={} threads={} seed={} corpus={} progress={} workspace={} "
            "keep={}\n"_el}
            .build(
                toString(configuration.run.mode),
                configuration.run.suite,
                configuration.run.duration.count(),
                configuration.run.threadCount,
                configuration.run.seed,
                configuration.run.corpusLimit,
                configuration.run.progressInterval.count(),
                configuration.run.workspace,
                configuration.run.keepFiles));
    for (const auto &scenario : configuration.scenarios) {
        effective.append(
            el::StringFormat{"{} {} {} {} {} {} {} {} {} {} {} {} {} {}\n"_el}.build(
                scenario.id,
                scenario.group,
                toString(scenario.direction),
                toString(scenario.fileType),
                toString(scenario.method),
                toString(scenario.locality),
                toString(scenario.chunkMode),
                scenario.fileSizeMinimum,
                scenario.fileSizeMaximum,
                scenario.chunkSizeMinimum,
                scenario.chunkSizeMaximum,
                toString(scenario.buffering),
                scenario.backBufferLimit,
                scenario.weight));
    }
    return digest(effective);
}

}
