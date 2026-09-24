// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/StdFormat.hpp>
#include <erbsland/text/u32/impl/U32StringData.hpp>
#include <erbsland/text/u32/impl/U32StringTraits.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>
#include <string_view>

TESTED_TARGETS(createU32StringData U32StringData U32StringDataPtr U32StringTraits)
class U32StringDataTest final : public el::UnitTest {
public:
    void testTraits() { REQUIRE_EQUAL(el::text::impl::U32StringTraits::kind, el::text::StringKind::U32); }

    void testEmptyStringCreatesNullStorage() {
        const auto storage = el::text::impl::createU32StringData(std::u32string_view{});

        REQUIRE(storage.isNull());
    }

    void testStdU32StringViewCreatesCopiedNullTerminatedStorage() {
        auto source = std::u32string{U"Gruezi"};
        const auto storage = el::text::impl::createU32StringData(std::u32string_view{source});

        source[0] = U'X';

        REQUIRE_FALSE(storage.isNull());
        REQUIRE_EQUAL(storage.constGet()->size(), 7U);
        REQUIRE_EQUAL(storage.constGet()->capacity(), 7U);
        REQUIRE_EQUAL(storage.constGet()->data()[0], U'G');
        REQUIRE_EQUAL(storage.constGet()->data()[5], U'i');
        REQUIRE_EQUAL(storage.constGet()->data()[6], U'\0');
    }

    void testRequestedSizeCreatesWritableNullTerminatedStorage() {
        const auto storage = el::text::impl::createU32StringData(std::size_t{5U});

        REQUIRE_FALSE(storage.isNull());
        REQUIRE_EQUAL(storage.constGet()->size(), 6U);
        REQUIRE_EQUAL(storage.constGet()->capacity(), 6U);
        REQUIRE_EQUAL(storage.constGet()->data()[5], U'\0');
    }

    void testRequestedSizeAndCapacityCreateWritableNullTerminatedStorage() {
        const auto storage = el::text::impl::createU32StringData(std::size_t{5U}, std::size_t{9U});

        REQUIRE_FALSE(storage.isNull());
        REQUIRE_EQUAL(storage.constGet()->size(), 6U);
        REQUIRE_EQUAL(storage.constGet()->capacity(), 10U);
        REQUIRE_EQUAL(storage.constGet()->data()[5], U'\0');
    }
};
