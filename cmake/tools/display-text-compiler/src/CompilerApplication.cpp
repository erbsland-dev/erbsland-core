// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CompilerApplication.hpp"

#include "DisplayTextCompiler.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OptionEditor.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringSide.hpp>

namespace erbsland::i18n::compiler {

using namespace options;
using namespace text::literals;

void CompilerApplication::registerCommandLineOptions(const OptionsPtr &options) {
    options->setHelpTitle("Erbsland Core Display Text Compiler"_el);
    options->addOption({"--input"_el, cInputOption})
        .setType(OptionType::Text)
        .setRequired()
        .setHelpTitle("Read display-text definitions"_el)
        .setHelpDescription("Read the display-text definitions from the required ELCL input file."_el);
    options->addOption({"--header-output"_el, cHeaderOutputOption})
        .setType(OptionType::Text)
        .setHelpTitle("Choose the generated header file"_el)
        .setHelpDescription(
            "Write the generated header to this path. Defaults to the input path with a .hpp suffix."_el);
    options->addOption({"--source-output"_el, cSourceOutputOption})
        .setType(OptionType::Text)
        .setHelpTitle("Choose the generated source file"_el)
        .setHelpDescription(
            "Write the generated source to this path. Defaults to the input path with a .cpp suffix."_el);
    options->addOption({"--class"_el, cClassOption})
        .setType(OptionType::Text)
        .setHelpTitle("Choose the generated class name"_el)
        .setHelpDescription("Use this C++ class name. Defaults to the input filename without its suffix."_el);
    options->addOption({"--namespace"_el, cNamespaceOption})
        .setType(OptionType::Text)
        .setHelpTitle("Choose the generated namespace"_el)
        .setHelpDescription("Place the generated class in this C++ namespace. Defaults to the global namespace."_el);
}

auto CompilerApplication::main() -> unit::ExitCode {
    const auto values = optionValues();
    const auto input = path::Path{values->getText(cInputOption)};
    const auto inputName = input.name();
    const auto stem = inputName.slice(
        text::StringSide::Front,
        unit::ByteIndex::fromSizeT(inputName.length().toSizeT() - input.suffix().length().toSizeT()));
    auto className = values->getText(cClassOption);
    if (className.isEmpty()) {
        className = stem;
    }
    auto compiler = DisplayTextCompiler{className, values->getText(cNamespaceOption)};
    auto header = path::Path{values->getText(cHeaderOutputOption)};
    auto source = path::Path{values->getText(cSourceOutputOption)};
    if (header.isEmpty()) {
        header = input.parent() / el::join(stem, ".hpp"_el);
    }
    if (source.isEmpty()) {
        source = input.parent() / el::join(stem, ".cpp"_el);
    }
    compiler.compile(input, header, source);
    return unit::ExitCode::success();
}

}
