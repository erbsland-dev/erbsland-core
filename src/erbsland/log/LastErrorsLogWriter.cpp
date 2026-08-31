// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LastErrorsLogWriter.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::log {

using namespace text::literals;

LastErrorsLogWriter::LastErrorsLogWriter(const std::size_t capacity) : _capacity{capacity} {
    if (_capacity == 0U) {
        throw err::ParameterError{"The last-errors capacity must be greater than zero."_el, "capacity"_el};
    }
}

void LastErrorsLogWriter::write(const LogEntryConstPtr &entry, [[maybe_unused]] const LogLineConstPtr &line) {
    if (entry->level() != LogLevel::Error) {
        return;
    }
    const auto lock = std::scoped_lock{_mutex};
    _entries.push_back(entry);
    while (_entries.size() > _capacity) {
        _entries.pop_front();
    }
}

auto LastErrorsLogWriter::snapshot() const -> std::vector<LogEntryConstPtr> {
    const auto lock = std::scoped_lock{_mutex};
    return {_entries.begin(), _entries.end()};
}

}
