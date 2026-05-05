// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardOptionRenderer.hpp"

#include "Option.hpp"
#include "OptionChoice.hpp"
#include "OptionChoices.hpp"
#include "OptionErrorContext.hpp"
#include "OptionHelp.hpp"
#include "OptionModule.hpp"
#include "Options.hpp"
#include "OptionSet.hpp"
#include "OptionType.hpp"

#include "../stream/StandardStreams.hpp"
#include "../stream/TextOutputStream.hpp"
#include "../text/Literals.hpp"
#include "../text/StringBuilder.hpp"

#include <memory>
#include <utility>

namespace erbsland::options {

using namespace text::literals;

namespace {

constexpr auto cDescriptionColumn = std::size_t{26U};

}

StandardOptionRenderer::StandardOptionRenderer() : StandardOptionRenderer{stream::stdOut(), stream::stdErr()} {
}

StandardOptionRenderer::StandardOptionRenderer(stream::TextOutputStreamPtr output, stream::TextOutputStreamPtr error) :
    _output{std::move(output)}, _error{std::move(error)} {
    if (_output == nullptr) {
        _output = stream::stdOut();
    }
    if (_error == nullptr) {
        _error = stream::stdErr();
    }
}

auto StandardOptionRenderer::create() -> StandardOptionRendererPtr {
    return std::make_shared<StandardOptionRenderer>();
}

auto StandardOptionRenderer::create(stream::TextOutputStreamPtr output, stream::TextOutputStreamPtr error)
    -> StandardOptionRendererPtr {
    return std::make_shared<StandardOptionRenderer>(std::move(output), std::move(error));
}

void StandardOptionRenderer::displayHelp(const OptionsPtr &options, text::StringView moduleName) {
    const auto module = findModule(options, moduleName);
    if (const auto title = titleText(options, module); !title.isEmpty()) {
        _output->writeLine(title);
    }
    if (const auto help = selectedHelp(options, module); help != nullptr) {
        writeHelpText(*help);
    }
    writeUsage(options, module);

    if (options != nullptr && module == nullptr && !options->optionModules().empty()) {
        writeSection("Modules"_el, moduleRows(options));
    }
    writeSection("Options"_el, optionRows(visibleOptionSets(options, module)));

    if (const auto help = selectedHelp(options, module); help != nullptr && !help->epilog().isEmpty()) {
        _output->writeLine();
        _output->writeLine(help->epilog());
    }
    _output->flush();
}

void StandardOptionRenderer::displayVersion(const OptionsPtr &options, text::StringView) {
    if (options == nullptr) {
        _output->writeLine("Version 0.0.0"_el);
        _output->flush();
        return;
    }

    const auto &displayInfo = options->displayInfo();
    if (displayInfo.applicationName().isEmpty()) {
        _output->printLine("Version ", displayInfo.applicationVersion().toString());
    } else {
        _output->printLine(displayInfo.applicationName(), " ", displayInfo.applicationVersion().toString());
    }
    if (!displayInfo.authorName().isEmpty()) {
        _output->printLine("Author: ", displayInfo.authorName());
    }
    if (!displayInfo.copyrightLine().isEmpty()) {
        _output->writeLine(displayInfo.copyrightLine());
    }
    if (!displayInfo.licenseText().isEmpty()) {
        _output->printLine("License: ", displayInfo.licenseText());
    }
    _output->flush();
}

void StandardOptionRenderer::displayError(const OptionsPtr &, const OptionErrorContext &errorContext) {
    if (errorContext.description().isEmpty()) {
        _error->writeLine("Error: Option processing failed"_el);
    } else {
        _error->printLine("Error: ", errorContext.description());
    }
    if (!errorContext.moduleName().isEmpty()) {
        _error->printLine("Module: ", errorContext.moduleName());
    }
    if (const auto option = errorContext.option(); option != nullptr) {
        _error->printLine("Option: ", optionTitle(option));
    }
    if (!errorContext.argumentIndex().isNoIndex()) {
        _error->printLine("Argument index: ", errorContext.argumentIndex().toRawValue());
    }
    _error->flush();
}

auto StandardOptionRenderer::findModule(const OptionsPtr &options, const text::StringView &moduleName) const
    -> OptionModulePtr {
    if (options == nullptr || moduleName.isEmpty()) {
        return {};
    }
    for (const auto &module : options->optionModules()) {
        if (module != nullptr && module->hasName(moduleName)) {
            return module;
        }
    }
    return {};
}

auto StandardOptionRenderer::visibleOptionSets(const OptionsPtr &options, const OptionModulePtr &module) const
    -> std::vector<OptionSetPtr> {
    auto result = std::vector<OptionSetPtr>{};
    if (options == nullptr) {
        return result;
    }
    const auto addVisibleSet = [this, &result](const OptionSetPtr &optionSet) -> void {
        if (visibleOptionSet(optionSet)) {
            result.emplace_back(optionSet);
        }
    };
    for (const auto &optionSet : options->optionSets()) {
        addVisibleSet(optionSet);
    }
    if (module != nullptr) {
        for (const auto &optionSet : module->optionSets()) {
            addVisibleSet(optionSet);
        }
    }
    return result;
}

auto StandardOptionRenderer::optionRows(const std::vector<OptionSetPtr> &optionSets) const -> std::vector<TextRow> {
    auto rows = std::vector<TextRow>{};
    rows.emplace_back(TextRow{"-h, --help"_els, "Display this help."_els, {}});
    rows.emplace_back(TextRow{"--version"_els, "Display version information."_els, {}});
    for (const auto &optionSet : optionSets) {
        for (const auto &option : optionSet->options()) {
            if (option == nullptr || option->isDisabled() || !visibleHelp(option->help())) {
                continue;
            }
            rows.emplace_back(TextRow{optionTitle(option), optionDescription(option), choiceRows(option)});
        }
    }
    return rows;
}

auto StandardOptionRenderer::moduleRows(const OptionsPtr &options) const -> std::vector<TextRow> {
    auto rows = std::vector<TextRow>{};
    if (options == nullptr) {
        return rows;
    }
    for (const auto &module : options->optionModules()) {
        if (module == nullptr || !visibleHelp(module->help())) {
            continue;
        }
        auto description = text::String{};
        if (!module->help().description().isEmpty()) {
            description = text::String{module->help().description()};
        } else if (!module->help().title().isEmpty()) {
            description = text::String{module->help().title()};
        }
        rows.emplace_back(TextRow{text::String{module->name()}, description, {}});
    }
    return rows;
}

auto StandardOptionRenderer::choiceRows(const OptionPtr &option) const -> std::vector<TextRow> {
    auto rows = std::vector<TextRow>{};
    if (option == nullptr || option->choices() == nullptr) {
        return rows;
    }
    for (const auto &choice : option->choices()->choices()) {
        if (choice == nullptr || !visibleHelp(choice->help())) {
            continue;
        }
        auto description = text::String{};
        if (!choice->help().description().isEmpty()) {
            description = text::String{choice->help().description()};
        } else if (!choice->help().title().isEmpty()) {
            description = text::String{choice->help().title()};
        }
        if (!description.isEmpty()) {
            rows.emplace_back(TextRow{text::String{choice->text()}, description, {}});
        }
    }
    return rows;
}

auto StandardOptionRenderer::optionTitle(const OptionPtr &option) const -> text::String {
    auto builder = text::StringBuilder{};
    auto first = true;
    if (option->isRegularOption()) {
        for (const auto &name : option->names()) {
            if (!Option::isOptionName(name)) {
                continue;
            }
            if (!first) {
                builder.append(", "_el);
            }
            builder.append(name);
            first = false;
        }
        if (option->type() == OptionType::Integer) {
            builder.append(" <integer>"_el);
        } else if (option->type() == OptionType::Text) {
            builder.append(" <value>"_el);
        } else if (option->type() == OptionType::Choice) {
            builder.append(" <choice>"_el);
        }
        return builder.toString();
    }
    if (!option->names().empty()) {
        builder.append(U'<');
        builder.append(option->names().front());
        builder.append(U'>');
    } else {
        builder.append("<value>"_el);
    }
    return builder.toString();
}

auto StandardOptionRenderer::optionDescription(const OptionPtr &option) const -> text::String {
    auto builder = text::StringBuilder{};
    if (!option->help().description().isEmpty()) {
        builder.append(option->help().description());
    } else if (!option->help().title().isEmpty()) {
        builder.append(option->help().title());
    }
    if (option->type() == OptionType::Choice && option->choices() != nullptr) {
        auto first = true;
        for (const auto &choice : option->choices()->choices()) {
            if (choice == nullptr || !visibleHelp(choice->help())) {
                continue;
            }
            if (first) {
                if (!builder.isEmpty()) {
                    builder.append(U' ');
                }
                builder.append("Choices: "_el);
            } else {
                builder.append(", "_el);
            }
            builder.append(choice->text());
            first = false;
        }
        if (!first) {
            builder.append(U'.');
        }
    }
    return builder.toString();
}

auto StandardOptionRenderer::visibleHelp(const OptionHelp &help) const noexcept -> bool {
    return help.visibility() != OptionHelpVisibility::Hidden && help.visibility() != OptionHelpVisibility::Detail;
}

auto StandardOptionRenderer::visibleOptionSet(const OptionSetPtr &optionSet) const noexcept -> bool {
    return optionSet != nullptr && !optionSet->flags().isSet(OptionFlag::Disabled) && visibleHelp(optionSet->help());
}

auto StandardOptionRenderer::displayName(const OptionsPtr &options) const -> text::StringView {
    static const auto fallback = "application"_els;
    if (options == nullptr || options->displayInfo().applicationName().isEmpty()) {
        return fallback;
    }
    return options->displayInfo().applicationName();
}

auto StandardOptionRenderer::titleText(const OptionsPtr &options, const OptionModulePtr &module) const
    -> text::StringView {
    if (module != nullptr && !module->help().title().isEmpty()) {
        return module->help().title();
    }
    if (options != nullptr && !options->help().title().isEmpty()) {
        return options->help().title();
    }
    if (options != nullptr && !options->displayInfo().applicationName().isEmpty()) {
        return options->displayInfo().applicationName();
    }
    return {};
}

auto StandardOptionRenderer::selectedHelp(const OptionsPtr &options, const OptionModulePtr &module) const
    -> const OptionHelp * {
    if (module != nullptr) {
        return &module->help();
    }
    if (options != nullptr) {
        return &options->help();
    }
    return nullptr;
}

void StandardOptionRenderer::writeUsage(const OptionsPtr &options, const OptionModulePtr &module) {
    auto builder = text::StringBuilder{};
    builder.append("Usage: "_el);
    builder.append(displayName(options));
    if (module != nullptr) {
        builder.append(U' ');
        builder.append(module->name());
    } else if (options != nullptr && !options->optionModules().empty()) {
        builder.append(" <module>"_el);
    }
    builder.append(" [options]"_el);

    if (module != nullptr || options == nullptr || options->optionModules().empty()) {
        for (const auto &optionSet : visibleOptionSets(options, module)) {
            for (const auto &option : optionSet->options()) {
                if (option == nullptr || !option->isPositionalArgument() || option->isDisabled() ||
                    !visibleHelp(option->help()) || option->names().empty()) {
                    continue;
                }
                builder.append(U' ');
                builder.append(U'<');
                builder.append(option->names().front());
                builder.append(U'>');
            }
        }
    }
    _output->writeLine(builder.toString());
}

void StandardOptionRenderer::writeHelpText(const OptionHelp &help) {
    if (!help.description().isEmpty()) {
        _output->writeLine(help.description());
    }
}

void StandardOptionRenderer::writeSection(text::StringView title, const std::vector<TextRow> &rows) {
    if (rows.empty()) {
        return;
    }
    _output->writeLine();
    _output->writeLine(title);
    writeRows(rows, "  "_el);
}

void StandardOptionRenderer::writeRows(const std::vector<TextRow> &rows, text::StringView indent) {
    for (const auto &row : rows) {
        writeRow(row, indent);
        if (!row.details.empty()) {
            writeDetailRows(row.details, "    "_el);
        }
    }
}

void StandardOptionRenderer::writeRow(const TextRow &row, text::StringView indent) {
    _output->write(indent);
    _output->write(row.title);
    if (!row.description.isEmpty()) {
        const auto usedColumns = indent.length().toSizeT() + row.title.length().toSizeT();
        const auto padding = usedColumns < cDescriptionColumn ? cDescriptionColumn - usedColumns : std::size_t{2U};
        for (auto index = std::size_t{0U}; index < padding; ++index) {
            _output->write(text::Char{U' '});
        }
        _output->write(row.description);
    }
    _output->writeLine();
}

void StandardOptionRenderer::writeDetailRows(const std::vector<TextRow> &rows, text::StringView indent) {
    for (const auto &row : rows) {
        writeRow(row, indent);
    }
}

}
