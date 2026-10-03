// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include <erbsland/ApplicationError.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <tool/github_workflows/TagCatalog.hpp>

using namespace el::text::literals;

TESTED_TARGETS(tool::github_workflows::StableTag tool::github_workflows::TagCatalog)
class TagCatalogTest final : public el::UnitTest {
public:
    void testNumericVersionsAndPeeledTagPrecedence() {
        const auto catalog = tool::github_workflows::TagCatalog{
            "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\trefs/tags/v10.0.0^{}\n"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\trefs/tags/v10.0.0\n"
            "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\trefs/tags/v2.0.0\n"
            "cccccccccccccccccccccccccccccccccccccccc\trefs/tags/v11.0.0-rc1\n"_el,
            "owner/repo"_el};
        REQUIRE_EQUAL(catalog.latest()->version.toString(), "v10.0.0"_el);
        REQUIRE_EQUAL(catalog.named("v10.0.0"_el)->sha, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"_el);
        REQUIRE_EQUAL(catalog.forSha("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"_el)->version.toString(), "v10.0.0"_el);
        REQUIRE_FALSE(catalog.forSha("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"_el).has_value());
        REQUIRE_FALSE(catalog.named("v10"_el).has_value());
    }

    void testMalformedAndConflictingResponses() {
        for (
            const auto &text : el::StringList{
                "not a tag\n"_el,
                "short\trefs/tags/v1.0.0\n"_el,
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\trefs/tags/v1.0.0^{}\n"_el,
                "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\trefs/tags/v1.0.0\n"
                "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\trefs/tags/v1.0.0\n"_el}) {
            REQUIRE_THROWS_AS(el::ApplicationError, tool::github_workflows::TagCatalog(text, "owner/repo"_el));
        }
        REQUIRE_FALSE(tool::github_workflows::StableTag::fromString("v01.0.0"_el).has_value());
        REQUIRE_FALSE(tool::github_workflows::StableTag::fromString("v18446744073709551616.0.0"_el).has_value());
        REQUIRE(tool::github_workflows::StableTag::fromString("v0.0.0"_el).has_value());
        REQUIRE_FALSE(tool::github_workflows::TagCatalog(""_el, "owner/repo"_el).latest().has_value());
    }
};
