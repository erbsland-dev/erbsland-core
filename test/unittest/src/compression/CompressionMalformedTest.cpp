// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/compression/ByteCompressor.hpp>
#include <erbsland/compression/ByteDecompressor.hpp>
#include <erbsland/compression/CompressionError.hpp>
#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

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
        for (const auto algorithm : CompressionAlgorithm::all()) {
            for (const auto format : {CompressionFormat::Raw, CompressionFormat::Zip, CompressionFormat::Core}) {
                if ((!full && format != CompressionFormat::Raw) ||
                    (algorithm == CompressionAlgorithm::Lz4Block && format == CompressionFormat::Zip)) {
                    continue;
                }
                const auto encoded = ByteCompressor{algorithm, format}.compress(source);
                const auto options = DecompressionOptions{}
                                         .setExpectedOutputLength(source.length())
                                         .setMaximumOutputLength(el::unit::ByteLength{32768U})
                                         .setMaximumWorkspaceLength(el::unit::ByteLength{64U * 1024U * 1024U});
                const auto decoder = ByteDecompressor{algorithm, format, options};
                const auto step = full ? std::size_t{1U} : encoded.span().size();
                for (auto index = std::size_t{}; index < encoded.span().size(); index += step) {
                    const auto position = el::unit::ByteIndex::fromSizeT(index);
                    WITH_CONTEXT(decodeBounded(decoder, encoded.slice({}, el::unit::ByteLength::fromSizeT(index))));
                    auto corrupted = el::mem::ByteBlockEditor{encoded};
                    const auto mask = el::mem::Byte::fromCroppedUInt32(1U << (index % 8U));
                    corrupted.setOrThrow(position, encoded.getOrThrow(position) ^ mask);
                    WITH_CONTEXT(decodeBounded(decoder, corrupted));
                }
            }
        }
    }

    void decodeBounded(const el::compression::ByteDecompressor &decoder, const el::mem::ByteBlock &input) {
        try {
            const auto output = decoder.decompress(input);
            REQUIRE_LESS_EQUAL(output.length(), el::unit::ByteLength{32768U});
        } catch (const el::compression::CompressionError &) {
            // Invalid format fields and integrity checks are expected to reject mutated input.
        } catch (const el::err::OutOfRangeError &) {
            // Malformed streams can legitimately exceed the bounded resource policy.
        }
    }
};
