// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/StdFormatForText.hpp>
#include <erbsland/text/u8/impl/U8StringData.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <string_view>

TESTED_TARGETS(createU8StringData)
class U8StringDataTest final : public el::UnitTest {
public:
    void testEmptyStringCreatesNullStorage() {
        const auto storage = el::text::impl::createU8StringData(std::string_view{});

        REQUIRE(storage.isNull());
    }

    void testStdStringViewCreatesCopiedNullTerminatedStorage() {
        auto source = std::string{"Hello"};
        const auto storage = el::text::impl::createU8StringData(std::string_view{source});

        source[0] = 'J';

        REQUIRE_FALSE(storage.isNull());
        REQUIRE_EQUAL(storage.constGet()->size(), 6U);
        REQUIRE_EQUAL(storage.constGet()->capacity(), 6U);
        REQUIRE_EQUAL(storage.constGet()->data()[0], 'H');
        REQUIRE_EQUAL(storage.constGet()->data()[4], 'o');
        REQUIRE_EQUAL(storage.constGet()->data()[5], '\0');
    }

    void testStdU8StringViewCreatesCopiedNullTerminatedStorage() {
        auto source = std::u8string{u8"Gruezi"};
        const auto storage = el::text::impl::createU8StringData(std::u8string_view{source});

        source[0] = u'X';

        REQUIRE_FALSE(storage.isNull());
        REQUIRE_EQUAL(storage.constGet()->size(), 7U);
        REQUIRE_EQUAL(storage.constGet()->data()[0], 'G');
        REQUIRE_EQUAL(storage.constGet()->data()[5], 'i');
        REQUIRE_EQUAL(storage.constGet()->data()[6], '\0');
    }

    void testRequestedSizeCreatesWritableNullTerminatedStorage() {
        const auto storage = el::text::impl::createU8StringData(std::size_t{5U});

        REQUIRE_FALSE(storage.isNull());
        REQUIRE_EQUAL(storage.constGet()->size(), 6U);
        REQUIRE_EQUAL(storage.constGet()->capacity(), 6U);
        REQUIRE_EQUAL(storage.constGet()->data()[5], '\0');
    }

    void testRequestedSizeAndCapacityCreateWritableNullTerminatedStorage() {
        const auto storage = el::text::impl::createU8StringData(std::size_t{5U}, std::size_t{9U});

        REQUIRE_FALSE(storage.isNull());
        REQUIRE_EQUAL(storage.constGet()->size(), 6U);
        REQUIRE_EQUAL(storage.constGet()->capacity(), 10U);
        REQUIRE_EQUAL(storage.constGet()->data()[5], '\0');
    }

    void testReservedEmptyStorageUsesRequestedCapacity() {
        const auto storage = el::text::impl::createU8StringData(std::size_t{0U}, std::size_t{7U});

        REQUIRE_FALSE(storage.isNull());
        REQUIRE_EQUAL(storage.constGet()->size(), 1U);
        REQUIRE_EQUAL(storage.constGet()->capacity(), 8U);
        REQUIRE_EQUAL(storage.constGet()->data()[0], '\0');
    }
};
