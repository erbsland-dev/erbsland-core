// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>

namespace demo {

using namespace el::text::literals;

/// Derive from `Application` when the program is driven by asynchronous work on the main event loop.
///
/// The default `Application::main()` enters the event loop.
/// Calling `quit()` from an event callback finishes the loop and returns the requested exit code.
class MaintenanceApplication final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override {
        info().setApplicationName("Maintenance Queue"_el);
        info().setApplicationVersion(el::Version{1, 0, 0});
        events()->invoke([this]() -> void { runMaintenance(); });
    }

private:
    void runMaintenance() {
        el::io::printLine("maintenance job started"_el);
        events()->invoke([this]() -> void {
            el::io::printLine("maintenance job completed"_el);
            quit();
        });
    }
};

/// Create the application and leave event dispatch to its default main implementation.
auto main(const int argc, char *argv[]) -> int {
    auto app = MaintenanceApplication{argc, argv};
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
