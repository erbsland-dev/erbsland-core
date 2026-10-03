// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ActiveLogConfiguration.hpp"

#include "LogLineFormatter.hpp"

#include <algorithm>
#include <memory>
#include <utility>

namespace erbsland::log::impl {

ActiveLogConfiguration::ActiveLogConfiguration(const LogConfiguration &configuration) :
    _managerOptions{configuration.managerOptions()}, _traceSections{configuration.traceSections()} {
    _writers.reserve(configuration.writerCount());
    _uniqueWriters.reserve(configuration.writerCount());
    auto defaultFormatter = LogLineFormatterPtr{};
    for (const auto &binding : configuration.writerBindings()) {
        auto formatter = LogLineFormatterConstPtr{};
        if (binding.lineFormat.has_value()) {
            formatter = std::make_shared<LogLineFormatter>(*binding.lineFormat);
        } else {
            if (!defaultFormatter) {
                defaultFormatter = std::make_shared<LogLineFormatter>(configuration.lineFormat());
            }
            formatter = defaultFormatter;
        }
        _writers.push_back(ActiveLogWriter{binding.writer, binding.filter, std::move(formatter)});
        if (std::ranges::find(_uniqueWriters, binding.writer) == _uniqueWriters.end()) {
            _uniqueWriters.push_back(binding.writer);
        }
    }
}

auto ActiveLogConfiguration::isTraceSectionEnabled(const LogTraceSection &section) const noexcept -> bool {
    if (section.isEmpty()) {
        return true;
    }
    return std::ranges::find(_traceSections, section) != _traceSections.end();
}

}
