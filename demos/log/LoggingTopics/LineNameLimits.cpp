// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Bound left-truncated stream names by a code-point count.
///
/// `setNameLimit()` is consulted only for `LogNameFormat::LeftTruncated`. Keeping the right side preserves the most
/// specific path segments; a zero limit disables name shortening even when that format is selected.
void lineNameLimits() {
    const auto showLimit = [](const el::String &label, const el::CpLength limit) -> void {
        el::io::printLine(label, ":"_el);
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{name}: {message}"_el)
            .setNameFormat(el::LogNameFormat::LeftTruncated)
            .setNameLimit(limit);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat))
            .addWriter(el::LogWriter::createForConsole(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        manager->createStream("guild/exploration/finland/lapland/northern-lights"_el)->info("Survey opened."_el);
        manager->shutdown();
    };

    showLimit("32 characters"_el, el::CpLength{32U});
    showLimit("18 characters"_el, el::CpLength{18U});
    showLimit("Unlimited"_el, el::CpLength::zero());
}

}
