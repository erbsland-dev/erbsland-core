// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/compression/ByteCompressor.hpp>
#include <erbsland/compression/ByteDecompressor.hpp>
#include <erbsland/compression/CompressionAlgorithm.hpp>
#include <erbsland/compression/CompressionError.hpp>
#include <erbsland/compression/CompressionFormat.hpp>
#include <erbsland/compression/CompressionLevel.hpp>
#include <erbsland/compression/DecompressionOptions.hpp>
#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/path/TempDirectory.hpp>
#include <erbsland/system/Subprocess.hpp>
#include <erbsland/system/SubprocessOptions.hpp>
#include <erbsland/system/SubprocessOutputMode.hpp>
#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/IntegerBase.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/text/u8/U8StringConstIterator.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(ByteCompressor ByteDecompressor CompressionAlgorithm CompressionLevel)
class CompressionInteropTest final : public el::UnitTest {
private:
    struct LevelCase final {
        el::compression::CompressionLevel level;
        el::text::String name;
    };

    struct DataCase final {
        el::text::String name;
        el::mem::ByteBlock data;
    };

    struct ConformanceVector final {
        el::compression::CompressionAlgorithm algorithm;
        el::text::String encoder;
        el::text::String version;
        el::text::String variant;
        el::text::String options;
        el::text::String caseName;
        el::mem::ByteBlock source;
        el::mem::ByteBlock compressed;
    };

private:
    static constexpr auto cDifferentialSeed = uint32_t{0x6d2b79f5U};

private:
    [[nodiscard]] static auto levels() -> std::array<LevelCase, 5U> {
        using Level = el::compression::CompressionLevel;
        return {{{Level::Fastest, "fastest"_el},
            {Level::Fast, "fast"_el},
            {Level::Default, "default"_el},
            {Level::High, "high"_el},
            {Level::Highest, "highest"_el}}};
    }

    [[nodiscard]] static auto nextRandom(uint32_t &state) noexcept -> uint32_t {
        state ^= state << 13U;
        state ^= state >> 17U;
        state ^= state << 5U;
        return state;
    }

    [[nodiscard]] static auto createAllBytes() -> el::mem::ByteBlock {
        auto bytes = std::vector<uint8_t>{};
        bytes.reserve(256U);
        for (auto value = uint16_t{}; value <= 0xffU; ++value) {
            bytes.push_back(static_cast<uint8_t>(value));
        }
        return el::mem::ByteBlock::fromVector(bytes);
    }

    [[nodiscard]] static auto createData(const std::size_t length, uint32_t seed) -> el::mem::ByteBlock {
        auto bytes = std::vector<uint8_t>{};
        bytes.reserve(length);
        for (auto index = std::size_t{}; index < length; ++index) {
            const auto random = nextRandom(seed);
            uint8_t value;
            if (index % 4096U < 512U) {
                value = static_cast<uint8_t>((index + seed) % 23U);
            } else if (index % 8192U < 6144U) {
                value = static_cast<uint8_t>(random & 0xffU);
            } else {
                value = static_cast<uint8_t>((index / 17U) & 7U);
            }
            bytes.push_back(value);
        }
        return el::mem::ByteBlock::fromVector(bytes);
    }

    static void appendBoundaryCases(std::vector<DataCase> &result) {
        static constexpr auto cBoundarySizes = std::array<std::size_t, 48U>{
            0U,
            1U,
            2U,
            3U,
            4U,
            5U,
            6U,
            7U,
            8U,
            9U,
            10U,
            11U,
            12U,
            13U,
            14U,
            15U,
            16U,
            17U,
            18U,
            19U,
            20U,
            21U,
            22U,
            23U,
            24U,
            25U,
            26U,
            27U,
            28U,
            29U,
            30U,
            31U,
            32U,
            33U,
            255U,
            256U,
            257U,
            4095U,
            4096U,
            4097U,
            32U * 1024U - 1U,
            32U * 1024U,
            32U * 1024U + 1U,
            64U * 1024U - 1U,
            64U * 1024U,
            64U * 1024U + 1U,
            128U * 1024U - 1U,
            128U * 1024U};
        for (const auto size : cBoundarySizes) {
            result.push_back(
                {el::text::StringFormat{"boundary-{}"_el}.build(size), createData(size, cDifferentialSeed ^ size)});
        }
        result.push_back(
            {"boundary-131073"_el, createData(128U * 1024U + 1U, cDifferentialSeed ^ (128U * 1024U + 1U))});
    }

    static void appendAlgorithmBoundaries(
        std::vector<DataCase> &result, const el::compression::CompressionAlgorithm algorithm) {
        auto sizes = std::vector<std::size_t>{};
        if (algorithm == el::compression::CompressionAlgorithm::Bzip2) {
            sizes = {100'000U - 1U, 100'000U, 100'000U + 1U, 900'000U - 1U, 900'000U, 900'000U + 1U};
        } else if (algorithm == el::compression::CompressionAlgorithm::Lzma) {
            sizes = {256U * 1024U - 1U,
                256U * 1024U,
                256U * 1024U + 1U,
                2U * 1024U * 1024U - 1U,
                2U * 1024U * 1024U,
                2U * 1024U * 1024U + 1U};
        }
        for (const auto size : sizes) {
            result.push_back(
                {el::text::StringFormat{"algorithm-boundary-{}"_el}.build(size),
                    createData(size, cDifferentialSeed ^ static_cast<uint32_t>(size))});
        }
    }

    static void appendDifferentialCases(std::vector<DataCase> &result) {
        auto state = cDifferentialSeed;
        for (auto index = std::size_t{}; index < 24U; ++index) {
            const auto seed = nextRandom(state);
            const auto size = static_cast<std::size_t>(nextRandom(state) % (192U * 1024U + 1U));
            result.push_back(
                {el::text::StringFormat{"differential-{:02}-seed-{:08X}-size-{}"_el}.build(index, seed, size),
                    createData(size, seed)});
        }
    }

    [[nodiscard]] static auto dataCases(const el::compression::CompressionAlgorithm algorithm)
        -> std::vector<DataCase> {
        auto result = std::vector<DataCase>{};
        result.push_back({"all-byte-values"_el, createAllBytes()});
        result.push_back(
            {"large-repetitive"_el,
                el::mem::ByteBlock{el::unit::ByteLength{384U * 1024U}, el::mem::Byte{0x41U}}});
        appendBoundaryCases(result);
        appendAlgorithmBoundaries(result, algorithm);
        appendDifferentialCases(result);
        return result;
    }

    [[nodiscard]] static auto referenceVariants(const el::compression::CompressionAlgorithm algorithm)
        -> std::vector<el::text::String> {
        using Algorithm = el::compression::CompressionAlgorithm;
        if (algorithm == Algorithm::Lz4Block) {
            return {"default"_el, "fast"_el, "high"_el};
        }
        if (algorithm == Algorithm::Deflate) {
            return {"stored"_el, "fixed"_el, "dynamic"_el};
        }
        if (algorithm == Algorithm::Bzip2) {
            return {"level-1"_el, "level-5"_el, "level-9"_el};
        }
        if (algorithm == Algorithm::Lzma) {
            return {"known-size"_el,
                "liblzma-eos-default"_el,
                "liblzma-eos-small-dictionary"_el,
                "liblzma-eos-alt-properties"_el};
        }
        return {"level-1-no-size"_el, "level-3-size"_el, "level-15-checksum"_el};
    }

    [[nodiscard]] static auto counterpartPath() -> el::path::Path {
        auto result = el::path::Path{el::unittest::fh::unitTestExecutablePath()}.parent();
#if defined(_WIN32)
        result /= "erbsland-core-compression-interop-counterpart.exe"_el;
#else
        result /= "erbsland-core-compression-interop-counterpart"_el;
#endif
        return result;
    }

    [[nodiscard]] static auto nativePathText(const el::path::Path &path) -> el::text::String {
        return el::text::StringConverter{path.toStdPath().string()}.toString();
    }

    [[nodiscard]] static auto casePath(
        const el::path::Path &directory, const std::size_t index, const el::text::String &kind) -> el::path::Path {
        return directory / el::text::StringFormat{"case-{:03}-{}.bin"_el}.build(index, kind);
    }

    [[nodiscard]] static auto diagnosticText(
        const el::compression::CompressionAlgorithm algorithm,
        const DataCase &dataCase,
        const el::text::String &variant = {}) -> el::text::String {
        auto result = el::text::StringFormat{"algorithm: {}\ndata case: {}\nsize: {}"_el}.build(
            algorithm.toString(), dataCase.name, dataCase.data.length());
        if (!variant.isEmpty()) {
            result = el::text::StringFormat{"{}\nreference variant: {}"_el}.build(result, variant);
        }
        return result;
    }

    [[nodiscard]] static auto parseHex(const el::text::String &text) -> el::mem::ByteBlock {
        if (text.length().toSizeT() % 2U != 0U) {
            throw el::err::RuntimeError{"Conformance vector has an odd number of hexadecimal digits."_el};
        }
        auto bytes = std::vector<uint8_t>{};
        bytes.reserve(text.length().toSizeT() / 2U);
        auto highNibble = uint8_t{};
        auto highNibblePending = false;
        for (const auto character : text) {
            const auto digit = character.digitValue(el::text::IntegerBase::Hexadecimal);
            if (!digit.has_value()) {
                throw el::err::RuntimeError{"Conformance vector contains a non-hexadecimal digit."_el};
            }
            if (!highNibblePending) {
                highNibble = static_cast<uint8_t>(*digit << 4U);
                highNibblePending = true;
            } else {
                bytes.push_back(static_cast<uint8_t>(highNibble | *digit));
                highNibblePending = false;
            }
        }
        return el::mem::ByteBlock::fromVector(bytes);
    }

    [[nodiscard]] static auto conformanceVectors() -> std::vector<ConformanceVector> {
        const auto path = el::path::Path{el::unittest::fh::unitTestExecutablePath()}.parent() /
            "compression-conformance-vectors.tsv"_el;
        const auto content = path.content().readTextOrThrow();
        const auto lines = el::text::StringList::fromSplit(
            content, el::text::CharSet{U'\n'}, el::unit::ItemCount::infinite(), true);
        auto result = std::vector<ConformanceVector>{};
        for (const auto &sourceLine : lines) {
            const auto line = sourceLine.trimmed();
            if (line.isEmpty() || line.startsWith("#"_el)) {
                continue;
            }
            const auto fields = el::text::StringList::fromSplit(
                line, el::text::CharSet{U'\t'}, el::unit::ItemCount::infinite(), true);
            if (fields.count() != el::unit::ItemCount{8U}) {
                throw el::err::RuntimeError{"Conformance vector does not have eight tab-separated fields."_el};
            }
            result.push_back(
                {.algorithm = el::compression::CompressionAlgorithm::fromStringOrThrow(fields.get(el::unit::ItemIndex{0U})),
                    .encoder = fields.get(el::unit::ItemIndex{1U}),
                    .version = fields.get(el::unit::ItemIndex{2U}),
                    .variant = fields.get(el::unit::ItemIndex{3U}),
                    .options = fields.get(el::unit::ItemIndex{4U}),
                    .caseName = fields.get(el::unit::ItemIndex{5U}),
                    .source = parseHex(fields.get(el::unit::ItemIndex{6U})),
                    .compressed = parseHex(fields.get(el::unit::ItemIndex{7U}))});
        }
        return result;
    }

    static void writeCoreCases(
        const el::path::Path &directory,
        const el::compression::CompressionAlgorithm algorithm,
        const std::vector<DataCase> &cases) {
        const auto levelCases = levels();
        for (auto caseIndex = std::size_t{}; caseIndex < cases.size(); ++caseIndex) {
            casePath(directory, caseIndex, "source"_el).content().writeDataOrThrow(cases[caseIndex].data);
            for (const auto &levelCase : levelCases) {
                const auto compressed =
                    el::compression::ByteCompressor{
                        algorithm, el::compression::CompressionFormat::Raw, levelCase.level}
                        .compress(cases[caseIndex].data);
                casePath(directory, caseIndex, el::text::StringFormat{"core-{}"_el}.build(levelCase.name))
                    .content()
                    .writeDataOrThrow(compressed);
            }
        }
    }

    void runCounterpart(
        const el::path::Path &directory,
        const el::compression::CompressionAlgorithm algorithm,
        const std::vector<DataCase> &cases) {
        auto options = el::system::SubprocessOptions{};
        options.setInheritStandardInput(false)
            .setStandardOutputMode(el::system::SubprocessOutputMode::Discard)
            .setStandardErrorMode(el::system::SubprocessOutputMode::Capture);
        auto process = el::system::Subprocess::start(
            counterpartPath(),
            el::text::StringList{"verify-batch"_el,
                algorithm.toString(),
                nativePathText(directory),
                el::text::String::fromInteger(cases.size())},
            options);
        const auto status = process.wait();
        const auto counterpartError = el::text::StringConverter{process.standardError()}.toString();
        if (!counterpartError.isEmpty()) {
            _counterpartError = el::text::StringFormat{"{}\nRust counterpart error:\n{}"_el}.build(
                _counterpartError, counterpartError);
        }
        REQUIRE(status.isSuccess());
    }

    void verifyReferenceCases(
        const el::path::Path &directory,
        const el::compression::CompressionAlgorithm algorithm,
        const std::vector<DataCase> &cases) {
        const auto variants = referenceVariants(algorithm);
        for (auto caseIndex = std::size_t{}; caseIndex < cases.size(); ++caseIndex) {
            for (const auto &variant : variants) {
                runWithContext(
                    SOURCE_LOCATION(),
                    [&]() -> void {
                        _counterpartError = diagnosticText(algorithm, cases[caseIndex], variant);
                        const auto reference =
                            casePath(
                                directory,
                                caseIndex,
                                el::text::StringFormat{"reference-{}"_el}.build(variant))
                                .content()
                                .readDataOrThrow();
                        const auto options = el::compression::DecompressionOptions{}.setExpectedOutputLength(
                            cases[caseIndex].data.length());
                        const auto decompressed =
                            el::compression::ByteDecompressor{
                                algorithm, el::compression::CompressionFormat::Raw, options}
                                .decompress(reference);
                        REQUIRE_EQUAL(decompressed, cases[caseIndex].data);
                        if (algorithm == el::compression::CompressionAlgorithm::Lzma) {
                            const auto withoutConfiguredLength =
                                el::compression::ByteDecompressor{
                                    algorithm, el::compression::CompressionFormat::Raw}
                                    .decompress(reference);
                            REQUIRE_EQUAL(withoutConfiguredLength, cases[caseIndex].data);
                        }
                    },
                    [&]() -> std::string {
                        return el::text::StringConverter{diagnosticText(algorithm, cases[caseIndex], variant)}
                            .toStdString();
                    });
            }
        }
    }

    void verifyAlgorithm(const el::compression::CompressionAlgorithm algorithm) {
        const auto cases = dataCases(algorithm);
        const auto directory = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        _counterpartError = el::text::StringFormat{"algorithm: {}"_el}.build(algorithm.toString());
        writeCoreCases(directory->path(), algorithm, cases);
        runCounterpart(directory->path(), algorithm, cases);
        verifyReferenceCases(directory->path(), algorithm, cases);
        _counterpartError = {};
    }

public: // implement UnitTest
    auto additionalErrorMessages() -> std::string override {
        if (_counterpartError.isEmpty()) {
            return {};
        }
        return el::text::StringConverter{
            el::text::StringFormat{"Interop context:\n{}"_el}.build(_counterpartError)}
            .toStdString();
    }

public:
    void testLz4Block() { verifyAlgorithm(el::compression::CompressionAlgorithm::Lz4Block); }
    void testDeflate() { verifyAlgorithm(el::compression::CompressionAlgorithm::Deflate); }
    void testBzip2() { verifyAlgorithm(el::compression::CompressionAlgorithm::Bzip2); }
    void testLzma() { verifyAlgorithm(el::compression::CompressionAlgorithm::Lzma); }
    void testZstandard() { verifyAlgorithm(el::compression::CompressionAlgorithm::Zstandard); }

    void testCommittedConformanceVectors() {
        const auto vectors = conformanceVectors();
        REQUIRE_EQUAL(vectors.size(), 32U);
        for (const auto &vector : vectors) {
            _counterpartError = el::text::StringFormat{
                "algorithm: {}\nencoder: {} {}\nvariant: {}\noptions: {}\ndata case: {}"_el}
                                    .build(
                                        vector.algorithm.toString(),
                                        vector.encoder,
                                        vector.version,
                                        vector.variant,
                                        vector.options,
                                        vector.caseName);
            const auto decompressed =
                el::compression::ByteDecompressor{
                    vector.algorithm,
                    el::compression::CompressionFormat::Raw,
                    el::compression::DecompressionOptions{}.setExpectedOutputLength(vector.source.length())}
                    .decompress(vector.compressed);
            REQUIRE_EQUAL(decompressed, vector.source);
            if (vector.algorithm == el::compression::CompressionAlgorithm::Lzma) {
                const auto withoutConfiguredLength =
                    el::compression::ByteDecompressor{
                        vector.algorithm, el::compression::CompressionFormat::Raw}
                        .decompress(vector.compressed);
                REQUIRE_EQUAL(withoutConfiguredLength, vector.source);
            }
        }
        _counterpartError = {};
    }

    void testUnsupportedZstandardDictionary() {
        const auto stream = el::mem::ByteBlock({0x28U, 0xb5U, 0x2fU, 0xfdU, 0x21U, 0x01U});
        try {
            const auto decompressor = el::compression::ByteDecompressor{
                el::compression::CompressionAlgorithm::Zstandard, el::compression::CompressionFormat::Raw};
            [[maybe_unused]] const auto unexpected = decompressor.decompress(stream);
            REQUIRE(false);
        } catch (const el::compression::CompressionError &error) {
            REQUIRE(error.reasonCode() == el::compression::CompressionErrorReason::UnsupportedFeature);
        }
    }

private:
    el::text::String _counterpartError;
};
