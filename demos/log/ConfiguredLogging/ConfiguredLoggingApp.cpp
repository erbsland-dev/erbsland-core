// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConfiguredLoggingApp.hpp"

#include <erbsland/conf/all.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/log/all.hpp>

#include <utility>

namespace demo {

/// Load and install an ELCL logging configuration as part of application startup.
///
/// The configuration can occupy the document root or a branch selected on the command line. Validation and parsing
/// errors intentionally reach `Application::run()`, which presents the complete diagnostic and returns a failure exit
/// code.
void ConfiguredLoggingApp::initialize() {
    info().setApplicationName("Configured Explorer Guild"_el);
    enableTerminal();
}

void ConfiguredLoggingApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpDescription("Load an application logging configuration from an ELCL file."_el);
    options->addOption("configuration"_el)
        .setRequired()
        .setHelpDescription("ELCL file containing the logging configuration."_el);
    options->addOption({"-b"_el, "--branch"_el, "branch"_el})
        .setType(el::OptionType::Text)
        .setHelpDescription("Optional dot-separated branch containing the logging configuration."_el);
}

void ConfiguredLoggingApp::parseCommandLine() {
    Application::parseCommandLine();
    if (optionValues() == nullptr) {
        return;
    }

    // Parse the application configuration file and select the requested branch.
    const auto path = el::Path::fromNativeOrThrow(optionValues()->getText("configuration"_el));
    const auto document = el::conf::Parser{}.parseFileOrThrow(path);
    auto branch = el::conf::ValuePtr{document};
    const auto branchName = optionValues()->getText("branch"_el);
    if (!branchName.isEmpty()) {
        branch = document->valueOrThrow(branchName);
    }

    // Pre-validation is useful when these rules are composed into broader application validation.
    el::LogConfigurationParser::validationRules()->validate(branch, 0);

    // A terminal is supplied because the selected configuration may create console writers.
    auto configuration = el::LogConfigurationParser{terminal()}.parse(branch);
    log().setConfiguration(std::move(configuration));
}

auto ConfiguredLoggingApp::main() -> el::ExitCode {
    const auto log = this->log().createStream("guild/configured"_el, el::LogTraceSection{"route-search"_el});
    if (log->traceEnabled()) {
        log->trace("Candidate route: Turku → Jääjärvi → Majakka"_el);
    }
    log->info("The explorer guild loaded its logging configuration."_el);
    log->warn("One route marker still needs confirmation."_el);
    return el::ExitCode::success();
}

}
