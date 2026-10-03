// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProjectFixture.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/vr/Rule.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/OptionValue.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/anti_patterns/AntiPatternsTool.hpp>
#include <tool/doc/BuildDocTool.hpp>
#include <tool/doc/CreateDemoTool.hpp>
#include <tool/doc/impl/ReferenceDocumentation.hpp>
#include <tool/doc/impl/SourceSelectors.hpp>
#include <tool/doc/ProcessRstTool.hpp>
#include <tool/doc/UpdateReferenceTool.hpp>
#include <tool/i18n/DisplayTextsTool.hpp>

TESTED_TARGETS(ProjectFiles ProcessRstTool BuildDocTool ReferenceDocumentation AntiPatternsTool)
class ToolErrorReportingTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
private:
    static constexpr auto cInputPath = "paths.input"_el; ///< Configuration value used by the fixture.

public:
    void testConfiguredPathRetainsValueLocationAndCause() {
        const auto document = el::conf::Parser{}.parseTextOrThrow("[paths]\ninput: \"../outside\"\n"_el);
        const auto value = document->valueOrThrow(cInputPath);
        try {
            files().checked(value);
            REQUIRE(false);
        } catch (const el::conf::ConfError &error) {
            REQUIRE_EQUAL(error.title(), "Invalid Configured Path"_el);
            REQUIRE_EQUAL(error.location(), value->location().codeLocation());
            REQUIRE_EQUAL(error.namePath(), value->namePath());
            REQUIRE(error.cause() != nullptr);
        }
    }

    void testOptionPathRetainsArgumentLocation() {
        const auto value = el::OptionValue::create(el::String{"../outside"_el}, {el::ArgumentIndex{3}});
        try {
            files().checked(value);
            REQUIRE(false);
        } catch (const el::OptionError &error) {
            REQUIRE_EQUAL(error.context().title(), "Invalid Path Option"_el);
            REQUIRE_EQUAL(error.context().argumentIndex(), el::ArgumentIndex{3});
            REQUIRE(error.cause() != nullptr);
        }
    }

    void testRequiredPathTypesKeepInputContext() {
        const auto document = el::conf::Parser{}.parseTextOrThrow("[paths]\ninput: \"missing.txt\"\n"_el);
        const auto value = document->valueOrThrow(cInputPath);
        try {
            const auto path = files().checked(value, el::PathType::RegularFile);
            REQUIRE(false);
        } catch (const el::conf::ConfError &error) {
            REQUIRE_EQUAL(error.namePath(), value->namePath());
            REQUIRE_EQUAL(error.location(), value->location().codeLocation());
            REQUIRE(error.cause() != nullptr);
        }
        const auto option = el::OptionValue::create(el::String{"tool"_el}, {el::ArgumentIndex{3}});
        try {
            const auto path = files().checked(option, el::PathType::RegularFile, true);
            REQUIRE(false);
        } catch (const el::OptionError &error) {
            REQUIRE_EQUAL(error.context().argumentIndex(), el::ArgumentIndex{3});
            REQUIRE(error.cause() != nullptr);
        }
        REQUIRE_EQUAL(files().checked(value, el::PathType::RegularFile, true), _root / "missing.txt"_el);
    }

    void testGlobalPathOptionsReportOptionErrors() {
        for (
            const auto &options : el::List<el::StringList>{
                el::StringList{"--config"_el, "missing.elcl"_el},
                el::StringList{"--config"_el, ""_el},
                el::StringList{"--log-file"_el, ""_el},
                el::StringList{"--log-file"_el, _root.toString()}}) {
            REQUIRE(runTool({}, {}, "probe"_el, options) != 0);
            REQUIRE(_errorText.contains("Option Help"_el));
        }
    }

    void testReferencePageFailureIdentifiesConfigurationValue() {
        write("src/erbsland/example/Value.hpp"_el, "class Value {};\n"_el);
        const auto document = el::conf::Parser{}.parseTextOrThrow(
            "[doc_update_reference]\n*[doc_update_reference.reference_groups]\n"
            "page: \"../../outside.rst\"\nheaders: \"example/Value.hpp\"\n"_el);
        const auto configuration = document->valueOrThrow("doc_update_reference"_el);
        tool::doc::UpdateReferenceTool{}.configurationRules()->validate(configuration, 1);
        const auto page = (*configuration->getSectionList("reference_groups"_el)->begin())->valueOrThrow("page"_el);
        const auto service = files();
        try {
            const auto reference = tool::doc::impl::ReferenceDocumentation{service, configuration};
            REQUIRE(false);
        } catch (const el::conf::ConfError &error) {
            REQUIRE_EQUAL(error.title(), "Invalid Reference Page Path"_el);
            REQUIRE_EQUAL(error.namePath(), page->namePath());
            REQUIRE_EQUAL(error.location(), page->location().codeLocation());
        }
    }

    void testDocumentationFailuresHaveSpecificTitles() {
        try {
            const auto selected = tool::doc::impl::SourceSelectors{"void example() {}\n"_el}.select("f:missing"_el, {});
            REQUIRE(false);
        } catch (const el::ApplicationError &error) {
            REQUIRE_EQUAL(error.title(), "Invalid Demo Source Selector"_el);
        }
        REQUIRE(
            runTool<tool::doc::CreateDemoTool>(
                {}, {}, "create-demo"_el, el::StringList{"../Invalid/Part"_el, "--yes"_el}) != 0);
        REQUIRE(_errorText.contains("Invalid Demo Path"_el));
        REQUIRE(_errorText.contains("Option Help"_el));
    }

    void testInvalidRstOptionsAndConfiguration() {
        write("page.rst"_el, "Page\n====\n"_el);
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[doc_process_rst]\n"_el);
        REQUIRE(
            runTool<tool::doc::ProcessRstTool>(
                {}, {}, "doc-process-rst"_el, el::StringList{"page.rst"_el, "--no-cleanup"_el}) != 0);
        REQUIRE(_errorText.contains("Invalid RST Processing Options"_el));
        REQUIRE(_errorText.contains("--no-cleanup"_el));
        write(
            "tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[doc_process_rst]\ndemo_executable_root: \"\"\n"_el);
        REQUIRE(runTool<tool::doc::ProcessRstTool>({}, {}, "doc-process-rst"_el, el::StringList{"page.rst"_el}) != 0);
        REQUIRE(_errorText.contains("Invalid Configured Path"_el));
        REQUIRE(_errorText.contains("demo_executable_root"_el));
    }

    void testWarningPatternsIdentifyTheFailedSetting() {
        for (const auto &key : el::StringList{"source_glob"_el, "message_regex"_el}) {
            auto settings = el::String{"source_glob: \"src/**\"\nmessage_regex: \"warning\"\n"_el};
            if (key == "source_glob"_el) {
                settings = "source_glob: \"[\"\n"_el;
            } else {
                settings = "source_glob: \"src/**\"\nmessage_regex: \"[\"\n"_el;
            }
            write(
                "tool/conf/erbsland-core-tool.elcl"_el,
                el::join(
                    "[rendering]\n[doc_build]\n*[doc_build.ignored_doxygen_warnings]\n"_el,
                    settings,
                    "reason: \"Test exception\"\n"_el));
            REQUIRE(runTool<tool::doc::BuildDocTool>({}, {}, "doc-build"_el) != 0);
            REQUIRE(_errorText.contains(key));
            REQUIRE(_errorText.contains("Configuration Error Details"_el));
        }
    }

    void testReferenceExclusionPatternRetainsConfigurationContext() {
        write("src/erbsland/example/Value.hpp"_el, "class Value {};\n"_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[doc_update_reference]\nexcluded_header_globs: \"[\"\n"_el);
        REQUIRE(runTool<tool::doc::UpdateReferenceTool>({}, {}, "doc-update-reference"_el) != 0);
        REQUIRE(_errorText.contains("Invalid Reference Exclusion Pattern"_el));
        REQUIRE(_errorText.contains("excluded_header_globs"_el));
    }

    void testAntiPatternOptionsAndRuleHelp() {
        write("src/Example.cpp"_el, ""_el);
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[anti_patterns]\nsource_directories: \"src\"\nfile_suffixes: \".cpp\"\n"_el);
        REQUIRE(
            runTool<tool::anti_patterns::AntiPatternsTool>(
                {}, {}, "anti-patterns"_el, el::StringList{"--jobs"_el, "0"_el}) != 0);
        REQUIRE(_errorText.contains("Invalid Worker Count"_el));
        REQUIRE(_errorText.contains("--jobs"_el));
        const auto document = el::conf::Parser{}.parseTextOrThrow(
            "[anti_patterns]\nsource_directories: \"src\"\nfile_suffixes: \".cpp\"\n"
            "[anti_patterns.missing_api_documentation]\n"_el);
        const auto section = document->valueOrThrow("anti_patterns"_el);
        tool::anti_patterns::AntiPatternsTool{}.configurationRules()->validate(section, 1);
        const auto rule = section->valueOrThrow("missing_api_documentation"_el)->validationRule();
        REQUIRE_EQUAL(rule->title(), "Missing API documentation"_el);
        REQUIRE_FALSE(rule->description().isEmpty());
    }
};
