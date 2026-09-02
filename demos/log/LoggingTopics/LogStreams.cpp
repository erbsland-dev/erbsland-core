// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace demo {

/// Build one coherent entry from printable values and formatting controls.
///
/// Log methods accept the same printable values as Erbsland Core text streams. Inline controls affect the values that
/// follow them, so structured values can stay typed until the stream prepares one complete, safely escaped message.
void logStreams() {
    auto format = el::LogLineFormat{};
    format.setPattern("{level} [{name}] {message}"_el);
    auto configuration = el::LogConfiguration{};
    configuration.setLineFormat(std::move(format))
        .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

    const auto manager = el::LogManager::create();
    manager->setConfiguration(std::move(configuration));
    const auto log = manager->createStream("guild/equipment"_el);

    const auto seal = el::ByteBlock::fromVector(std::vector<std::uint8_t>{0x2aU, 0x7cU, 0x91U, 0xe0U});
    log->info(
        "Crates="_el,
        el::IntegerFormat::hexadecimal(),
        42,
        ", checked="_el,
        el::BooleanFormat::yesNo(),
        true,
        ", seal="_el,
        el::ByteFormat::separated(),
        seal);
    manager->shutdown();
}

}
