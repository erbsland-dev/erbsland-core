// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::log {

class LogEntry;
/// Shared pointer to a mutable log entry.
using LogEntryPtr = std::shared_ptr<LogEntry>;
/// Shared pointer to an immutable log entry.
using LogEntryConstPtr = std::shared_ptr<const LogEntry>;

}
