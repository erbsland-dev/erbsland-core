// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/u8/impl/U8StringLiteralStorage.hpp>
#include <erbsland/text/u8/impl/U8StringSharedStorage.hpp>
#include <erbsland/text/u8/impl/U8StringStorage.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string_view>
#include <variant>

TESTED_TARGETS(U8StringStorage)
class U8StringStorageTest final : public el::UnitTest {
public:
    void testVariantAlternatives() {
        auto storage = el::text::impl::U8StringStorage{};
        REQUIRE(std::holds_alternative<std::monostate>(storage));

        storage = el::text::impl::U8StringSharedStorage{std::string_view{"Hello"}};
        REQUIRE(std::holds_alternative<el::text::impl::U8StringSharedStorage>(storage));

        storage = el::text::impl::U8StringLiteralStorage{"Hello", 5U};
        REQUIRE(std::holds_alternative<el::text::impl::U8StringLiteralStorage>(storage));
    }
};
