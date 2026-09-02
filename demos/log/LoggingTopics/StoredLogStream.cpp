// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Store one log stream with the component that gives its path meaning.
///
/// The component receives the shared manager rather than the whole application. It creates `_log` once, keeps the
/// lightweight pointer, and can safely use that same stream from work performed on different threads.
class ExpeditionJournal final {
public:
    explicit ExpeditionJournal(el::LogManager &manager) : _log{manager.createStream("guild/journal"_el)} {}

    void open() { _log->info("Opened the expedition journal."_el); }
    void addWeatherWarning() { _log->warn("Fresh snow covered the eastern trail markers."_el); }

private:
    el::LogStreamPtr _log;
};

void storedLogStream() {
    auto format = el::LogLineFormat{};
    format.setPattern("{level} [{name}] {message}"_el);
    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format))
        .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    auto journal = ExpeditionJournal{*manager};
    journal.open();
    journal.addWeatherWarning();
    manager->shutdown();
}

}
