// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteBlockInputStream.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/StreamError.hpp>
#include <erbsland/StreamPositionOrigin.hpp>
#include <erbsland/StreamReadStatus.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using el::mem::ByteBlock;
using el::stream::ByteBlockInputStream;
using el::stream::StreamPositionOrigin;
using el::stream::StreamReadStatus;
using el::unit::ByteCount;
using el::unit::ByteIndex;
using el::unit::ByteOffset;

TESTED_TARGETS(ByteBlockInputStream)
class ByteBlockInputStreamTest final : public el::UnitTest {
public:
    void testEmptyAndBoundedReads() {
        auto empty = ByteBlockInputStream{ByteBlock{}};
        REQUIRE(empty.isReady());
        REQUIRE(empty.supportsPositioning());
        REQUIRE_EQUAL(empty.read(ByteCount{4U}).status(), StreamReadStatus::Finished);

        auto stream = ByteBlockInputStream{ByteBlock({'a', 'b', 'c', 'd', 'e'})};
        auto first = stream.read(ByteCount{2U});
        REQUIRE_EQUAL(first.status(), StreamReadStatus::Data);
        REQUIRE_EQUAL(first.data(), ByteBlock({'a', 'b'}));
        REQUIRE_EQUAL(stream.position(), ByteIndex{2U});
        auto second = stream.read(ByteCount{10U});
        REQUIRE_EQUAL(second.data(), ByteBlock({'c', 'd', 'e'}));
        REQUIRE_EQUAL(stream.read(ByteCount{1U}).status(), StreamReadStatus::Finished);
    }

    void testPositioningAndLimits() {
        auto stream = ByteBlockInputStream{ByteBlock({'a', 'b', 'c', 'd'})};
        REQUIRE(stream.setPosition(ByteIndex{1U}).isSuccess());
        REQUIRE_EQUAL(stream.read(ByteCount{2U}).data(), ByteBlock({'b', 'c'}));
        REQUIRE(stream.movePosition(StreamPositionOrigin::End, ByteOffset{-1}).isSuccess());
        REQUIRE_EQUAL(stream.read(ByteCount{1U}).data(), ByteBlock({'d'}));
        REQUIRE(stream.setPosition(ByteIndex{20U}).isSuccess());
        REQUIRE_EQUAL(stream.position(), ByteIndex{20U});
        REQUIRE_EQUAL(stream.read(ByteCount{1U}).status(), StreamReadStatus::Finished);
        REQUIRE_THROWS_AS(el::err::ParameterFailure, stream.movePosition(StreamPositionOrigin::Start, ByteOffset{-1}));
    }

    void testSensitivityAndLifecycle() {
        auto data = ByteBlock({'s', 'e', 'c', 'r', 'e', 't'});
        data.markAsSensitive();
        auto stream = ByteBlockInputStream{data};
        REQUIRE(stream.inputSettings().isSensitive());
        REQUIRE(stream.isOpen());
        REQUIRE(stream.close().isClosed());
        REQUIRE_FALSE(stream.isOpen());
        REQUIRE_THROWS_AS(el::stream::StreamError, stream.read(ByteCount{1U}));

        auto aborted = ByteBlockInputStream{ByteBlock({'x'})};
        aborted.abort();
        REQUIRE_FALSE(aborted.isOpen());
        REQUIRE_THROWS_AS(el::stream::StreamError, aborted.position());
    }
};
