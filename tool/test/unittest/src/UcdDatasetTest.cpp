// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "UcdDatasetFixture.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/unittest/TextHelper.hpp>

TESTED_TARGETS(UcdDataset)
class UcdDatasetTest final : public UNITTEST_SUBCLASS(UcdDatasetFixture) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testVerifiedSnapshot() {
        runInsideApplication([&]() -> void {
            prepareDataset();
            const auto dataset = tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el};
            REQUIRE(dataset.text("UnicodeData.txt"_el).contains("LATIN SMALL LETTER A"_el));
            REQUIRE_THROWS(dataset.text("unlisted.txt"_el));
        });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testChecksumAndVersionFailures() {
        runInsideApplication([&]() -> void {
            prepareDataset();
            REQUIRE_THROWS((tool::unicode::UcdDataset{el::Path{"ucd"_el}, "18.0.0"_el}));
            write("ucd/Blocks.txt"_el, "corrupted"_el);
            REQUIRE_THROWS((tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el}));
            write("ucd/manifest.elcl"_el, "[dataset]\nunicode_version: \"16.0.0\"\n"_el);
            REQUIRE_THROWS((tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el}));
        });
    }

    void testCrLfSourceWithLfManifest() {
        runInsideApplication([&] {
            prepareDataset();
            const auto path = el::Path{"ucd/UnicodeData.txt"_el};
            write(path.toString(), files().readText(path).replacedAll("\n"_el, "\r\n"_el));
            const auto dataset = tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el};
            REQUIRE(dataset.text("UnicodeData.txt"_el).contains("LATIN SMALL LETTER A"_el));
        });
    }

    void testSourceVersionMismatch() {
        runInsideApplication([&]() -> void {
            prepareDataset("0061;LETTER A;Ll;0;L;;;;;N;;;;;\n"_el, {}, "16.0.0"_el);
            try {
                [[maybe_unused]] const auto dataset = tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el};
                REQUIRE(false);
            } catch (const el::ApplicationError &error) {
                REQUIRE_FALSE(error.title().isEmpty());
                REQUIRE(error.description().contains("expected Unicode version"_el));
                REQUIRE_EQUAL(error.sourcePath(), (_root / "ucd/Blocks.txt"_el).toString());
                REQUIRE_FALSE(error.cause());
            }
        });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMissingManifestEntriesAndInput() {
        runInsideApplication([&]() -> void {
            prepareDataset();
            write("ucd/manifest.elcl"_el, "[dataset]\nunicode_version: \"17.0.0\"\n[files]\n"_el);
            REQUIRE_THROWS((tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el}));
            prepareDataset();
            (_root / "ucd/Blocks.txt"_el).operations().removeOrThrow();
            REQUIRE_THROWS((tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el}));
        });
    }

    void testTolerantUtf8WithValidChecksum() {
        runInsideApplication([&]() -> void {
            auto overrides = el::StringMap<el::String>{};
            const auto body = el::String{el::unittest::th::stdStringFromHex("80")};
            overrides.set("Blocks.txt"_el, body);
            prepareDataset("0061;LETTER A;Ll;0;L;;;;;N;;;;;\n"_el, overrides);
            const auto dataset = tool::unicode::UcdDataset{el::Path{"ucd"_el}, "17.0.0"_el};
            const auto expected = el::join("# Blocks.txt-17.0.0.txt\n"_el, body);
            REQUIRE(dataset.text("Blocks.txt"_el).compareBinary(expected) == std::strong_ordering::equal);
        });
    }
};
