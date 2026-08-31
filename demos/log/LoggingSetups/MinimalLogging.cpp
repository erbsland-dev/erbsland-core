// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>
#include <erbsland/log/all.hpp>

namespace demo {

using namespace el::text::literals;

/// Give a small command-line application one root log stream.
///
/// The application creates the default console configuration lazily. Keeping the root stream is enough when the
/// program has only one useful source of messages and readers do not need subsystem names.
auto runMinimalLogging(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    app.setMainFn([&app]() -> el::ExitCode {
        const auto log = app.logStream();
        log->info("Explorer registry opened."_el);
        log->warn("The route notes for 'Revontuli' are incomplete."_el);
        return el::ExitCode::success();
    });
    return app.run();
}

}
