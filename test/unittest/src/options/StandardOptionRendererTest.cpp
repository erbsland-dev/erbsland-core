// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/ApplicationInfo.hpp>
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
#include <erbsland/options/StandardOptionRenderer.hpp>
#include <erbsland/stream/StringBuilderStream.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unit/Version.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <string>

using namespace el::options;
using namespace el::text::literals;

TESTED_TARGETS(ApplicationInfo OptionDisplayText OptionRendererBase StandardOptionRenderer)
class StandardOptionRendererTest final : public el::UnitTest {
public:
    void testRootHelp() {
        const auto options = makeOptions();
        const auto output = el::stream::StringBuilderStream::create();
        const auto error = el::stream::StringBuilderStream::create();
        auto renderer = StandardOptionRenderer{output, error};

        renderer.displayHelp(options, {});
        const auto text = toStdString(output);

        REQUIRE(text.starts_with("► Demo Tool 1.2.3 Help\n"));
        requireContains(text, "Operate demo files.\n");
        requireContains(text, "Usage: demo-tool <module> [options]\n");
        requireContains(text, "Modules\n");
        requireContains(text, "run");
        requireContains(text, "Run module.");
        requireContains(text, "Options\n");
        requireContains(text, "-h, --help");
        requireContains(text, "--version");
        requireContains(text, "-v, --verbose");
        requireContains(text, "Enable verbose output.");
        requireContains(text, "--mode <choice>");
        requireContains(text, "Choices: fast, safe.");
        requireContains(text, "Prefer speed.");
        requireContains(text, "Prefer safety.");
        requireMissing(text, "debug");
        requireMissing(text, "expert");
        requireMissing(text, "verbose-alias");
        REQUIRE(error->isEmpty());
    }

    void testModuleHelpIncludesRootAndModuleOptions() {
        const auto options = makeOptions();
        const auto output = el::stream::StringBuilderStream::create();
        const auto error = el::stream::StringBuilderStream::create();
        auto renderer = StandardOptionRenderer{output, error};

        renderer.displayHelp(options, "RUN"_el);
        const auto text = toStdString(output);

        requireContains(text, "Run Commands\n");
        requireContains(text, "Run module.\n");
        requireContains(text, "Usage: demo-tool run [options]\n");
        requireContains(text, "-v, --verbose");
        requireContains(text, "--force");
        requireMissing(text, "Modules\n");
        REQUIRE(error->isEmpty());
    }

    void testVersionOutput() {
        const auto options = makeOptions();
        const auto output = el::stream::StringBuilderStream::create();
        const auto error = el::stream::StringBuilderStream::create();
        auto renderer = StandardOptionRenderer{output, error};

        renderer.displayVersion(options, {});

        REQUIRE_EQUAL(
            toStdString(output),
            std::string{"Demo Tool 1.2.3\nAuthor: Tobias Erbsland\nCopyright 2026\nLicense: Apache-2.0\n"});
        REQUIRE(error->isEmpty());
    }

    void testCustomDisplayText() {
        const auto options = makeOptions();
        const auto output = el::stream::StringBuilderStream::create();
        const auto error = el::stream::StringBuilderStream::create();
        auto displayText = OptionDisplayText::defaultText();
        displayText.setUsageLabel("Use: "_el);
        displayText.setModulePlaceholder("<action>"_el);
        displayText.setOptionsPlaceholder("[flags]"_el);
        displayText.setOptionsHeading("Flags"_el);
        displayText.setHelpOptionDescription("Show assistance."_el);
        displayText.setVersionOptionDescription("Show release information."_el);
        displayText.setChoicePlaceholder("<variant>"_el);
        auto renderer = StandardOptionRenderer{output, error, displayText};

        renderer.displayHelp(options, {});
        const auto text = toStdString(output);

        requireContains(text, "Use: demo-tool <action> [flags]\n");
        requireContains(text, "Flags\n");
        requireContains(text, "-h, --help");
        requireContains(text, "Show assistance.");
        requireContains(text, "--version");
        requireContains(text, "Show release information.");
        requireContains(text, "--mode <variant>");
        REQUIRE(error->isEmpty());
    }

    void testErrorOutput() {
        const auto output = el::stream::StringBuilderStream::create();
        const auto error = el::stream::StringBuilderStream::create();
        auto renderer = StandardOptionRenderer{output, error};
        const auto option = Option::create({"--name"_el});
        option->setType(OptionType::Text);
        auto context = OptionErrorContext{};
        context.setDescription("Invalid option value"_el)
            .setModuleName("run"_el)
            .setOption(option)
            .setArgumentIndex(el::unit::ArgumentIndex{3U});

        renderer.displayError({}, context);

        REQUIRE(output->isEmpty());
        REQUIRE_EQUAL(
            toStdString(error),
            std::string{"Error: Invalid option value\nModule: run\nOption: --name <value>\nArgument index: 3\n"});
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

    [[nodiscard]] static auto toStdString(const el::stream::StringBuilderStreamPtr &stream) -> std::string {
        return el::text::StringConverter{stream->toU8String()}.toStdString();
    }

    void requireContains(const std::string &text, const std::string &needle) {
        REQUIRE(text.find(needle) != std::string::npos);
    }

    void requireMissing(const std::string &text, const std::string &needle) {
        REQUIRE(text.find(needle) == std::string::npos);
    }
};
