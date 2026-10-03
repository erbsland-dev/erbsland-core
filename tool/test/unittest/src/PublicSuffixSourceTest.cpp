// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/base_n/BaseNEncoder.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/PathWriteDataOptions.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/unittest/TextHelper.hpp>

TESTED_TARGETS(PublicSuffixSource)
class PublicSuffixSourceTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public: // tests
    void testVerifiedTextAndVersionWithoutUnicode() {
        const auto text = "// VERSION: fixture_1\n// Unicode is retained: café\n"_el;
        prepareSource(text);
        runWithSources(el::StringList{tool::psl::PublicSuffixSource::sourceIdentifier()}, [&] {
            const auto &source = tool::frame::app().source<tool::psl::PublicSuffixSource>();
            REQUIRE_EQUAL(source.text(), text);
            REQUIRE_EQUAL(source.version(), "fixture_1"_el);
            REQUIRE_EQUAL(source.inputFile(), _root / "snapshot/list.dat"_el);
            REQUIRE_THROWS(tool::frame::app().source<tool::unicode::UnicodeSource>());
        });
    }

    void testCrLfSnapshotWithLfManifest() {
        const auto text = "// VERSION: fixture_1\n// retained\n"_el;
        prepareSource(text);
        write("snapshot/list.dat"_el, el::String{text}.replacedAll("\n"_el, "\r\n"_el));
        REQUIRE_EQUAL(runSource(), 0);
    }

    void testIntegrityFailure() {
        prepareSource("// VERSION: fixture_1\n"_el);
        write("snapshot/list.dat"_el, "// VERSION: fixture_1\nmodified\n"_el);
        REQUIRE_EQUAL(runSource(), el::ExitCode::failure().toRawValue());
        REQUIRE(_errorText.contains("Integrity Check Failed"_el));
    }

    void testTolerantUtf8WithValidChecksum() {
        const auto text =
            el::join("// VERSION: fixture_1\n// "_el, el::String{el::unittest::th::stdStringFromHex("ff")}, "\n"_el);
        prepareSource(text);
        runWithSources(el::StringList{tool::psl::PublicSuffixSource::sourceIdentifier()}, [&] {
            const auto &source = tool::frame::app().source<tool::psl::PublicSuffixSource>();
            REQUIRE(source.text().compareBinary(text) == std::strong_ordering::equal);
            REQUIRE_EQUAL(source.version(), "fixture_1"_el);
        });
    }

    void testAutomaticBomHandling() {
        prepareSource("\uFEFF// VERSION: fixture_1\n"_el);
        runWithSources(el::StringList{tool::psl::PublicSuffixSource::sourceIdentifier()}, [&] {
            const auto &source = tool::frame::app().source<tool::psl::PublicSuffixSource>();
            REQUIRE_EQUAL(source.text(), "// VERSION: fixture_1\n"_el);
            REQUIRE_EQUAL(source.version(), "fixture_1"_el);
        });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMissingRepeatedAndMismatchedVersionHeaders() {
        for (
            const auto &text : el::StringList{
                "// no version\n"_el, "// VERSION: other\n"_el, "// VERSION: fixture_1\n// VERSION: fixture_1\n"_el}) {
            prepareSource(text);
            REQUIRE_EQUAL(runSource(), el::ExitCode::failure().toRawValue());
            REQUIRE(_errorText.contains("version"_el));
            REQUIRE(_errorText.contains("list.dat"_el));
        }
        prepareSource("// VERSION: fixture_1\n"_el, "invalid version"_el);
        REQUIRE_EQUAL(runSource(), el::ExitCode::failure().toRawValue());
        REQUIRE(_errorText.contains("version syntax"_el));
    }

private: // fixture preparation
    /// Encode valid fixture text as unchanged UTF-8 bytes.
    void prepareSource(const el::String &text, const el::String &version = "fixture_1"_el) {
        prepareSource(el::StringEncoder{text}.encode(el::StringEncoding::Utf8), version);
    }
    /// Write fixture bytes and their integrity manifest.
    void prepareSource(const el::ByteBlock &bytes, const el::String &version = "fixture_1"_el) {
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[public_suffix]\ninput_file: \"snapshot/list.dat\"\n"_el);
        (_root / "snapshot"_el)
            .operations()
            .createDirectoryOrThrow(
                el::PathCreateDirectoryOptions{}.setCreationMode(el::PathCreateMode::CreateOrOverwrite));
        (_root / "snapshot/list.dat"_el)
            .content()
            .writeDataOrThrow(bytes, el::PathWriteDataOptions{}.setCreationMode(el::PathCreateMode::CreateOrOverwrite));
        auto hash = el::Hasher{el::HashAlgorithm::Sha3_256};
        hash.update(bytes);
        const auto checksum =
            el::text::base_n::BaseNEncoder{hash.finalize(), el::text::base_n::BaseNFormat::base16()}.toString();
        write(
            "snapshot/manifest.elcl"_el,
            el::StringFormat{"[snapshot]\nversion: \"{}\"\n[files]\n\"list.dat\": \"{}\"\n"_el}.build(
                version, checksum));
    }
    /// Run only the snapshot source to capture initialization failures.
    /// @return The process exit code.
    [[nodiscard]] auto runSource() -> int {
        const auto previous = SourceTestTool::identifiers;
        SourceTestTool::identifiers = el::StringList{tool::psl::PublicSuffixSource::sourceIdentifier()};
        const auto result = runTool<SourceTestTool>();
        SourceTestTool::identifiers = previous;
        return result;
    }
};
