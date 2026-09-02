// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

namespace demo {

/// Application logging is ready without an explicit configuration.
///
/// The root stream writes information, warning, and error messages to the console.
void applicationLogging() {
    const auto log = el::application().logStream();

    // Write directly through the application root stream for a minimal tool.
    log->info("Explorer guild registry opened."_el);
    log->warn("Route notes for 'Pohjoinen tähti' are incomplete."_el);
    log->error("The ice-cave entrance could not be verified."_el);
}

}
