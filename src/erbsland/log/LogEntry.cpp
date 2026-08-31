// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogEntry.hpp"

#include <utility>

namespace erbsland::log {

LogEntry::LogEntry(
    const uint64_t sequence,
    time::DateTime timestamp,
    const LogLevel level,
    LogPath path,
    text::String message,
    const bool truncated) :
    _sequence{sequence},
    _timestamp{std::move(timestamp)},
    _level{level},
    _path{std::move(path)},
    _message{std::move(message)},
    _truncated{truncated} {
}

}
