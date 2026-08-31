// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/all.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

using namespace el::text::literals;

/// Let each component in a medium application own a stable, named stream.
///
/// The application still owns and configures the manager. Components receive it during construction, create the
/// stream that identifies their responsibility, and retain that stream as `_log` for their whole lifetime.
class RouteCatalog final {
public:
    explicit RouteCatalog(el::LogManager &manager) : _log{manager.createStream("guild/routes"_el)} {}

    void load() { _log->info("Loaded the route catalog for 'Revontuli'."_el); }

private:
    el::LogStreamPtr _log;
};

class SupplyLedger final {
public:
    explicit SupplyLedger(el::LogManager &manager) : _log{manager.createStream("guild/supplies"_el)} {}

    void verify() { _log->warn("Two lanterns still need fresh oil."_el); }

private:
    el::LogStreamPtr _log;
};

class MediumLoggingApplication final : public el::Application {
public:
    using Application::Application;

protected:
    void initialize() override {
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{level} [{name}] {message}"_el);
        auto configuration = log().configuration();
        configuration.setLineFormat(std::move(lineFormat));
        log().setConfiguration(std::move(configuration));

        _routes = std::make_unique<RouteCatalog>(log());
        _supplies = std::make_unique<SupplyLedger>(log());
    }

    [[nodiscard]] auto main() -> el::ExitCode override {
        _routes->load();
        _supplies->verify();
        return el::ExitCode::success();
    }

private:
    std::unique_ptr<RouteCatalog> _routes;
    std::unique_ptr<SupplyLedger> _supplies;
};

auto runMediumLogging(const int argc, char *argv[]) -> int {
    auto app = MediumLoggingApplication{argc, argv};
    return app.run();
}

}
