// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/log/all.hpp>

#include <memory>
#include <utility>

namespace demo {

/// Render retained UTC timestamps either in UTC or in the process-local time zone.
///
/// `setTimestampZone()` changes presentation only; the entry keeps its original UTC instant. ISO output includes `Z`
/// for UTC or the applicable numeric offset for local time.
void lineTimestampZones() {
    const auto showZone = [](const el::String &label, const el::LogTimestampZone zone) -> void {
        el::io::printLine(label, ":"_el);
        auto lineFormat = el::LogLineFormat{};
        lineFormat.setPattern("{time} - {message}"_el).setTimestampZone(zone);
        auto configuration = el::LogConfiguration{};
        configuration.setLineFormat(std::move(lineFormat))
            .addWriter(std::make_shared<el::ConsoleLogWriter>(el::application().terminal()));

        const auto manager = el::LogManager::create();
        manager->setConfiguration(std::move(configuration));
        manager->rootStream()->info("Expedition clock synchronized."_el);
        manager->shutdown();
    };

    showZone("UTC rendering"_el, el::LogTimestampZone::Utc);
    showZone("Local rendering"_el, el::LogTimestampZone::Local);
}

}
