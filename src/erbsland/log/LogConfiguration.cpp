// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogConfiguration.hpp"

#include "LogWriter.hpp"

#include "../err/ParameterFailure.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::log {

using namespace text::literals;

auto LogConfiguration::setLineFormat(LogLineFormat value) noexcept -> LogConfiguration & {
    _lineFormat = std::move(value);
    return *this;
}

auto LogConfiguration::setManagerOptions(LogManagerOptions value) -> LogConfiguration & {
    if (value.reservedErrorEntries() > value.maximumEntries()) {
        throw err::ParameterFailure{
            "The reserved log entry count must not exceed the queue limit."_el, "reservedErrorEntries"_el};
    }
    if (value.reservedErrorBytes() > value.maximumBytes()) {
        throw err::ParameterFailure{
            "The reserved log byte count must not exceed the queue limit."_el, "reservedErrorBytes"_el};
    }
    _managerOptions = std::move(value);
    return *this;
}

auto LogConfiguration::addWriter(LogWriterPtr writer, LogWriterFilter filter, std::optional<LogLineFormat> lineFormat)
    -> LogConfiguration & {
    if (!writer) {
        throw err::ParameterFailure{"A log writer must not be empty."_el, "writer"_el};
    }
    _writers.push_back(impl::LogWriterBinding{std::move(writer), std::move(filter), std::move(lineFormat)});
    return *this;
}

auto LogConfiguration::enableTraceSection(LogTraceSection section) -> LogConfiguration & {
    if (!section.isEmpty() && !isTraceSectionEnabled(section)) {
        _traceSections.emplace_back(std::move(section));
    }
    return *this;
}

auto LogConfiguration::isTraceSectionEnabled(const LogTraceSection &section) const noexcept -> bool {
    if (section.isEmpty()) {
        return true;
    }
    for (const auto &enabled : _traceSections) {
        if (enabled == section) {
            return true;
        }
    }
    return false;
}

}
