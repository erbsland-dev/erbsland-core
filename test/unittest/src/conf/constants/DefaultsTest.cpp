// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/impl/constants/Defaults.hpp>
#include <erbsland/conf/impl/constants/Limits.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace erbsland::conf;
using namespace erbsland::text::literals;

TESTED_TARGETS(defaults limits)
class DefaultsTest final : public el::UnitTest {
public:
    void testDefaultValues() {
        REQUIRE(impl::defaults::documentHashAlgorithm == el::cryptology::HashAlgorithm::Sha3_256);
        REQUIRE_EQUAL(el::text::String{impl::defaults::textSourceIdentifier}, el::text::String{"text"_el});
        REQUIRE_EQUAL(el::text::String{impl::defaults::fileSourceIdentifier}, el::text::String{"file"_el});
        REQUIRE_EQUAL(el::text::String{impl::defaults::namePathIdentifier}, el::text::String{"name-path"_el});
        REQUIRE_EQUAL(el::text::String{impl::defaults::languageVersion}, el::text::String{"1.0"_el});
    }

    void testLimitValues() {
        REQUIRE_EQUAL(limits::maxLineLength, static_cast<std::size_t>(4000));
        REQUIRE_EQUAL(limits::maxNameLength, static_cast<std::size_t>(100));
        REQUIRE_EQUAL(limits::maxTextLength, static_cast<std::size_t>(10'000'000));
        REQUIRE_EQUAL(limits::maxDecimalDigits, static_cast<std::size_t>(19));
        REQUIRE_EQUAL(limits::maxHexadecimalDigits, static_cast<std::size_t>(16));
        REQUIRE_EQUAL(limits::maxBinaryDigits, static_cast<std::size_t>(64));
        REQUIRE_EQUAL(limits::maxOctalDigits, static_cast<std::size_t>(22));
        REQUIRE_EQUAL(limits::maximumDigits(el::text::IntegerBase::Decimal), static_cast<std::size_t>(19));
        REQUIRE_EQUAL(limits::maximumDigits(el::text::IntegerBase::Hexadecimal), static_cast<std::size_t>(16));
        REQUIRE_EQUAL(limits::maximumDigits(el::text::IntegerBase::Binary), static_cast<std::size_t>(64));
        REQUIRE_EQUAL(limits::maximumDigits(el::text::IntegerBase::Octal), static_cast<std::size_t>(22));
        REQUIRE_EQUAL(limits::maxNamePathLength, static_cast<std::size_t>(10));
        REQUIRE_EQUAL(limits::maxDocumentNesting, static_cast<std::size_t>(5));
    }
};
