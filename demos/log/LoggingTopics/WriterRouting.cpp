// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <atomic>
#include <memory>

namespace demo {

class RouteCountingLogWriter final : public el::LogWriter {
public:
    void write(const el::LogEntryConstPtr &, const el::LogLineConstPtr &) override { ++_count; }
    [[nodiscard]] auto count() const noexcept -> std::size_t { return _count.load(); }

private:
    std::atomic<std::size_t> _count{};
};

/// Writer filters combine accepted levels with zero or more hierarchical path roots.
///
/// Empty path filters accept every stream. Once roots are present, matching uses complete path segments, so a route
/// for `guild` includes `guild/archive` but not `guildhall`. If several filters match, every matching writer receives
/// the entry.
void writerRouting() {
    const auto operations = std::make_shared<RouteCountingLogWriter>();
    const auto archive = std::make_shared<RouteCountingLogWriter>();

    auto operationsFilter = el::LogWriterFilter{el::LogLevels{el::LogLevel::Warning, el::LogLevel::Error}};
    operationsFilter.addPath(el::LogPath{"guild"_el});
    auto archiveFilter =
        el::LogWriterFilter{el::LogLevels{el::LogLevel::Information, el::LogLevel::Warning, el::LogLevel::Error}};
    archiveFilter.addPath(el::LogPath{"guild/archive"_el});

    auto configuration = el::LogConfiguration{};
    configuration.addWriter(operations, std::move(operationsFilter)).addWriter(archive, std::move(archiveFilter));

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto archiveLog = manager->createStream("guild/archive"_el);
    const auto guildHallLog = manager->createStream("guildhall"_el);

    archiveLog->info("The autumn route ledger is ready."_el); // Archive only.
    archiveLog->warn("The archive door was left open."_el);   // Both writers.
    guildHallLog->error("The guildhall bell rope broke."_el); // Neither writer.
    manager->shutdown();

    el::io::printLine("Operational entries: "_el, operations->count());
    el::io::printLine("Archive entries    : "_el, archive->count());
}

}
