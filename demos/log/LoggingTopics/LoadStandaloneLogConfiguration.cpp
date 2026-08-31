// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/all.hpp>
#include <erbsland/conf/vr/Rules.hpp>
#include <erbsland/log/all.hpp>

#include <utility>

namespace demo {

/// Load logging from a dedicated `log.elcl` file.
///
/// The whole document follows the logging schema. Validate the document root, parse the complete configuration, and
/// install it before the application starts its ordinary work.
/// @notest{Compiled and executed by the logging topics demo.}
void loadStandaloneLogConfiguration() {
    const auto path = el::Path{"demos/log/LoggingTopics/data/log.elcl"_el};
    const auto document = el::conf::Parser{}.parseFileOrThrow(path);

    // The log schema is automatically validated by the parser.
    auto configuration = el::LogConfigurationParser{el::application().terminal()}.parse(document);
    el::application().log().setConfiguration(std::move(configuration));

    const auto log = el::application().log().createStream("log/startup"_el);
    log->info("Loaded logging from log.elcl."_el);
}

}
