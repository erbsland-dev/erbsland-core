// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogStream.hpp"

#include "impl/LogManagerData.hpp"

#include "../text/EscapeFormat.hpp"

#include <utility>

namespace erbsland::log {

LogStream::LogStream(LogPath path, LogTraceSection traceSection, impl::LogManagerDataWeakPtr manager, PrivateTag) :
    _path{std::move(path)}, _traceSection{std::move(traceSection)}, _manager{std::move(manager)} {
    _traceEnabled = false;
}

auto LogStream::createMuted() -> LogStreamPtr {
    return std::make_shared<LogStream>(LogPath{}, LogTraceSection{}, impl::LogManagerDataWeakPtr{}, PrivateTag{});
}

void LogStream::emitText(const LogLevel level, time::DateTime timestamp, const text::String &message) {
    if (_manager.expired()) {
        return;
    }
    const auto manager = _manager.lock();
    if (!manager) {
        return;
    }
    manager->enqueue(level, timestamp, _path, message.toEscaped(text::EscapeFormat::Log));
}

}
