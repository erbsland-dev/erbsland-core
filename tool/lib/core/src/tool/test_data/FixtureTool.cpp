// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FixtureTool.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/StringFormat.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::test_data {

using namespace el::text::literals;

auto FixtureTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({"-o"_el, "--output-directory"_el, "output-directory"_el})
        .setType(el::OptionType::Text)
        .setValueName("directory"_el)
        .setHelpTitle("Choose the fixture directory"_el)
        .setHelpDescription("Override output_directory with a path inside the project."_el);
    module->addOption({"-c"_el, "--check"_el, "check"_el})
        .setHelpTitle("Check generated fixtures"_el)
        .setHelpDescription("Report differences and fail without writing files or creating output directories."_el);
    return module;
}

auto FixtureTool::configurationRules() const -> el::conf::vr::RulesPtr {
    using namespace el::conf::vr::builder;
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Test fixture generation"_el}, Description{"Output settings for generated test fixtures."_el});
    rules.addRule(
        cOutputDirectoryValue,
        el::conf::vr::RuleType::Text,
        Title{"Fixture Output Directory"_el},
        Description{"Project-contained directory receiving the generated test fixtures."_el},
        Default{defaultDirectory()},
        Minimum{1});
    return rules.takeRules();
}

void FixtureTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    auto &files = frame::app().files();
    _outputOption = values->value("output-directory"_el);
    _outputConfig = configuration->valueOrThrow(cOutputDirectoryValue);
    if (_outputOption) {
        _outputDirectory = files.checked(_outputOption, true);
    } else {
        _outputDirectory = files.checked(_outputConfig, true);
    }
    if (_outputDirectory.info().exists() && !_outputDirectory.info().isDirectory()) {
        throwOutputError("The output path must be a directory."_el);
    }
    _checkOnly = values->getFlag("check"_el);
    _outputs.clear();
    _renderer = frame::app().createRenderEnvironment(frame::TemplateLanguage::Text);
}

void FixtureTool::validateOutputs(const el::StringList &names) const {
    for (const auto &name : names) {
        try {
            const auto path = frame::app().files().checked(_outputDirectory / name, true);
            if (path.info().exists() && !path.info().isRegularFile()) {
                throwOutputError(
                    el::StringFormat{"Fixture destination '{}' is occupied by a non-regular file."_el}.build(path));
            }
        } catch (const el::ApplicationError &error) {
            throwOutputError(error.description(), std::current_exception());
        }
    }
}

auto FixtureTool::renderRows(const el::String &schema, const el::render::ValueList &rows) const -> el::String {
    auto context = el::render::Context{};
    context.set("command"_el, identifier().replacedAll("_"_el, "-"_el));
    context.set("schema"_el, schema);
    context.set("rows"_el, rows);
    return _renderer->render("test_data/table.tpl"_el, context);
}

void FixtureTool::publish() const {
    auto changed = false;
    auto &files = frame::app().files();
    for (const auto &[name, text] : _outputs) {
        const auto path = _outputDirectory / name;
        auto differs = true;
        if (!_checkOnly) {
            files.createDirectories(path.parent());
        }
        if (path.parent().info().isDirectory()) {
            differs = files.updateText(path, text, _checkOnly);
        }
        changed = changed || differs;
        auto status = "unchanged"_el;
        if (differs) {
            status = "updated"_el;
            if (_checkOnly) {
                status = "differs"_el;
            }
        }
        log()->info(path, ": "_el, status);
    }
    if (_checkOnly && changed) {
        throw el::ApplicationError{
            "Generated Fixtures Differ"_el, "Run the same command without --check to update the fixtures."_el};
    }
}

void FixtureTool::throwOutputError(const el::String &description, const std::exception_ptr &cause) const {
    if (_outputOption) {
        throw el::OptionError{"Invalid Fixture Output Directory"_el, description, _outputOption, cause};
    }
    throw el::conf::ConfError{_outputConfig, "Invalid Fixture Output Directory"_el, description, cause};
}

}
