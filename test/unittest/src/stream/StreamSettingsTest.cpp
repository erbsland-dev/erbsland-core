// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/InputStreamSettings.hpp>
#include <erbsland/OutputStreamSettings.hpp>
#include <erbsland/PathReadDataOptions.hpp>
#include <erbsland/PathReadTextOptions.hpp>
#include <erbsland/PathWriteDataOptions.hpp>
#include <erbsland/PathWriteTextOptions.hpp>
#include <erbsland/stream/impl/StreamBufferSizes.hpp>
#include <erbsland/StreamBuffering.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(
    InputStreamSettings OutputStreamSettings PathReadDataOptions PathReadTextOptions PathWriteDataOptions
        PathWriteTextOptions)
class StreamSettingsTest final : public el::UnitTest {
public:
    void testDefaults() {
        const auto input = el::stream::InputStreamSettings{};
        const auto output = el::stream::OutputStreamSettings{};

        REQUIRE_EQUAL(input.timeout(), el::time::TimeDelta::milliseconds(1000));
        REQUIRE_EQUAL(input.buffering(), el::stream::StreamBuffering::Balanced);
        REQUIRE_FALSE(input.isSensitive());
        REQUIRE_EQUAL(output.timeout(), el::time::TimeDelta::milliseconds(1000));
        REQUIRE_EQUAL(output.buffering(), el::stream::StreamBuffering::Balanced);
        REQUIRE_EQUAL(output.backBufferLimit(), el::unit::ByteCount{16U * 1024U * 1024U});
    }

    void testSettingsAreFixedInPathOptions() {
        const auto input = el::stream::InputStreamSettings{}
                               .setTimeout(el::time::TimeDelta::milliseconds(25))
                               .setBuffering(el::stream::StreamBuffering::Interactive)
                               .setSensitive(true);
        const auto output = el::stream::OutputStreamSettings{}
                                .setTimeout(el::time::TimeDelta::milliseconds(50))
                                .setBuffering(el::stream::StreamBuffering::Throughput)
                                .setBackBufferLimit(el::unit::ByteCount{4096U});
        const auto readOptions = el::path::PathReadDataOptions{}.setStreamSettings(input);
        const auto readTextOptions = el::path::PathReadTextOptions{}.setStreamSettings(input);
        const auto writeOptions = el::path::PathWriteDataOptions{}.setStreamSettings(output);
        const auto writeTextOptions = el::path::PathWriteTextOptions{}.setStreamSettings(output);

        REQUIRE_EQUAL(readOptions.streamSettings().timeout(), el::time::TimeDelta::milliseconds(25));
        REQUIRE_EQUAL(readOptions.streamSettings().buffering(), el::stream::StreamBuffering::Interactive);
        REQUIRE(readOptions.streamSettings().isSensitive());
        REQUIRE_EQUAL(readTextOptions.streamSettings().timeout(), el::time::TimeDelta::milliseconds(25));
        REQUIRE_EQUAL(readTextOptions.streamSettings().buffering(), el::stream::StreamBuffering::Interactive);
        REQUIRE(readTextOptions.streamSettings().isSensitive());
        REQUIRE_EQUAL(writeOptions.streamSettings().timeout(), el::time::TimeDelta::milliseconds(50));
        REQUIRE_EQUAL(writeOptions.streamSettings().buffering(), el::stream::StreamBuffering::Throughput);
        REQUIRE_EQUAL(writeOptions.streamSettings().backBufferLimit(), el::unit::ByteCount{4096U});
        REQUIRE_EQUAL(writeTextOptions.streamSettings().timeout(), el::time::TimeDelta::milliseconds(50));
        REQUIRE_EQUAL(writeTextOptions.streamSettings().buffering(), el::stream::StreamBuffering::Throughput);
        REQUIRE_EQUAL(writeTextOptions.streamSettings().backBufferLimit(), el::unit::ByteCount{4096U});
    }

    void testBufferingPresetsAndBackLimitOverride() {
        using enum el::stream::StreamBuffering;
        constexpr auto kib = std::size_t{1024U};
        constexpr auto mib = std::size_t{1024U * 1024U};
        struct Case final {
            el::stream::StreamBuffering buffering;
            el::unit::ByteCount ioRing;
            el::unit::ByteCount aggregateChunk;
            el::unit::ByteCount decoder;
            el::unit::ByteCount outputRetainedInitial;
            el::unit::ByteCount outputBackLimit;
        };
        const auto cases = std::array{
            Case{
                MinimalMemory,
                el::unit::ByteCount{4U * kib},
                el::unit::ByteCount{16U * kib},
                el::unit::ByteCount{4U * kib},
                el::unit::ByteCount{4U * kib},
                el::unit::ByteCount{256U * kib}},
            Case{
                Interactive,
                el::unit::ByteCount{16U * kib},
                el::unit::ByteCount{32U * kib},
                el::unit::ByteCount{16U * kib},
                el::unit::ByteCount{4U * kib},
                el::unit::ByteCount{1U * mib}},
            Case{
                Balanced,
                el::unit::ByteCount{64U * kib},
                el::unit::ByteCount{64U * kib},
                el::unit::ByteCount{64U * kib},
                el::unit::ByteCount{16U * kib},
                el::unit::ByteCount{16U * mib}},
            Case{
                Throughput,
                el::unit::ByteCount{256U * kib},
                el::unit::ByteCount{256U * kib},
                el::unit::ByteCount{128U * kib},
                el::unit::ByteCount{64U * kib},
                el::unit::ByteCount{64U * mib}},
            Case{
                Bulk,
                el::unit::ByteCount{1U * mib},
                el::unit::ByteCount{1U * mib},
                el::unit::ByteCount{256U * kib},
                el::unit::ByteCount{256U * kib},
                el::unit::ByteCount{256U * mib}},
        };
        for (const auto &testCase : cases) {
            const auto actual = el::stream::impl::streamBufferSizes(testCase.buffering);
            REQUIRE_EQUAL(actual.ioRing, testCase.ioRing);
            REQUIRE_EQUAL(actual.aggregateChunk, testCase.aggregateChunk);
            REQUIRE_EQUAL(actual.decoder, testCase.decoder);
            REQUIRE_EQUAL(actual.outputRetainedInitial, testCase.outputRetainedInitial);
            REQUIRE_EQUAL(actual.outputBackLimit, testCase.outputBackLimit);
        }

        auto settings = el::stream::OutputStreamSettings{}.setBuffering(Interactive);
        REQUIRE_EQUAL(settings.backBufferLimit(), el::unit::ByteCount{1U * mib});
        settings.setBackBufferLimit(el::unit::ByteCount{7U * kib}).setBuffering(Bulk);
        REQUIRE_EQUAL(settings.backBufferLimit(), el::unit::ByteCount{7U * kib});
        REQUIRE_EQUAL(settings.clearBackBufferLimit().backBufferLimit(), el::unit::ByteCount{256U * mib});
    }

    void testPathTimeoutDefaultsAndWrappers() {
        auto readData = el::path::PathReadDataOptions{};
        auto readText = el::path::PathReadTextOptions{};
        auto writeData = el::path::PathWriteDataOptions{};
        auto writeText = el::path::PathWriteTextOptions{};

        REQUIRE_EQUAL(readData.timeout(), el::time::TimeDelta::seconds(60));
        REQUIRE_EQUAL(readText.timeout(), el::time::TimeDelta::seconds(60));
        REQUIRE_EQUAL(writeData.timeout(), el::time::TimeDelta::seconds(60));
        REQUIRE_EQUAL(writeText.timeout(), el::time::TimeDelta::seconds(60));

        const auto custom = el::time::TimeDelta::seconds(12);
        REQUIRE_EQUAL(readData.setTimeout(custom).timeout(), custom);
        REQUIRE_EQUAL(readText.setTimeout(custom).timeout(), custom);
        REQUIRE_EQUAL(writeData.setTimeout(custom).timeout(), custom);
        REQUIRE_EQUAL(writeText.setTimeout(custom).timeout(), custom);
    }
};
