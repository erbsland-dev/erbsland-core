// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/impl/UnsafeU16StringAccess.hpp>
#include <erbsland/text/u16/U16String.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string_view>

using el::text::U16String;

TESTED_TARGETS(UnsafeU16StringAccess)
class UnsafeU16StringAccessTest final : public el::UnitTest {
public:
    void testEmptyStringReturnsNullPointer() {
        const auto text = U16String{};

        REQUIRE_EQUAL(el::text::impl::UnsafeU16StringAccess{text}.data(), nullptr);
    }

    void testStringReturnsNullTerminatedData() {
        const auto text = U16String{std::u16string_view{u"Hello"}};
        const auto *data = el::text::impl::UnsafeU16StringAccess{text}.data();

        REQUIRE_NOT_EQUAL(data, nullptr);
        REQUIRE_EQUAL(data[0], u'H');
        REQUIRE_EQUAL(data[4], u'o');
        REQUIRE_EQUAL(data[5], u'\0');
    }
};
