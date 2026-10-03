// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TaskLogWriter.hpp"

namespace tool::pre_commit::impl {

void TaskLogWriter::write(const el::LogEntryConstPtr &entry, const el::log::LogLineConstPtr &) {
    _errors = _errors || entry->level() == el::LogLevel::Error;
    _warnings = _warnings || entry->level() == el::LogLevel::Warning;
    _truncated = _truncated || entry->isTruncated();
    if (_verbose || entry->level() == el::LogLevel::Error || entry->level() == el::LogLevel::Warning) {
        _entries.append(entry);
    }
}

}
