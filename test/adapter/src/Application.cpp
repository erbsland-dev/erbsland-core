// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Application.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/Parser.hpp>

using namespace el::text::literals;

void Application::initialize() {
    info().setApplicationName("Erbsland Core ELCL Test Adapter"_el);
    info().setApplicationVersion(el::Version{1, 0, 0});
}

void Application::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpTitle("Erbsland Core ELCL Test Adapter"_el);
    options->setHelpDescription("Runs one official ELCL conformance test file."_el);
    options->setParserFlag(el::OptionParserFlag::DisableVersion);
    options->addOption({"--version"_el, "language-version"_el})
        .setType(el::OptionType::Text)
        .setDefaultValue("1.0"_el)
        .setValueName("language-version"_el)
        .setHelpDescription("ELCL language version used to parse the test file."_el);
    options->addOption("configuration-file"_el)
        .setType(el::OptionType::Text)
        .setRequired()
        .setHelpDescription("ELCL configuration file to parse."_el);
}

auto Application::main() -> el::ExitCode {
    const auto languageVersion = optionValues()->getText("language-version"_el);
    if (languageVersion != "1.0"_el) {
        el::io::printErrorLine("Error: Unsupported language version: "_el, languageVersion);
        return el::ExitCode{2};
    }

    const auto configurationFile = el::Path{optionValues()->getText("configuration-file"_el)};
    if (!configurationFile.info().isRegularFile()) {
        el::io::printErrorLine("Error: Configuration file does not exist: "_el, configurationFile.toString());
        return el::ExitCode{2};
    }

    try {
        const auto document = el::conf::Parser{}.parseOrThrow(el::conf::Source::fromFile(configurationFile));
        for (const auto &[namePath, value] : document->toFlatValueMap()) {
            el::io::printLine(namePath.toText(), " = "_el, value->toTestText());
        }
    } catch (const el::conf::ConfError &error) {
        el::io::printLine("FAIL = "_el, error.category().toText(), "("_el, error.toString(), ")"_el);
        return el::ExitCode::failure();
    } catch (const el::Exception &error) {
        el::io::printErrorLine("Unexpected adapter error: "_el, error.toString());
        return el::ExitCode{2};
    }
    return el::ExitCode::success();
}
