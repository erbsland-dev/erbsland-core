// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>

namespace demo {

/// File writers create parent directories, recover from external file replacement, and optionally rotate archives.
///
/// Append or overwrite applies to the first successful open. Size rotation uses numbered archives, while hourly,
/// daily, and weekly rotation derive archive names from entry timestamps.
void fileWriters() {
    auto temporaryOptions = el::PathTempDirectoryOptions{};
    temporaryOptions.setPrefix("retkikunta-"_el).setRandomLength(el::CpLength{8U});
    const auto temporary =
        el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);
    const auto path = temporary->path() / "guild.log"_el;

    auto writerOptions = el::FileLogWriterOptions{path};
    writerOptions.setMode(el::LogFileMode::Overwrite)
        .setRotation(el::LogFileRotation::Size)
        .setMaximumSize(el::ByteLength{150U})
        .setRetention(2U);

    auto format = el::LogLineFormat{};
    format.setPattern("{level} [{name}] {message}"_el);
    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format)).addWriter(el::LogWriter::createForFile(writerOptions));

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto log = manager->createStream("guild/journal"_el);
    log->info("Retkikunta Revontuli departed from the cedar gate."_el);
    log->info("The first camp was established beside Jääjärvi."_el);
    log->warn("Fresh snowfall covered the eastern trail markers."_el);
    log->info("The expedition returned with a complete route sketch."_el);
    manager->shutdown();

    el::io::printLine("Active log exists: "_el, el::BooleanFormat::yesNo(), path.info().exists());
    el::io::printLine(
        "First archive exists: "_el, el::BooleanFormat::yesNo(), path.withStem("guild.1"_el).info().exists());
}

}
