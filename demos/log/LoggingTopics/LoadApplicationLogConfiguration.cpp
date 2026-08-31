// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/all.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/log/all.hpp>

#include <utility>

namespace demo {

/// Load logging from the fixed `Log` section of `application.elcl`.
///
/// The application owns the section name as part of its configuration schema. Select that known section, validate it as
/// a logging configuration, and install the parsed snapshot before ordinary application work begins.
/// @notest{Compiled and executed by the logging topics demo.}
void loadApplicationLogConfiguration() {
    const auto path = el::Path{"demos/log/LoggingTopics/data/application.elcl"_el};
    const auto document = el::conf::Parser{}.parseFileOrThrow(path);
    const auto logSection = document->valueOrThrow("log"_el);

    // You can validate the Log section independently from configuration parsing, in case you like to keep
    // the whole application configuration validation in one place and fail early.
    el::LogConfigurationParser::validationRules()->validate(logSection, el::LogConfigurationParser::version());

    // Calling `parse` will nevertheless validate the configuration again.
    auto configuration = el::LogConfigurationParser{el::application().terminal()}.parse(logSection);
    el::application().log().setConfiguration(std::move(configuration));

    const auto log = el::application().log().createStream("application/startup"_el);
    log->info("Loaded logging from application.elcl."_el);
}

}
