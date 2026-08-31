// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Use several streams when one component carries several distinct responsibilities.
///
/// The expedition service separates route decisions from supply accounting. Names in the rendered lines tell the
/// reader where an event originated, and the same paths can later become precise writer routes.
class ExpeditionService final {
public:
    explicit ExpeditionService(el::LogManager &manager) :
        _routeLog{manager.createStream("guild/expedition/routes"_el)},
        _supplyLog{manager.createStream("guild/expedition/supplies"_el)} {}

    void prepare() {
        _routeLog->info("The ridge route has three checkpoints."_el);
        _supplyLog->warn("Two lanterns still need fresh oil."_el);
    }

private:
    el::LogStreamPtr _routeLog;
    el::LogStreamPtr _supplyLog;
};

void multipleLogStreams() {
    auto format = el::LogLineFormat{};
    format.setPattern("{level} [{name}] {message}"_el);
    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format))
        .addWriter(std::make_shared<el::ConsoleLogWriter>(el::application().terminal()));

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    auto expedition = ExpeditionService{*manager};
    expedition.prepare();
    manager->shutdown();
}

}
