// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>
#include <erbsland/log/all.hpp>

namespace demo {

using namespace el::text::literals;

/// Display recent errors when an application exits with a failure.
///
/// Enable the retained-error safety net before startup work can fail. The application keeps these errors across later
/// logging configuration changes and displays them during final cleanup when `main()` returns a failure exit code.
auto runLastErrorDump(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    app.enableLastErrorDump();
    app.setMainFn([&app]() -> el::ExitCode {
        const auto log = app.log().createStream("guild/expedition"_el);
        log->info("The expedition entered the northern pass."_el);
        log->error("The reserve compass failed its check."_el);
        log->error("The return route is blocked by ice."_el);
        return el::ExitCode{2};
    });
    return app.run();
}

}
