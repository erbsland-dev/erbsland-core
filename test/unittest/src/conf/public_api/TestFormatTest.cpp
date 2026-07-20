// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/TestFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using erbsland::conf::TestFormat;

TESTED_TARGETS(TestFormat)
class TestFormatTest final : public el::UnitTest {
public:
    TestFormat testFormat;

    void testBasics() {
        testFormat = {};
        REQUIRE_FALSE(testFormat.isSet(TestFormat::ShowContainerSize));
        REQUIRE_FALSE(testFormat.isSet(TestFormat::ShowPosition));
        REQUIRE_FALSE(testFormat.isSet(TestFormat::ShowSourceIdentifier));

        testFormat = {TestFormat::ShowContainerSize, TestFormat::ShowSourceIdentifier};
        REQUIRE(testFormat.isSet(TestFormat::ShowContainerSize) == true);
        REQUIRE(testFormat.isSet(TestFormat::ShowPosition) == false);
        REQUIRE(testFormat.isSet(TestFormat::ShowSourceIdentifier) == true);

        testFormat = TestFormat::ShowPosition;
        REQUIRE(testFormat.isSet(TestFormat::ShowContainerSize) == false);
        REQUIRE(testFormat.isSet(TestFormat::ShowPosition) == true);
        REQUIRE(testFormat.isSet(TestFormat::ShowSourceIdentifier) == false);

        auto f1 = TestFormat{TestFormat::ShowContainerSize};
        auto f2 = TestFormat{TestFormat::ShowSourceIdentifier};
        auto f3 = TestFormat{TestFormat::ShowContainerSize, TestFormat::ShowSourceIdentifier};
        auto f4 = TestFormat{TestFormat::ShowContainerSize, TestFormat::ShowSourceIdentifier};
        REQUIRE(f1 != f2);
        REQUIRE(f1 != f3);
        REQUIRE(f3 == f4);

        testFormat = f1 | f2;
        REQUIRE(testFormat == TestFormat{TestFormat::ShowContainerSize, TestFormat::ShowSourceIdentifier});
        testFormat = f1 | TestFormat::ShowSourceIdentifier;
        REQUIRE(testFormat == TestFormat{TestFormat::ShowContainerSize, TestFormat::ShowSourceIdentifier});
        testFormat = TestFormat::ShowContainerSize | f2;
        REQUIRE(testFormat == TestFormat{TestFormat::ShowContainerSize, TestFormat::ShowSourceIdentifier});
        testFormat = f1;
        testFormat |= f2;
        REQUIRE(testFormat == TestFormat{TestFormat::ShowContainerSize, TestFormat::ShowSourceIdentifier});
        testFormat = f1;
        testFormat |= TestFormat::ShowSourceIdentifier;
        REQUIRE(testFormat == TestFormat{TestFormat::ShowContainerSize, TestFormat::ShowSourceIdentifier});
    }
};
