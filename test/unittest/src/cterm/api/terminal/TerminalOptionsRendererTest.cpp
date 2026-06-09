// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/TerminalTestBackend.hpp"

#include <erbsland/core/ApplicationInfo.hpp>
#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/cterm/TerminalOptionsRenderer.hpp>
#include <erbsland/options/Option.hpp>
#include <erbsland/options/OptionChoice.hpp>
#include <erbsland/options/OptionChoices.hpp>
#include <erbsland/options/OptionDisplayText.hpp>
#include <erbsland/options/OptionErrorContext.hpp>
#include <erbsland/options/OptionHelp.hpp>
#include <erbsland/options/OptionHelpVisibility.hpp>
#include <erbsland/options/OptionModule.hpp>
#include <erbsland/options/Options.hpp>
#include <erbsland/options/OptionSet.hpp>
#include <erbsland/options/OptionType.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unit/Version.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <memory>
#include <string>

using namespace el::cterm;
using namespace el::options;
using namespace el::text::literals;

TESTED_TARGETS(OptionDisplayText OptionRendererBase TerminalOptionsRenderer TerminalOptionsTheme)
class TerminalOptionsRendererTest final : public el::UnitTest {
public:
    void testDefaultThemeIsStable() {
        const auto &first = TerminalOptionsTheme::defaultTheme();
        const auto &second = TerminalOptionsTheme::defaultTheme();

        REQUIRE_EQUAL(&first, &second);
        REQUIRE(first.heading() == second.heading());
        REQUIRE(first.error() == second.error());
    }

    void testHelpContainsStandardContent() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        const auto terminal = std::make_shared<Terminal>(backend, bgeo::BlockSize{60, 25});
        auto renderer = TerminalOptionsRenderer{terminal};

        renderer.displayHelp(makeOptions(), {});
        const auto text = backend->output();

        requireContains(text, "Demo Tool");
        requireContains(text, "1.2.3 Help");
        requireContains(text, "Operate demo files.");
        requireContains(text, "Usage:");
        requireContains(text, "demo-tool");
        requireContains(text, "Modules");
        requireContains(text, "run");
        requireContains(text, "Options");
        requireContains(text, "-h, --help");
        requireContains(text, "--version");
        requireContains(text, "-v, --verbose");
        requireContains(text, "--mode <choice>");
        requireContains(text, "Choices: fast, safe.");
        requireMissing(text, "debug");
        requireMissing(text, "expert");
    }

    void testCustomThemeIsUsed() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        backend->_supportsColorCodes = false;
        const auto terminal = std::make_shared<Terminal>(backend, bgeo::BlockSize{60, 25});
        auto theme = TerminalOptionsTheme::defaultTheme();
        theme.setHeading(BlockStyle{fg::Red});
        auto renderer = TerminalOptionsRenderer{terminal, theme};

        renderer.displayHelp(makeOptions(), {});

        auto foundHeadingColor = false;
        for (const auto &color : backend->_emittedColors) {
            if (color == Color{fg::Red, bg::Default}) {
                foundHeadingColor = true;
            }
        }
        REQUIRE(foundHeadingColor);
    }

    void testCustomDisplayTextIsUsed() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        const auto terminal = std::make_shared<Terminal>(backend, bgeo::BlockSize{60, 25});
        auto displayText = OptionDisplayText::defaultText();
        displayText.setUsageLabel("Use: "_el);
        displayText.setModulePlaceholder("<action>"_el);
        displayText.setOptionsPlaceholder("[flags]"_el);
        displayText.setOptionsHeading("Flags"_el);
        displayText.setHelpOptionDescription("Show assistance."_el);
        displayText.setVersionOptionDescription("Show release information."_el);
        displayText.setChoicePlaceholder("<variant>"_el);
        auto renderer = TerminalOptionsRenderer{terminal, TerminalOptionsTheme::defaultTheme(), displayText};

        renderer.displayHelp(makeOptions(), {});
        const auto text = backend->output();

        requireContains(text, "Use:");
        requireContains(text, "<action>");
        requireContains(text, "[flags]");
        requireContains(text, "Flags");
        requireContains(text, "Show assistance.");
        requireContains(text, "Show release information.");
        requireContains(text, "--mode <variant>");
    }

    void testVersionAndErrorOutput() {
        const auto backend = std::make_shared<TerminalTestBackend>();
        const auto terminal = std::make_shared<Terminal>(backend, bgeo::BlockSize{60, 25});
        auto renderer = TerminalOptionsRenderer{terminal};
        const auto options = makeOptions();

        renderer.displayVersion(options, {});
        auto text = backend->output();
        requireContains(text, "Demo Tool");
        requireContains(text, "1.2.3");
        requireContains(text, "Author:");

        backend->clearOutput();
        const auto option = Option::create({"--name"_el});
        option->setType(OptionType::Text);
        auto context = OptionErrorContext{};
        context.setDescription("Invalid option value"_el)
            .setModuleName("run"_el)
            .setOption(option)
            .setArgumentIndex(el::unit::ArgumentIndex{3U});

        renderer.displayError({}, context);
        text = backend->output();

        requireContains(text, "Error:");
        requireContains(text, "Invalid option value");
        requireContains(text, "Module:");
        requireContains(text, "run");
        requireContains(text, "Option:");
        requireContains(text, "--name <value>");
        requireContains(text, "Argument index:");
        requireContains(text, "3");
    }

private:
    [[nodiscard]] static auto makeOptions() -> OptionsPtr {
        auto options = Options::create();
        auto applicationInfo = el::core::ApplicationInfo{};
        applicationInfo.setApplicationName("Demo Tool"_el);
        applicationInfo.setApplicationVersion(el::unit::Version{1, 2, 3, 4});
        applicationInfo.setAuthorName("Tobias Erbsland"_el);
        applicationInfo.setCopyrightLine("Copyright 2026"_el);
        applicationInfo.setLicenseText("Apache-2.0"_el);
        options->setApplicationInfo(applicationInfo);
        options->setExecutablePath("/usr/bin/demo-tool.exe"_el);

        auto rootHelp = OptionHelp{"Operate demo files."_el};
        rootHelp.setTitle("Demo Tool"_el);
        options->setHelp(rootHelp);

        options->addOption({"-v"_el, "--verbose"_el, "verbose-alias"_el}).setHelp("Enable verbose output."_el);
        auto hiddenHelp = OptionHelp{"Hidden debug mode."_el};
        hiddenHelp.setVisibility(OptionHelpVisibility::Hidden);
        options->addOption("--debug"_el).setHelp(hiddenHelp);

        auto choices = OptionChoices::create();
        choices->addChoice(OptionChoice::create("fast"_el, OptionHelp{"Prefer speed."_el}));
        choices->addChoice(OptionChoice::create("safe"_el, OptionHelp{"Prefer safety."_el}));
        auto expertHelp = OptionHelp{"Expert mode."_el};
        expertHelp.setVisibility(OptionHelpVisibility::Detail);
        choices->addChoice(OptionChoice::create("expert"_el, expertHelp));
        options->addOption("--mode"_el).setHelp("Select mode."_el).setChoices(choices);

        auto module = OptionModule::create("run"_el);
        auto moduleHelp = OptionHelp{"Run module."_el};
        moduleHelp.setTitle("Run Commands"_el);
        module->setHelp(moduleHelp);
        module->addOption("--force"_el).setHelp("Force execution."_el);
        options->addModule(module);

        return options;
    }

    void requireContains(const std::string &text, const std::string &needle) {
        REQUIRE(text.find(needle) != std::string::npos);
    }

    void requireMissing(const std::string &text, const std::string &needle) {
        REQUIRE(text.find(needle) == std::string::npos);
    }
};
