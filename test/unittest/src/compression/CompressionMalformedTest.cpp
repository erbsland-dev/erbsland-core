// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ParallelTestWork.hpp"

#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/ByteCompressor.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteDecompressor.hpp>
#include <erbsland/CompressionError.hpp>
#include <erbsland/OutOfRangeError.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

TESTED_TARGETS(ByteDecompressor)
class CompressionMalformedTest final : public el::UnitTest {
public:
    void testMutatedStreamsLight() { WITH_CONTEXT(checkStreams(false)); }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMutatedStreams() { WITH_CONTEXT(checkStreams(true)); }

private:
    void checkStreams(const bool full) {
        using namespace el::compression;
        auto source = el::mem::ByteBlockEditor{};
        for (auto index = uint32_t{}; index < (full ? 512U : 32U); ++index) {
            source.append(el::mem::Byte::fromCroppedUInt32((index * 17U + index / 11U) % 23U + 'a'));
        }
        using TestCase = std::pair<CompressionAlgorithm, CompressionFormat>;
        auto cases = std::vector<TestCase>{};
        for (const auto algorithm : CompressionAlgorithm::all()) {
            for (const auto format : {CompressionFormat::Raw, CompressionFormat::Zip, CompressionFormat::Core}) {
                if ((!full && format != CompressionFormat::Raw) ||
                    (algorithm == CompressionAlgorithm::Lz4Block && format == CompressionFormat::Zip)) {
                    continue;
                }
                cases.emplace_back(algorithm, format);
            }
        }
        const auto data = el::mem::ByteBlock{source};
        const auto results = parallel_test_work::run(
            cases.size(),
            [&](const std::size_t caseIndex) -> el::unit::ByteCount {
                const auto &[algorithm, format] = cases[caseIndex];
                const auto encoded = ByteCompressor{algorithm, format}.compress(data);
                const auto options = DecompressionOptions{}
                                         .setExpectedOutputLength(data.length())
                                         .setMaximumOutputLength(el::unit::ByteCount{32768U})
                                         .setMaximumWorkspaceLength(el::unit::ByteCount{64U * 1024U * 1024U});
                const auto decoder = ByteDecompressor{algorithm, format, options};
                auto maximumLength = el::unit::ByteCount{};
                const auto step = full ? std::size_t{1U} : encoded.span().size();
                for (auto index = std::size_t{}; index < encoded.span().size(); index += step) {
                    const auto position = el::unit::ByteIndex::fromSizeT(index);
                    maximumLength = std::max(
                        maximumLength,
                        decodeBounded(decoder, encoded.slice({}, el::unit::ByteCount::fromSizeT(index))));
                    auto corrupted = el::mem::ByteBlockEditor{encoded};
                    const auto mask = el::mem::Byte::fromCroppedUInt32(1U << (index % 8U));
                    corrupted.setOrThrow(position, encoded.getOrThrow(position) ^ mask);
                    maximumLength = std::max(maximumLength, decodeBounded(decoder, corrupted));
                }
                return maximumLength;
            },
            2U);
        for (auto index = std::size_t{}; index < cases.size(); ++index) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    REQUIRE_NOTHROW(results[index].value());
                    REQUIRE_LESS_EQUAL(results[index].value(), el::unit::ByteCount{32768U});
                },
                [&]() -> std::string {
                    using namespace el::text::literals;
                    return el::text::StringConverter{
                        el::text::StringFormat{"algorithm={} format={}"_el}.build(
                            cases[index].first.toString(), static_cast<unsigned>(cases[index].second))}
                        .toStdString();
                });
        }
    }

    [[nodiscard]] static auto decodeBounded(
        const el::compression::ByteDecompressor &decoder, const el::mem::ByteBlock &input) -> el::unit::ByteCount {
        try {
            return decoder.decompress(input).length();
        } catch (const el::compression::CompressionError &) {
            // Invalid format fields and integrity checks are expected to reject mutated input.
        } catch (const el::err::OutOfRangeError &) {
            // Malformed streams can legitimately exceed the bounded resource policy.
        }
        return {};
    }
};
