// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/path/PathReadDataOptions.hpp>
#include <erbsland/path/PathReadTextOptions.hpp>
#include <erbsland/path/PathWriteDataOptions.hpp>
#include <erbsland/path/PathWriteTextOptions.hpp>
#include <erbsland/stream/InputStreamSettings.hpp>
#include <erbsland/stream/OutputStreamSettings.hpp>
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
        REQUIRE_EQUAL(input.bufferCapacity(), el::unit::ByteLength{64U * 1024U});
        REQUIRE_EQUAL(output.timeout(), el::time::TimeDelta::milliseconds(1000));
        REQUIRE_EQUAL(output.bufferCapacity(), el::unit::ByteLength{64U * 1024U});
        REQUIRE_EQUAL(output.backBufferLimit(), el::unit::ByteLength{10'000'000U});
    }

    void testSettingsAreFixedInPathOptions() {
        const auto input = el::stream::InputStreamSettings{}
                               .setTimeout(el::time::TimeDelta::milliseconds(25))
                               .setBufferCapacity(el::unit::ByteLength{1024U});
        const auto output = el::stream::OutputStreamSettings{}
                                .setTimeout(el::time::TimeDelta::milliseconds(50))
                                .setBufferCapacity(el::unit::ByteLength{2048U})
                                .setBackBufferLimit(el::unit::ByteLength{4096U});
        const auto readOptions = el::path::PathReadDataOptions{}.setStreamSettings(input);
        const auto readTextOptions = el::path::PathReadTextOptions{}.setStreamSettings(input);
        const auto writeOptions = el::path::PathWriteDataOptions{}.setStreamSettings(output);
        const auto writeTextOptions = el::path::PathWriteTextOptions{}.setStreamSettings(output);

        REQUIRE_EQUAL(readOptions.streamSettings().timeout(), el::time::TimeDelta::milliseconds(25));
        REQUIRE_EQUAL(readOptions.streamSettings().bufferCapacity(), el::unit::ByteLength{1024U});
        REQUIRE_EQUAL(readTextOptions.streamSettings().timeout(), el::time::TimeDelta::milliseconds(25));
        REQUIRE_EQUAL(readTextOptions.streamSettings().bufferCapacity(), el::unit::ByteLength{1024U});
        REQUIRE_EQUAL(writeOptions.streamSettings().timeout(), el::time::TimeDelta::milliseconds(50));
        REQUIRE_EQUAL(writeOptions.streamSettings().bufferCapacity(), el::unit::ByteLength{2048U});
        REQUIRE_EQUAL(writeOptions.streamSettings().backBufferLimit(), el::unit::ByteLength{4096U});
        REQUIRE_EQUAL(writeTextOptions.streamSettings().timeout(), el::time::TimeDelta::milliseconds(50));
        REQUIRE_EQUAL(writeTextOptions.streamSettings().bufferCapacity(), el::unit::ByteLength{2048U});
        REQUIRE_EQUAL(writeTextOptions.streamSettings().backBufferLimit(), el::unit::ByteLength{4096U});
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
