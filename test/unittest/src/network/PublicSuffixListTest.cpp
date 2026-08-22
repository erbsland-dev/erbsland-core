// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/network/impl/http/cookie/PublicSuffixData.hpp>
#include <erbsland/network/impl/http/cookie/PublicSuffixList.hpp>
#include <erbsland/network/impl/http/cookie/PublicSuffixLookup.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

using namespace el::text::literals;

TESTED_TARGETS(PublicSuffixList PublicSuffixLookup)
class PublicSuffixListTest final : public el::UnitTest {
public:
    void testPinnedCompleteListAndRegistrableDomains() {
        REQUIRE_FALSE(el::network::impl::public_suffix_data::version().isEmpty());
        const auto data = el::network::impl::public_suffix_data::data();
        REQUIRE_EQUAL(data.ruleCount, 10248U);
        REQUIRE(data.encodedStorageByteCount < 45000U);
        REQUIRE(data.encodedStorageByteCount * 4U <= data.originalRuleTextByteCount + data.ruleCount * 8U);

        REQUIRE(el::network::impl::public_suffix::isPublicSuffix("com"_el));
        REQUIRE(el::network::impl::public_suffix::isPublicSuffix("co.uk"_el));
        REQUIRE_EQUAL(el::network::impl::public_suffix::registrableDomain("www.example.co.uk"_el), "example.co.uk"_el);
        REQUIRE_EQUAL(
            el::network::impl::public_suffix::registrableDomain("images.blogspot.com"_el), "images.blogspot.com"_el);
    }

    void testDeepExactAndUnknownRules() {
        REQUIRE(el::network::impl::public_suffix::isPublicSuffix("aaa"_el));
        REQUIRE(el::network::impl::public_suffix::isPublicSuffix("co.ae"_el));
        REQUIRE(el::network::impl::public_suffix::isPublicSuffix("org.zw"_el));
        REQUIRE(
            el::network::impl::public_suffix::isPublicSuffix(
                "s3-accesspoint.dualstack.cn-north-1.amazonaws.com.cn"_el));
        REQUIRE_EQUAL(
            el::network::impl::public_suffix::registrableDomain(
                "bucket.s3-accesspoint.dualstack.cn-north-1.amazonaws.com.cn"_el),
            "bucket.s3-accesspoint.dualstack.cn-north-1.amazonaws.com.cn"_el);
        REQUIRE(el::network::impl::public_suffix::isPublicSuffix("made-up-tld"_el));
        REQUIRE_EQUAL(
            el::network::impl::public_suffix::registrableDomain("example.made-up-tld"_el), "example.made-up-tld"_el);
    }

    void testWildcardAndExceptionRules() {
        REQUIRE(el::network::impl::public_suffix::isPublicSuffix("b.ck"_el));
        REQUIRE_EQUAL(el::network::impl::public_suffix::registrableDomain("a.b.ck"_el), "a.b.ck"_el);
        REQUIRE_FALSE(el::network::impl::public_suffix::isPublicSuffix("www.ck"_el));
        REQUIRE_EQUAL(el::network::impl::public_suffix::registrableDomain("www.ck"_el), "www.ck"_el);
        REQUIRE(el::network::impl::public_suffix::isPublicSuffix("foo.kawasaki.jp"_el));
        REQUIRE_FALSE(el::network::impl::public_suffix::isPublicSuffix("city.kawasaki.jp"_el));
        REQUIRE_EQUAL(
            el::network::impl::public_suffix::registrableDomain("city.kawasaki.jp"_el), "city.kawasaki.jp"_el);
    }

    void testEmptyAndTrailingDotInputs() {
        const auto lookup = el::network::impl::PublicSuffixLookup{};
        REQUIRE_EQUAL(lookup.publicSuffixLabelCount({}), 0U);
        REQUIRE_FALSE(el::network::impl::public_suffix::isPublicSuffix({}));
        REQUIRE(el::network::impl::public_suffix::registrableDomain({}).isEmpty());
        REQUIRE_EQUAL(lookup.publicSuffixLabelCount("com."_el), 0U);
        REQUIRE_FALSE(el::network::impl::public_suffix::isPublicSuffix("com."_el));
        REQUIRE(el::network::impl::public_suffix::registrableDomain("com."_el).isEmpty());
    }
};
