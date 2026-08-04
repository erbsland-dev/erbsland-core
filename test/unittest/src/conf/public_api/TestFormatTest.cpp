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
        REQUIRE_EQUAL(testFormat.isSet(TestFormat::ShowContainerSize), true);
        REQUIRE_EQUAL(testFormat.isSet(TestFormat::ShowPosition), false);
        REQUIRE_EQUAL(testFormat.isSet(TestFormat::ShowSourceIdentifier), true);

        testFormat = TestFormat::ShowPosition;
        REQUIRE_EQUAL(testFormat.isSet(TestFormat::ShowContainerSize), false);
        REQUIRE_EQUAL(testFormat.isSet(TestFormat::ShowPosition), true);
        REQUIRE_EQUAL(testFormat.isSet(TestFormat::ShowSourceIdentifier), false);

        auto f1 = TestFormat{TestFormat::ShowContainerSize};
        auto f2 = TestFormat{TestFormat::ShowSourceIdentifier};
        auto f3 = TestFormat{TestFormat::ShowContainerSize, TestFormat::ShowSourceIdentifier};
        auto f4 = TestFormat{TestFormat::ShowContainerSize, TestFormat::ShowSourceIdentifier};
        REQUIRE_NOT_EQUAL(f1, f2);
        REQUIRE_NOT_EQUAL(f1, f3);
        REQUIRE_EQUAL(f3, f4);

        testFormat = f1 | f2;
        REQUIRE_EQUAL(testFormat, f3);
        testFormat = f1 | TestFormat::ShowSourceIdentifier;
        REQUIRE_EQUAL(testFormat, f3);
        testFormat = TestFormat::ShowContainerSize | f2;
        REQUIRE_EQUAL(testFormat, f3);
        testFormat = f1;
        testFormat |= f2;
        REQUIRE_EQUAL(testFormat, f3);
        testFormat = f1;
        testFormat |= TestFormat::ShowSourceIdentifier;
        REQUIRE_EQUAL(testFormat, f3);
    }
};
