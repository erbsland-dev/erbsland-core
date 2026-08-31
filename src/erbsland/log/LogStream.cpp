// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogStream.hpp"

#include "impl/LogManagerData.hpp"

#include "../text/EscapeFormat.hpp"

#include <utility>

namespace erbsland::log {

LogStream::LogStream(
    LogPath path, LogTraceSection traceSection, impl::LogManagerDataWeakPtr manager, ConstructionToken) :
    _path{std::move(path)}, _traceSection{std::move(traceSection)}, _manager{std::move(manager)} {
}

void LogStream::emitText(const LogLevel level, time::DateTime timestamp, text::String message) {
    const auto manager = _manager.lock();
    if (!manager) {
        return;
    }
    manager->enqueue(level, std::move(timestamp), _path, message.toEscaped(text::EscapeFormat::Log));
}

}
