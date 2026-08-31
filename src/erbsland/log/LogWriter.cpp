// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogWriter.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::log {

using namespace text::literals;

LogWriter::BatchItem::BatchItem(LogEntryConstPtr entry, LogLineConstPtr line) :
    _entry{std::move(entry)}, _line{std::move(line)} {
    if (!_entry) {
        throw err::ParameterError{"A log writer batch entry must not be empty."_el, "entry"_el};
    }
    if (!_line) {
        throw err::ParameterError{"A log writer batch line must not be empty."_el, "line"_el};
    }
}

void LogWriter::writeBatch(const Batch batch) {
    for (const auto &item : batch) {
        write(item.entry(), item.line());
    }
}

auto LogWriter::bind(const void *manager) noexcept -> bool {
    const auto lock = std::scoped_lock{_bindingMutex};
    if (_manager != nullptr && _manager != manager) {
        return false;
    }
    _manager = manager;
    return true;
}

void LogWriter::release(const void *manager) noexcept {
    const auto lock = std::scoped_lock{_bindingMutex};
    if (_manager == manager) {
        _manager = nullptr;
    }
}

}
