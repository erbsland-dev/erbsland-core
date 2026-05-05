// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/impl/UnsafeU8StringAccess.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstring>
#include <string_view>

using el::text::U8String;

TESTED_TARGETS(UnsafeU8StringAccess)
class UnsafeU8StringAccessTest final : public el::UnitTest {
public:
    void testEmptyStringReturnsNullPointer() {
        const auto text = U8String{};

        REQUIRE_EQUAL(el::text::impl::UnsafeU8StringAccess{text}.data(), nullptr);
    }

    void testStringReturnsNullTerminatedData() {
        const auto text = U8String{std::string_view{"Hello"}};
        const auto *data = el::text::impl::UnsafeU8StringAccess{text}.data();

        REQUIRE_NOT_EQUAL(data, nullptr);
        REQUIRE_EQUAL(std::strcmp(data, "Hello"), 0);
    }
};
