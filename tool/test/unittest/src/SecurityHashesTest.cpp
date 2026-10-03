// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/Rule.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <tool/security_hashes/Inventory.hpp>
#include <tool/security_hashes/Manifest.hpp>
#include <tool/security_hashes/SecurityHashesTool.hpp>

#include <filesystem>

TESTED_TARGETS(SecurityHashesTool Settings Inventory Manifest)
class SecurityHashesTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
private:
    static constexpr auto cSection = "security_hashes"_el; ///< Configuration value used by the fixture.

public:                                                    // implement UnitTest
    void setUp() override {
        ProjectFixture::setUp();
        write("inputs/a.cpp"_el, "a\n"_el);
        write("inputs/b.cpp"_el, "b\n"_el);
        configure();
    }

public: // tests
    void testSerialAndParallelManifestLight() {
        const auto service = files();
        const auto settings = readSettings(service);
        const auto inputs = tool::security_hashes::Inventory{settings, service}.collect();
        REQUIRE_EQUAL(inputs.count(), el::ItemCount{2});
        REQUIRE_EQUAL(inputs.getRef(el::ItemIndex{}).relative, "inputs/a.cpp"_el);
        const auto manifest = tool::security_hashes::Manifest{service};
        const auto serial = manifest.generate(inputs, 1);
        REQUIRE_EQUAL(serial, manifest.generate(inputs, 4));
        REQUIRE_EQUAL(serial, manifest.generate(inputs, 64));
        REQUIRE(serial.contains("\n* 4fd12a6ee8c0d4342f0e6b656b31c57f728f1f3558a74691a55103f2855cdd0f\n"_el));
        REQUIRE(serial.contains("# Hash format: security-hashes-v2\n"_el));
        REQUIRE(serial.contains("\ninputs/ "_el));
        REQUIRE(serial.contains("\ninputs/a.cpp "_el));
    }

    void testSchemaMetadataLight() {
        const auto document = files().readConfiguration(el::Path{"tool/conf/erbsland-core-tool.elcl"_el});
        const auto section = document->valueOrThrow(cSection);
        tool::security_hashes::Settings::rules()->validate(section, 1);
        REQUIRE_FALSE(section->validationRule()->title().isEmpty());
        REQUIRE_FALSE(section->validationRule()->description().isEmpty());
        for (const auto &value : *section) {
            REQUIRE_FALSE(value->validationRule()->title().isEmpty());
            REQUIRE_FALSE(value->validationRule()->description().isEmpty());
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRepeatedParallelRuns() {
        const auto content = el::String{el::StringEditor{}.append("source\r\n"_el, el::ItemCount{2048})};
        for (auto index = 0; index < 40; ++index) {
            write(el::StringFormat{"inputs/sub/source-{}.cpp"_el}.build(index), content);
        }
        const auto service = files();
        const auto settings = readSettings(service);
        const auto inputs = tool::security_hashes::Inventory{settings, service}.collect();
        const auto manifest = tool::security_hashes::Manifest{service};
        const auto serial = manifest.generate(inputs, 1);
        for (auto run = 0; run < 5; ++run) {
            REQUIRE_EQUAL(manifest.generate(inputs, 8), serial);
        }
    }

    void testBinaryPathOrderingLight() {
        write("inputs/z.cpp"_el, "z"_el);
        write("inputs/é.cpp"_el, "accented"_el);
        write("inputs/Beta.cpp"_el, "B"_el);
        const auto service = files();
        const auto settings = readSettings(service);
        const auto inputs = tool::security_hashes::Inventory{settings, service}.collect();
        auto names = el::StringList{};
        for (const auto &input : inputs) {
            names.append(input.relative);
        }
        REQUIRE_EQUAL(
            names,
            (el::StringList{
                "inputs/Beta.cpp"_el, "inputs/a.cpp"_el, "inputs/b.cpp"_el, "inputs/z.cpp"_el, "inputs/é.cpp"_el}));
    }

    void testSelectionAndDeduplicationLight() {
        write("inputs/data/fixture.cpp"_el, "excluded source fixture"_el);
        write("inputs/a.bin"_el, "excluded binary"_el);
        write("inputs/.cache/cache.cpp"_el, "excluded cache"_el);
        write("inputs/ignored.cpp"_el, "excluded filename"_el);
        write("inputs/sub/template.tpl"_el, "template"_el);
        write("inputs/sub/config.elcl"_el, "configuration"_el);
        write("inputs/sub/CMakeLists.txt"_el, "build"_el);
        const auto service = files();
        const auto settings = readSettings(service);
        const auto inputs = tool::security_hashes::Inventory{settings, service}.collect();
        REQUIRE_EQUAL(inputs.count(), el::ItemCount{5});
        REQUIRE(inputs.getRef(el::ItemIndex{}).directoryRecord);
        REQUIRE(inputs.getRef(el::ItemIndex{}).fileRecord);
        const auto manifest = tool::security_hashes::Manifest{service};
        const auto before = manifest.generate(inputs, 2);
        write("inputs/data/fixture.cpp"_el, "changed data"_el);
        write("inputs/new-data.txt"_el, "new data"_el);
        (_root / "inputs/empty"_el).operations().createDirectoryOrThrow();
        REQUIRE_EQUAL(before, manifest.generate(tool::security_hashes::Inventory{settings, service}.collect(), 2));
        write("inputs/sub/template.tpl"_el, "changed template"_el);
        REQUIRE(before != manifest.generate(inputs, 2));
        write("inputs/sub/template.tpl"_el, "template"_el);
        write("inputs/sub/config.elcl"_el, "changed configuration"_el);
        REQUIRE(before != manifest.generate(inputs, 2));
        write("inputs/sub/config.elcl"_el, "configuration"_el);
        write("inputs/new.cpp"_el, "new source"_el);
        REQUIRE_EQUAL((tool::security_hashes::Inventory{settings, service}.collect().count()), el::ItemCount{6});
        (_root / "inputs/new.cpp"_el).operations().removeOrThrow();
        (_root / "inputs/b.cpp"_el).operations().removeOrThrow();
        write("inputs/renamed.cpp"_el, "b\n"_el);
        REQUIRE(before != manifest.generate(tool::security_hashes::Inventory{settings, service}.collect(), 2));
    }

    void testLineEndingsAndSignificantBytesLight() {
        const auto service = files();
        const auto settings = readSettings(service);
        const auto inputs = tool::security_hashes::Inventory{settings, service}.collect();
        const auto manifest = tool::security_hashes::Manifest{service};
        write("inputs/a.cpp"_el, "first\nsecond\nthird\n"_el);
        const auto expected = manifest.generate(inputs, 2);
        for (
            const auto &text : el::StringList{
                "first\r\nsecond\r\nthird\r\n"_el, "first\rsecond\rthird\r"_el, "first\r\nsecond\rthird\n"_el}) {
            write("inputs/a.cpp"_el, text);
            REQUIRE_EQUAL(expected, manifest.generate(inputs, 2));
        }
        for (
            const auto &text : el::StringList{
                "first\nsecond\nthird"_el,
                "first \nsecond\nthird\n"_el,
                "\uFEFFfirst\nsecond\nthird\n"_el,
                "First\nsecond\nthird\n"_el}) {
            write("inputs/a.cpp"_el, text);
            REQUIRE(expected != manifest.generate(inputs, 2));
        }
    }

    void testPublishingVerificationAndFailurePreservationLight() {
        const auto service = files();
        const auto settings = readSettings(service);
        const auto inputs = tool::security_hashes::Inventory{settings, service}.collect();
        const auto manifest = tool::security_hashes::Manifest{service};
        const auto text = manifest.generate(inputs, 2);
        REQUIRE_THROWS_AS(el::ApplicationError, manifest.publish(settings.output, text, true));
        REQUIRE_FALSE(settings.output.info().exists());
        REQUIRE(manifest.publish(settings.output, text, false));
        REQUIRE_FALSE(manifest.publish(settings.output, text, false));
        REQUIRE_FALSE(manifest.publish(settings.output, text, true));
        REQUIRE_EQUAL((tool::security_hashes::Inventory{settings, service}.collect().count()), el::ItemCount{2});
        for (const auto &ending : el::StringList{"\r\n"_el, "\r"_el}) {
            write("inputs/manifest.elcl"_el, text.replacedAll("\n"_el, ending));
            REQUIRE_FALSE(manifest.publish(settings.output, text, true));
        }
        write("inputs/manifest.elcl"_el, text.trimmed());
        REQUIRE_THROWS_AS(el::ApplicationError, manifest.publish(settings.output, text, true));
        REQUIRE(manifest.publish(settings.output, text, false));
        (_root / "inputs/b.cpp"_el).operations().removeOrThrow();
        REQUIRE_THROWS_AS(el::ApplicationError, manifest.generate(inputs, 2));
        REQUIRE_EQUAL(service.readText(service.checked(settings.output)), text);
    }

    void testConfigurationDiagnosticsLight() {
        const auto service = files();
        for (
            const auto &setting : el::StringList{
                "output_file: \"../escape.txt\"\n"_el,
                "output_file: \"inputs\"\n"_el,
                "directories: \"missing\"\n"_el,
                "files: \"missing.cpp\"\n"_el,
                "files: \"inputs\"\n"_el,
                "output_file: \"missing/manifest.txt\"\n"_el,
                "output_file: \"/absolute.txt\"\n"_el,
                "output_file: \"inputs/unsafe\\nname.txt\"\n"_el,
                "included_file_suffixes: \"cpp\"\n"_el,
                "excluded_directory_names: \"../escape\"\n"_el}) {
            const auto document = el::conf::Parser{}.parseTextOrThrow(el::join("[security_hashes]\n"_el, setting));
            const auto section = document->valueOrThrow(cSection);
            tool::security_hashes::Settings::rules()->validate(section, 1);
            auto settings = tool::security_hashes::Settings{};
            REQUIRE_THROWS_AS(el::conf::ConfError, settings.read(section, service));
        }
    }

    void testSymlinkInputIsRejectedLight() {
        auto error = std::error_code{};
        std::filesystem::create_symlink(
            (_root / "inputs/a.cpp"_el).toStdPath(), (_root / "inputs/link.cpp"_el).toStdPath(), error);
        if (error) {
            return; // Windows may not grant symlink creation to the test account.
        }
        const auto service = files();
        const auto settings = readSettings(service);
        REQUIRE_THROWS_AS(el::ApplicationError, (tool::security_hashes::Inventory{settings, service}.collect()));
    }

    void testCommandWorkerOptionsAndCheckAliasLight() {
        REQUIRE_EQUAL(runCommand(el::StringList{"--jobs"_el, "0"_el}), el::ExitCode::failure().toRawValue());
        REQUIRE(_errorText.contains("--jobs"_el));
        REQUIRE_FALSE((_root / "inputs/manifest.elcl"_el).info().exists());
        REQUIRE_EQUAL(runCommand(el::StringList{"--jobs"_el, "65"_el}), el::ExitCode::failure().toRawValue());
        REQUIRE_EQUAL(
            runCommand(el::StringList{"--jobs"_el, "not-an-integer"_el}), el::ExitCode::failure().toRawValue());
        REQUIRE(_errorText.contains("--jobs"_el));
        REQUIRE_EQUAL(runCommand(el::StringList{"--jobs"_el, "1"_el}), el::ExitCode::success().toRawValue());
        REQUIRE_EQUAL(runCommand(el::StringList{"--check"_el}), el::ExitCode::success().toRawValue());
        REQUIRE_EQUAL(runCommand(el::StringList{"--verify"_el, "-j"_el, "4"_el}), el::ExitCode::success().toRawValue());
        write("inputs/a.cpp"_el, "changed\n"_el);
        const auto previous = files().readData(_root / "inputs/manifest.elcl"_el);
        REQUIRE_EQUAL(runCommand(el::StringList{"--verify"_el}), el::ExitCode::failure().toRawValue());
        REQUIRE_EQUAL(files().readData(_root / "inputs/manifest.elcl"_el), previous);
    }

private:
    void configure() {
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[security_hashes]\noutput_file: \"inputs/manifest.elcl\"\n"
            "directories: \"inputs\", \"inputs\"\nfiles: \"inputs/a.cpp\", \"inputs/a.cpp\"\n"
            "included_file_suffixes: \".cpp\", \".elcl\", \".tpl\"\nincluded_file_names: \"CMakeLists.txt\"\n"
            "excluded_directory_paths: \"inputs/data\"\nexcluded_directory_names: \".cache\"\n"
            "excluded_file_names: \"ignored.cpp\"\n"_el);
    }
    [[nodiscard]] auto readSettings(const tool::frame::ProjectFiles &service) -> tool::security_hashes::Settings {
        const auto document = service.readConfiguration(el::Path{"tool/conf/erbsland-core-tool.elcl"_el});
        const auto section = document->valueOrThrow(cSection);
        tool::security_hashes::Settings::rules()->validate(section, 1);
        auto result = tool::security_hashes::Settings{};
        result.read(section, service);
        return result;
    }
    auto runCommand(const el::StringList &options) -> int {
        return runTool<tool::security_hashes::SecurityHashesTool>({}, {}, "security-hashes"_el, options);
    }
};
