// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Corpus.hpp"

#include "CorpusDirectory.hpp"

#include <erbsland/profiling/Seed.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <random>
#include <vector>

namespace app::compression {

namespace el = erbsland;
namespace pf = erbsland::profiling;

using namespace el::text::literals;

Corpus::Corpus() :
    _entries{
        CorpusEntry{
            "prose"_el,
            "Multilingual natural-language text with varied lines and punctuation."_el,
            CorpusKind::SeedFile,
            "prose.txt"_el,
            11757646895756958825ULL},
        CorpusEntry{
            "structured"_el,
            "Nested structured records with repeated field names and varied values."_el,
            CorpusKind::SeedFile,
            "records.json"_el,
            7949590349430545083ULL},
        CorpusEntry{
            "source"_el,
            "C++-like source text with identifiers, indentation, and punctuation."_el,
            CorpusKind::SeedFile,
            "source.cpp"_el,
            8773910176654656920ULL},
        CorpusEntry{
            "runs"_el,
            "Short and extended byte runs spanning token and RLE boundaries."_el,
            CorpusKind::Runs,
            {},
            4526904750125495025ULL},
        CorpusEntry{
            "matches"_el,
            "Near and long-distance matches with deterministic mutations."_el,
            CorpusKind::Matches,
            {},
            14668094390913816767ULL},
        CorpusEntry{
            "sparse-binary"_el,
            "Zero-heavy binary records with counters and random islands."_el,
            CorpusKind::SparseBinary,
            {},
            16895624476744074025ULL},
        CorpusEntry{
            "entropy"_el,
            "Uniform deterministic bytes representing incompressible input."_el,
            CorpusKind::Entropy,
            {},
            15749438015918057378ULL},
        CorpusEntry{
            "mixed"_el,
            "Interleaved text, runs, matches, sparse records, and entropy."_el,
            CorpusKind::Mixed,
            {},
            4873325999941141621ULL},
    } {
}

auto Corpus::entries() const noexcept -> const el::List<CorpusEntry> & {
    return _entries;
}

auto Corpus::find(const el::String &id) const noexcept -> const CorpusEntry * {
    for (const auto &entry : _entries) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

auto Corpus::build(const CorpusEntry &entry, const el::ByteLength size, const std::uint64_t globalSeed) const
    -> el::ByteBlock {
    constexpr auto cCanonicalSize = std::uint64_t{1024U * 1024U};
    constexpr auto cCanonicalSeed = std::uint64_t{0x455242534c414e44ULL};
    const auto rawSize = size.toRawValue();
    const auto corpusSeed = pf::deriveSeed(globalSeed, entry.id, static_cast<std::uint32_t>(rawSize), rawSize >> 32U);
    auto result = el::ByteBlock{};
    switch (entry.kind) {
    case CorpusKind::SeedFile:
        result = expandSeed(loadSeed(entry.sourceFile), size, corpusSeed);
        break;
    case CorpusKind::Runs:
        result = buildRuns(size, corpusSeed);
        break;
    case CorpusKind::Matches:
        result = buildMatches(size, corpusSeed);
        break;
    case CorpusKind::SparseBinary:
        result = buildSparseBinary(size, corpusSeed);
        break;
    case CorpusKind::Entropy:
        result = buildEntropy(size, corpusSeed);
        break;
    case CorpusKind::Mixed:
        result = buildMixed(size, corpusSeed);
        break;
    }
    if (rawSize == cCanonicalSize && globalSeed == cCanonicalSeed) {
        const auto actual = digest(result);
        if (entry.canonicalDigest == 0U) {
            throw el::ApplicationError{
                el::StringFormat{"Canonical corpus '{}' digest is {}."_el}.build(entry.id, actual)};
        }
        if (actual != entry.canonicalDigest) {
            throw el::ApplicationError{el::StringFormat{"Canonical corpus '{}' digest changed from {} to {}."_el}.build(
                entry.id, entry.canonicalDigest, actual)};
        }
    }
    return result;
}

auto Corpus::digest(const el::ByteBlock &data) const noexcept -> std::uint64_t {
    auto result = std::uint64_t{14695981039346656037ULL};
    for (const auto byte : data.span()) {
        result ^= byte.toUInt64();
        result *= 1099511628211ULL;
    }
    return result;
}

auto Corpus::loadSeed(const el::String &fileName) const -> el::ByteBlock {
    auto options = el::path::PathReadDataOptions{};
    options.setMaximumByteLength(el::ByteLength{1024U * 1024U});
    const auto result = (el::Path{el::String{cCorpusDirectory}} / fileName).content().readDataOrThrow(options);
    if (result.isEmpty()) {
        throw el::ApplicationError{"A compression corpus seed file is empty."_el};
    }
    return result;
}

auto Corpus::expandSeed(const el::ByteBlock &seed, const el::ByteLength size, const std::uint64_t markerSeed) const
    -> el::ByteBlock {
    auto result = el::ByteBlockEditor{};
    result.reserve(size);
    auto repetition = std::uint64_t{};
    while (result.length() < size) {
        const auto remaining = size.toSizeTOrThrow() - result.length().toSizeTOrThrow();
        const auto count = std::min(remaining, seed.span().size());
        result.append(seed.span().first(count));
        if (count < seed.span().size() || result.length() == size) {
            break;
        }
        const auto marker = markerSeed ^ (repetition++ * 0x9e3779b97f4a7c15ULL);
        for (auto shift = 0U; shift < 64U && result.length() < size; shift += 8U) {
            result.append(el::Byte::fromCroppedUInt64(marker >> shift));
        }
    }
    return el::ByteBlock{result};
}

auto Corpus::buildRuns(const el::ByteLength size, const std::uint64_t seed) const -> el::ByteBlock {
    static constexpr auto cRunLengths =
        std::array<std::size_t, 14U>{1U, 2U, 3U, 4U, 5U, 14U, 15U, 16U, 31U, 255U, 259U, 260U, 4096U, 65'537U};
    auto result = el::ByteBlockEditor{};
    result.reserve(size);
    auto runIndex = std::size_t{};
    while (result.length() < size) {
        const auto remaining = size.toSizeTOrThrow() - result.length().toSizeTOrThrow();
        const auto length = std::min(remaining, cRunLengths[runIndex % cRunLengths.size()]);
        const auto value = el::Byte::fromCroppedUInt64(seed + runIndex * 37U + runIndex / cRunLengths.size());
        result.append(value, el::ByteLength::fromSizeT(length));
        ++runIndex;
    }
    return el::ByteBlock{result};
}

auto Corpus::buildMatches(const el::ByteLength size, const std::uint64_t seed) const -> el::ByteBlock {
    const auto target = size.toSizeTOrThrow();
    auto bytes = std::vector<std::uint8_t>(target);
    auto random = std::mt19937_64{seed};
    const auto prefix = std::min(target, std::size_t{65'536U});
    for (auto index = std::size_t{}; index < prefix; ++index) {
        bytes[index] = static_cast<std::uint8_t>(random());
    }
    static constexpr auto cDistances = std::array<std::size_t, 5U>{4U, 257U, 32'768U, 65'535U, 262'144U};
    for (auto index = prefix; index < target; ++index) {
        const auto distance = cDistances[(index / 4096U) % cDistances.size()];
        bytes[index] = distance <= index ? bytes[index - distance] : static_cast<std::uint8_t>(random());
        if (index % 4093U == 0U) {
            bytes[index] ^= static_cast<std::uint8_t>((index + seed) & 0xffU);
        }
    }
    return el::ByteBlock::fromVector(bytes);
}

auto Corpus::buildSparseBinary(const el::ByteLength size, const std::uint64_t seed) const -> el::ByteBlock {
    const auto target = size.toSizeTOrThrow();
    auto bytes = std::vector<std::uint8_t>(target, 0U);
    auto random = std::mt19937_64{seed};
    for (auto record = std::size_t{}; record < target; record += 4096U) {
        for (auto offset = std::size_t{}; offset < 8U && record + offset < target; ++offset) {
            bytes[record + offset] = static_cast<std::uint8_t>((record >> (offset * 8U)) & 0xffU);
        }
        const auto islandEnd = std::min(target, record + 96U);
        for (auto index = record + std::min(std::size_t{16U}, target - record); index < islandEnd; ++index) {
            bytes[index] = static_cast<std::uint8_t>(random());
        }
    }
    return el::ByteBlock::fromVector(bytes);
}

auto Corpus::buildEntropy(const el::ByteLength size, const std::uint64_t seed) const -> el::ByteBlock {
    auto random = std::mt19937_64{seed};
    auto bytes = std::vector<std::uint8_t>(size.toSizeTOrThrow());
    for (auto &byte : bytes) {
        byte = static_cast<std::uint8_t>(random());
    }
    return el::ByteBlock::fromVector(bytes);
}

auto Corpus::buildMixed(const el::ByteLength size, const std::uint64_t seed) const -> el::ByteBlock {
    const auto target = size.toSizeTOrThrow();
    const auto quarter = target / 4U;
    const auto lengths = std::array<std::size_t, 4U>{quarter, quarter, quarter, target - quarter * 3U};
    auto result = el::ByteBlockEditor{};
    result.reserve(size);
    result.append(buildRuns(el::ByteLength::fromSizeT(lengths[0U]), seed ^ 0x11U));
    result.append(buildMatches(el::ByteLength::fromSizeT(lengths[1U]), seed ^ 0x22U));
    result.append(buildSparseBinary(el::ByteLength::fromSizeT(lengths[2U]), seed ^ 0x33U));
    result.append(buildEntropy(el::ByteLength::fromSizeT(lengths[3U]), seed ^ 0x44U));
    return el::ByteBlock{result};
}

}
