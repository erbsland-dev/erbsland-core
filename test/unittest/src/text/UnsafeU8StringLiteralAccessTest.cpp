// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/Literals.hpp>
#include <erbsland/text/impl/UnsafeU8StringLiteralAccess.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(UnsafeU8StringLiteralAccess)
class UnsafeU8StringLiteralAccessTest final : public el::UnitTest {
public:
    void testConstantByteAccess() {
        constexpr auto literal = "a\0b"_el;
        constexpr auto valid = [](const auto &value) constexpr -> bool {
            const auto bytes = el::text::impl::UnsafeU8StringLiteralAccess{value}.constexprDataSpan();
            return bytes.size() == 3U && bytes[0] == 'a' && bytes[1] == '\0' && bytes[2] == 'b';
        }(literal);
        static_assert(valid);
        REQUIRE(valid);
    }

    void testRuntimeAccess() {
        const auto literal = "a\0b"_el;
        const auto u8Literal = u8"a\0b"_el;
        const auto bytes = el::text::impl::UnsafeU8StringLiteralAccess{literal}.dataSpan();
        const auto u8Bytes = el::text::impl::UnsafeU8StringLiteralAccess{u8Literal}.dataSpan();
        REQUIRE_EQUAL(bytes.size(), std::size_t{3});
        REQUIRE_EQUAL(u8Bytes.size(), bytes.size());
        for (auto index = std::size_t{}; index < bytes.size(); ++index) {
            REQUIRE_EQUAL(u8Bytes[index], bytes[index]);
        }
        const auto empty = ""_el;
        REQUIRE(el::text::impl::UnsafeU8StringLiteralAccess{empty}.dataSpan().empty());
    }
};
