// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardOptionRenderer.hpp"

#include "Option.hpp"
#include "OptionErrorContext.hpp"
#include "OptionHelp.hpp"
#include "OptionModule.hpp"
#include "Options.hpp"
#include "OptionSet.hpp"

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
    StandardOptionRenderer{std::move(output), std::move(error), OptionDisplayText::defaultText()} {
}

StandardOptionRenderer::StandardOptionRenderer(
    stream::TextOutputStreamPtr output, stream::TextOutputStreamPtr error, OptionDisplayText displayText) :
    OptionRendererBase{std::move(displayText)}, _output{std::move(output)}, _error{std::move(error)} {
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

auto StandardOptionRenderer::create(
    stream::TextOutputStreamPtr output, stream::TextOutputStreamPtr error, OptionDisplayText displayText)
    -> StandardOptionRendererPtr {
    return std::make_shared<StandardOptionRenderer>(std::move(output), std::move(error), std::move(displayText));
}

void StandardOptionRenderer::displayHelp(const OptionsPtr &options, text::StringView moduleName) {
    const auto model = impl::OptionDisplayModel{options, moduleName, displayText()};
    _output->writeLine(model.helpTitleText());
    if (const auto title = model.titleText(); !title.isEmpty() && title != model.displayName()) {
        _output->writeLine(title);
    }
    if (const auto help = model.selectedHelp(); help != nullptr) {
        writeHelpText(*help);
    }
    writeUsage(model);

    if (options != nullptr && model.module() == nullptr && !options->optionModules().empty()) {
        writeSection(displayText().modulesHeading(), model.moduleRows());
    }
    writeSection(displayText().optionsHeading(), model.optionRows());

    if (const auto help = model.selectedHelp(); help != nullptr && !help->epilog().isEmpty()) {
        _output->writeLine();
        _output->writeLine(help->epilog());
    }
    _output->flush();
}

void StandardOptionRenderer::displayVersion(const OptionsPtr &options, text::StringView) {
    if (options == nullptr) {
        _output->printLine(displayText().versionLabel(), "0.0.0"_el);
        _output->flush();
        return;
    }

    const auto &applicationInfo = options->applicationInfo();
    if (applicationInfo.applicationName().isEmpty()) {
        _output->printLine(displayText().versionLabel(), applicationInfo.applicationVersion().toString());
    } else {
        _output->printLine(applicationInfo.applicationName(), " ", applicationInfo.applicationVersion().toString());
    }
    if (!applicationInfo.authorName().isEmpty()) {
        _output->printLine(displayText().authorLabel(), applicationInfo.authorName());
    }
    if (!applicationInfo.copyrightLine().isEmpty()) {
        _output->writeLine(applicationInfo.copyrightLine());
    }
    if (!applicationInfo.licenseText().isEmpty()) {
        _output->printLine(displayText().licenseLabel(), applicationInfo.licenseText());
    }
    _output->flush();
}

void StandardOptionRenderer::displayError(const OptionsPtr &, const OptionErrorContext &errorContext) {
    if (errorContext.description().isEmpty()) {
        _error->printLine(displayText().errorLabel(), displayText().genericErrorMessage());
    } else {
        _error->printLine(displayText().errorLabel(), errorContext.description());
    }
    if (!errorContext.moduleName().isEmpty()) {
        _error->printLine(displayText().moduleLabel(), errorContext.moduleName());
    }
    if (const auto option = errorContext.option(); option != nullptr) {
        _error->printLine(displayText().optionLabel(), impl::OptionDisplayModel::optionTitle(option, displayText()));
    }
    if (!errorContext.argumentIndex().isNoIndex()) {
        _error->printLine(displayText().argumentIndexLabel(), errorContext.argumentIndex().toRawValue());
    }
    _error->flush();
}

void StandardOptionRenderer::writeUsage(const impl::OptionDisplayModel &model) {
    auto builder = text::StringBuilder{};
    builder.append(displayText().usageLabel());
    builder.append(model.executableName());
    if (model.module() != nullptr) {
        builder.append(U' ');
        builder.append(model.module()->name());
    } else if (model.options() != nullptr && !model.options()->optionModules().empty()) {
        builder.append(U' ');
        builder.append(displayText().modulePlaceholder());
    }
    builder.append(U' ');
    builder.append(displayText().optionsPlaceholder());

    if (model.module() != nullptr || model.options() == nullptr || model.options()->optionModules().empty()) {
        for (const auto &optionSet : model.visibleOptionSets()) {
            for (const auto &option : optionSet->options()) {
                if (option == nullptr || !option->isPositionalArgument() || option->isDisabled() ||
                    !impl::OptionDisplayModel::visibleHelp(option->help()) || option->names().empty()) {
                    continue;
                }
                builder.append(U' ');
                builder.append(displayText().placeholderPrefix());
                builder.append(option->names().front());
                builder.append(displayText().placeholderSuffix());
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

void StandardOptionRenderer::writeSection(text::StringView title, const std::vector<impl::OptionDisplayRow> &rows) {
    if (rows.empty()) {
        return;
    }
    _output->writeLine();
    _output->writeLine(title);
    writeRows(rows, "  "_el);
}

void StandardOptionRenderer::writeRows(const std::vector<impl::OptionDisplayRow> &rows, text::StringView indent) {
    for (const auto &row : rows) {
        writeRow(row, indent);
        if (!row.details.empty()) {
            writeDetailRows(row.details, "    "_el);
        }
    }
}

void StandardOptionRenderer::writeRow(const impl::OptionDisplayRow &row, text::StringView indent) {
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

void StandardOptionRenderer::writeDetailRows(const std::vector<impl::OptionDisplayRow> &rows, text::StringView indent) {
    for (const auto &row : rows) {
        writeRow(row, indent);
    }
}

}
