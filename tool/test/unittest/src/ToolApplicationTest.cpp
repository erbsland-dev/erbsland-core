// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConfiguredTool.hpp"
#include "ProjectFixture.hpp"

#include <erbsland/DateTime.hpp>
#include <erbsland/render/FileSystemLoader.hpp>
#include <erbsland/Year.hpp>

using namespace el::text::literals;

TESTED_TARGETS(ToolApplication)
class ToolApplicationTest final : public UNITTEST_SUBCLASS(ProjectFixture) {
private:
    static constexpr auto cMessageValue = "message"_el;

    /// Probe whose configuration identifier differs from its canonical command spelling.
    class MultiWordTool final : public NoConfigurationTool {
    public: // implement Tool
        auto identifier() const -> el::String override { return "test_probe"_el; }
        auto configurationRules() const -> el::conf::vr::RulesPtr override {
            return ConfiguredTool{}.configurationRules();
        }
    };

    class StandaloneProbe final : public NoConfigurationTool {
    public: // implement Tool
        auto requiresProject() const noexcept -> bool override { return false; }
        void execute() override {
            log()->trace("standalone trace entry"_el);
            NoConfigurationTool::execute();
        }
    };

public:
    void testStandaloneLoggingUsesSharedOptions() {
        const auto logFile = _root / "standalone.log"_el;
        const auto action = [this](const el::conf::ValuePtr &section) -> void {
            REQUIRE_FALSE(section);
            REQUIRE_THROWS_AS(el::LogicFailure, tool::frame::app().files());
        };
        REQUIRE_EQUAL(
            runTool<StandaloneProbe>(
                action, {}, "probe"_el, el::StringList{"--log-file"_el, logFile.toString()}, false),
            0);
        REQUIRE_FALSE(_outputText.contains("standalone trace entry"_el));
        REQUIRE(logFile.content().readTextOrThrow().contains("standalone trace entry"_el));

        REQUIRE_EQUAL(runTool<StandaloneProbe>(action, {}, "probe"_el, el::StringList{"--verbose"_el}, false), 0);
        REQUIRE(_outputText.contains("standalone trace entry"_el));
    }

    void testCanonicalCommandDispatchKeepsConfigurationIdentity() {
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[test_probe]\nmessage: \"configured\"\n"_el);
        for (const auto &command : el::StringList{"test-probe"_el, "test_probe"_el}) {
            auto executed = false;
            REQUIRE_EQUAL(
                runTool<MultiWordTool>(
                    [&](const auto &section) {
                        REQUIRE(section);
                        REQUIRE_EQUAL(section->getTextOrThrow(cMessageValue), "configured"_el);
                        REQUIRE_EQUAL(tool::frame::app().optionValues()->moduleName(), "test-probe"_el);
                        executed = true;
                    },
                    {},
                    command),
                el::ExitCode::success().toRawValue());
            REQUIRE(executed);
        }
        REQUIRE_EQUAL(tool::idna::IdnaTool{}.identifier(), "generate_idna_data"_el);
        REQUIRE_EQUAL(tool::idna::IdnaTool{}.optionModule()->name(), "generate-idna-data"_el);
    }

    void testCppTemplateDelimiters() {
        runInsideApplication([&] {
            write(
                "template.cpp"_el,
                "[[nodiscard]] auto value() -> int {\n/*% if true %*/    return /*{ 42 }*/;\n/*% endif %*/}\n"_el);
            const auto renderer = tool::frame::app().createRenderEnvironment(tool::frame::TemplateLanguage::Cpp);
            renderer->addLayoutLoader(el::render::FileSystemLoader::create(_root));
            REQUIRE_EQUAL(
                renderer->render("template.cpp"_el), "[[nodiscard]] auto value() -> int {\n    return 42;\n}\n"_el);
        });
    }

    void testCapturedApplicationAndTerminalOutput() {
        runInsideApplication([&] {
            el::stream::stdOut()->writeLine("captured standard output"_el);
            el::stream::stdErr()->writeLine("captured error output"_el);
            tool::frame::app().terminal()->write("captured terminal output"_el);
            tool::frame::app().terminal()->flush();
        });
        REQUIRE(_outputText.contains("captured standard output"_el));
        REQUIRE(_outputText.contains("captured terminal output"_el));
        REQUIRE(_errorText.contains("captured error output"_el));
    }

    void testDuplicateRegistration() {
        auto application = tool::frame::ToolApplication{TestToolApplication::coreProject()};
        application.registerTool<tool::idna::IdnaTool>();
        try {
            application.registerTool<tool::idna::IdnaTool>();
            REQUIRE(false);
        } catch (const el::LogicFailure &error) {
            REQUIRE(error.reason().contains("Invalid tool or source registration"_el));
            REQUIRE(error.reason().contains("already registered"_el));
        }
    }

    void testNoIndividualConfiguration() {
        REQUIRE_FALSE(NoConfigurationTool{}.configurationRules());
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[generate_idna_data]\noutput_file: 1\n"_el);
        auto executed = false;
        REQUIRE_EQUAL(
            runTool([&](const auto &section) {
                REQUIRE_FALSE(section);
                REQUIRE_EQUAL(&tool::frame::app(), &el::application());
                REQUIRE_EQUAL(tool::frame::app().files().projectRoot(), _root);
                executed = true;
            }),
            el::ExitCode::success().toRawValue());
        REQUIRE(executed);
    }

    void testUnexpectedSectionPreventsExecution() {
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[probe]\nunknown: 1\n"_el);
        auto executed = false;
        REQUIRE_EQUAL(runTool([&](const auto &) { executed = true; }), el::ExitCode::failure().toRawValue());
        REQUIRE_FALSE(executed);
    }

    void testOnlySelectedSectionIsValidated() {
        write(
            "tool/conf/erbsland-core-tool.elcl"_el,
            "[rendering]\n[probe]\nmessage: \"hello\"\n[generate_idna_data]\noutput_file: 1\n"_el);
        auto executed = false;
        REQUIRE_EQUAL(
            runTool<ConfiguredTool>([&](const auto &section) {
                REQUIRE(section);
                REQUIRE_EQUAL(section->getTextOrThrow(cMessageValue), "hello"_el);
                executed = true;
            }),
            el::ExitCode::success().toRawValue());
        REQUIRE(executed);
    }

    void testDefaultsAreApplied() {
        write("tool/conf/erbsland-core-tool.elcl"_el, "[rendering]\n[probe]\n"_el);
        auto executed = false;
        REQUIRE_EQUAL(
            runTool<ConfiguredTool>([&](const auto &section) {
                REQUIRE_EQUAL(section->getTextOrThrow(cMessageValue), "default probe"_el);
                executed = true;
            }),
            el::ExitCode::success().toRawValue());
        REQUIRE(executed);
    }

    void testMissingSectionPreventsExecution() {
        auto executed = false;
        REQUIRE_EQUAL(
            runTool<ConfiguredTool>([&](const auto &) { executed = true; }), el::ExitCode::failure().toRawValue());
        REQUIRE_FALSE(executed);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testInvalidSelectedSectionPreventsExecution() {
        for (
            const auto &configuration : el::StringList{
                "[rendering]\n[probe]\nmessage: 1\n"_el,
                "[rendering]\n[probe]\nunknown: 1\n"_el,
                "[rendering]\n*[probe]\nmessage: \"hello\"\n"_el}) {
            write("tool/conf/erbsland-core-tool.elcl"_el, configuration);
            auto executed = false;
            REQUIRE_EQUAL(
                runTool<ConfiguredTool>([&](const auto &) { executed = true; }), el::ExitCode::failure().toRawValue());
            REQUIRE_FALSE(executed);
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testRenderingDefaultsAndOverrides() {
        runInsideApplication([&] {
            write("headers.cpp"_el, "/*{ copyright_line }*/\n/*{ license_line }*/\n/*{ current_year }*/\n"_el);
            const auto renderer = tool::frame::app().createRenderEnvironment(tool::frame::TemplateLanguage::Cpp);
            renderer->addLayoutLoader(el::render::FileSystemLoader::create(_root));
            const auto year = el::time::DateTime::now().year().toRawValue();
            REQUIRE_EQUAL(
                renderer->render("headers.cpp"_el),
                el::StringFormat{
                    "Copyright (c) {} Tobias Erbsland - https://erbsland.dev\nSPDX-License-Identifier: Apache-2.0\n{}\n"_el}
                    .build(year, year));
            auto local = el::render::Context{};
            local.set("copyright_line"_el, "Local copyright"_el);
            local.set("current_year"_el, 2030);
            REQUIRE_EQUAL(
                renderer->render("headers.cpp"_el, local),
                "Local copyright\nSPDX-License-Identifier: Apache-2.0\n2030\n"_el);
        });
    }

    void testRenderingIncludesAndFilters() {
        write("tool/conf/erbsland-core-tool.elcl"_el, "@include: \"headers.elcl\"\n"_el);
        write(
            "tool/conf/headers.elcl"_el,
            R"([rendering]
copyright_line: "Year ${var:year|upper:ascii} ${env:PATH|trim}"
license_line: "literal \${var:year}"
)"_el);
        runInsideApplication([&] {
            write("headers.cpp"_el, "/*{ copyright_line }*/\n/*{ license_line }*/"_el);
            const auto renderer = tool::frame::app().createRenderEnvironment(tool::frame::TemplateLanguage::Cpp);
            renderer->addLayoutLoader(el::render::FileSystemLoader::create(_root));
            const auto output = renderer->render("headers.cpp"_el);
            REQUIRE(output.startsWith("Year "_el));
            REQUIRE(output.contains("literal ${var:year}"_el));
        });
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testInvalidRenderingPreventsExecution() {
        for (
            const auto &config : el::StringList{
                "[rendering]\nunknown: 1\n"_el,
                "[rendering]\nlicense_line: 1\n"_el,
                "[rendering]\nlicense_line: \"\"\n"_el,
                R"([rendering]
license_line: "line\nline"
)"_el,
                "[rendering]\nlicense_line: \"${var:missing}\"\n"_el,
                "[rendering]\nlicense_line: \"${var:year|missing}\"\n"_el}) {
            write("tool/conf/erbsland-core-tool.elcl"_el, config);
            auto executed = false;
            REQUIRE_EQUAL(runTool([&](const auto &) { executed = true; }), el::ExitCode::failure().toRawValue());
            REQUIRE_FALSE(executed);
            REQUIRE_FALSE(_errorText.isEmpty());
        }
    }
    void testRenderingSectionIsRequired() {
        write("tool/conf/erbsland-core-tool.elcl"_el, ""_el);
        auto executed = false;
        REQUIRE_EQUAL(runTool([&](const auto &) { executed = true; }), el::ExitCode::failure().toRawValue());
        REQUIRE_FALSE(executed);
        REQUIRE(_errorText.contains("requires a [rendering] section"_el));
    }
};
