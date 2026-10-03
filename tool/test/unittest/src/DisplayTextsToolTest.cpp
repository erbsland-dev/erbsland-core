// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFixture.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/StringFormat.hpp>
#include <tool/i18n/DisplayTextsTool.hpp>

#include <filesystem>

TESTED_TARGETS(DisplayTextsTool)
class DisplayTextsToolTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
public:
    void testConfigurationValidation() {
        auto tool = tool::i18n::DisplayTextsTool{};
        REQUIRE_EQUAL(tool.optionModule()->name(), "generate-display-texts"_el);
        auto parser = el::conf::Parser{};
        const auto section = parser.parseTextOrThrow("[generate_display_texts]\n"_el)->value(tool.identifier());
        tool.configurationRules()->validate(section, 1);
        REQUIRE_EQUAL(section->getTextOrThrow("class_name"_el), "DisplayTextCore"_el);
        for (
            const auto text :
            {"[generate_display_texts]\ninput_file: 1\n"_el, "[generate_display_texts]\nunknown: 1\n"_el}) {
            REQUIRE_THROWS(
                tool.configurationRules()->validate(parser.parseTextOrThrow(text)->value(tool.identifier()), 1));
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testGenerateAndCheck() {
        configureFixture();
        REQUIRE_EQUAL(runGenerator(el::StringList{"--check"_el}), el::ExitCode::failure().toRawValue());
        REQUIRE_FALSE((_root / "catalog/Words.hpp"_el).content().readText().has_value());
        REQUIRE_EQUAL(runGenerator(), el::ExitCode::success().toRawValue());
        const auto header = files().readText(el::Path{"catalog/Words.hpp"_el});
        const auto source = files().readText(el::Path{"catalog/Words.cpp"_el});
        REQUIRE(header.startsWith("// Copyright (c) 2001 Test Owner\n// SPDX-License-Identifier: Apache-2.0\n"_el));
        REQUIRE(source.startsWith("// Copyright (c) 2001 Test Owner\n// SPDX-License-Identifier: Apache-2.0\n"_el));
        REQUIRE(header.contains("namespace sample"_el));
        REQUIRE(source.contains("\"word\"_el, \"hello\"_el"_el));
        const auto timestamp = std::filesystem::last_write_time((_root / "catalog/Words.cpp"_el).toStdPath());
        REQUIRE_EQUAL(runGenerator(), el::ExitCode::success().toRawValue());
        REQUIRE_EQUAL(std::filesystem::last_write_time((_root / "catalog/Words.cpp"_el).toStdPath()), timestamp);
        REQUIRE_EQUAL(runGenerator(el::StringList{"--check"_el}), el::ExitCode::success().toRawValue());
        write("catalog/Words.elcl"_el, "[Root]\nWord: \"changed\"\n"_el);
        REQUIRE_EQUAL(runGenerator(el::StringList{"--check"_el}), el::ExitCode::failure().toRawValue());
        REQUIRE_EQUAL(files().readText(el::Path{"catalog/Words.cpp"_el}), source);
        REQUIRE_EQUAL(runGenerator(), el::ExitCode::success().toRawValue());
        REQUIRE(files().readText(el::Path{"catalog/Words.cpp"_el}).contains("changed"_el));
    }

    void testGenerateLight() {
        configureFixture();
        REQUIRE_EQUAL(runGenerator(), el::ExitCode::success().toRawValue());
        REQUIRE(files().readText(el::Path{"catalog/Words.hpp"_el}).contains("class Words final"_el));
        REQUIRE(files().readText(el::Path{"catalog/Words.cpp"_el}).contains("hello"_el));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testInvalidInputPreservesOutputs() {
        configureFixture();
        REQUIRE_EQUAL(runGenerator(), el::ExitCode::success().toRawValue());
        const auto header = files().readText(el::Path{"catalog/Words.hpp"_el});
        const auto source = files().readText(el::Path{"catalog/Words.cpp"_el});
        write("catalog/Words.elcl"_el, "[Root]\nWord: 42\n"_el);
        REQUIRE_EQUAL(runGenerator(), el::ExitCode::failure().toRawValue());
        REQUIRE_EQUAL(files().readText(el::Path{"catalog/Words.hpp"_el}), header);
        REQUIRE_EQUAL(files().readText(el::Path{"catalog/Words.cpp"_el}), source);
    }

    void testOutputPathsMustBeDistinctAndConfined() {
        configureFixture();
        for (const auto output : {"../outside.hpp"_el, "catalog/Words.elcl"_el, "catalog/Words.cpp"_el}) {
            writeConfiguration(output);
            REQUIRE_EQUAL(runGenerator(), el::ExitCode::failure().toRawValue());
            REQUIRE_EQUAL(files().readText(el::Path{"catalog/Words.elcl"_el}), "[Root]\nWord: \"hello\"\n"_el);
        }
    }

private:
    void configureFixture() {
        writeConfiguration("catalog/Words.hpp"_el);
        write("catalog/Words.elcl"_el, "[Root]\nWord: \"hello\"\n"_el);
    }

    void writeConfiguration(const el::String &header) {
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            el::StringFormat{"[rendering]\ncopyright_line: \"Copyright (c) 2001 Test Owner\"\n"
                             "[generate_display_texts]\ninput_file: \"catalog/Words.elcl\"\n"
                             "output_header: \"{}\"\noutput_source: \"catalog/Words.cpp\"\n"
                             "class_name: \"Words\"\nnamespace: \"sample\"\n"_el}
                .build(header));
    }

    auto runGenerator(const el::StringList &options = {}) -> int {
        return runTool<tool::i18n::DisplayTextsTool>({}, {}, "generate-display-texts"_el, options);
    }
};
