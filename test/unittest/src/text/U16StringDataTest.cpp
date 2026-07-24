// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/u16/impl/U16StringData.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <string_view>

TESTED_TARGETS(createU16StringData)
class U16StringDataTest final : public el::UnitTest {
public:
    void testEmptyStringCreatesNullStorage() {
        const auto storage = el::text::impl::createU16StringData(std::u16string_view{});

        REQUIRE(storage.isNull());
    }

    void testStdU16StringViewCreatesCopiedNullTerminatedStorage() {
        auto source = std::u16string{u"Gruezi"};
        const auto storage = el::text::impl::createU16StringData(std::u16string_view{source});

        source[0] = u'X';

        REQUIRE_FALSE(storage.isNull());
        REQUIRE_EQUAL(storage.constGet()->size(), 7U);
        REQUIRE_EQUAL(storage.constGet()->capacity(), 7U);
        REQUIRE_EQUAL(storage.constGet()->data()[0], u'G');
        REQUIRE_EQUAL(storage.constGet()->data()[5], u'i');
        REQUIRE_EQUAL(storage.constGet()->data()[6], u'\0');
    }

    void testRequestedSizeCreatesWritableNullTerminatedStorage() {
        const auto storage = el::text::impl::createU16StringData(std::size_t{5U});

        REQUIRE_FALSE(storage.isNull());
        REQUIRE_EQUAL(storage.constGet()->size(), 6U);
        REQUIRE_EQUAL(storage.constGet()->capacity(), 6U);
        REQUIRE_EQUAL(storage.constGet()->data()[5], u'\0');
    }

    void testRequestedSizeAndCapacityCreateWritableNullTerminatedStorage() {
        const auto storage = el::text::impl::createU16StringData(std::size_t{5U}, std::size_t{9U});

        REQUIRE_FALSE(storage.isNull());
        REQUIRE_EQUAL(storage.constGet()->size(), 6U);
        REQUIRE_EQUAL(storage.constGet()->capacity(), 10U);
        REQUIRE_EQUAL(storage.constGet()->data()[5], u'\0');
    }
};
