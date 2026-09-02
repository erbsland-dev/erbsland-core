// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProcessInfo.hpp"

#include "PlatformError.hpp"

#include "impl/SystemInfoBackend.hpp"

#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::system {

using namespace text::literals;

ProcessInfo::ProcessInfo() noexcept : _processId{impl::systemInfoBackend().currentProcessId()} {
    reload();
}

ProcessInfo::ProcessInfo(const ProcessId processId) noexcept : _processId{processId} {
    reload();
}

auto ProcessInfo::executablePathOrThrow() const -> path::Path {
    if (_data.executablePath.isEmpty()) {
        throwUnavailable(_data.executablePathError, "Process executable path is unavailable."_el);
    }
    return _data.executablePath;
}

auto ProcessInfo::parentProcessIdOrThrow() const -> ProcessId {
    if (!_data.parentProcessId.isValid()) {
        throwUnavailable(_data.parentProcessIdError, "Parent process identifier is unavailable."_el);
    }
    return _data.parentProcessId;
}

auto ProcessInfo::startTimeOrThrow() const -> time::DateTime {
    if (!_data.startTime.isValid()) {
        throwUnavailable(_data.startTimeError, "Process start time is unavailable."_el);
    }
    return _data.startTime;
}

auto ProcessInfo::ownerIdOrThrow() const -> UserId {
    if (_data.ownerId.isEmpty()) {
        throwUnavailable(_data.ownerIdError, "Process owner identifier is unavailable."_el);
    }
    return _data.ownerId;
}

void ProcessInfo::reload() noexcept {
    try {
        _data = impl::systemInfoBackend().loadProcessInfo(_processId);
    } catch (...) {
        _data = {};
    }
}

void ProcessInfo::reloadOrThrow() {
    auto data = impl::systemInfoBackend().loadProcessInfo(_processId);
    _data = std::move(data);
    if (_data.lookupError.hasError()) {
        throw PlatformError{_data.lookupError.reason, _data.lookupError.context};
    }
}

void ProcessInfo::throwUnavailable(
    const impl::ProcessInfoError &attributeError, const text::String &fallbackReason) const {
    if (attributeError.hasError()) {
        throw PlatformError{attributeError.reason, attributeError.context};
    }
    if (_data.processError.hasError()) {
        throw PlatformError{_data.processError.reason, _data.processError.context};
    }
    if (_data.lookupError.hasError()) {
        throw PlatformError{_data.lookupError.reason, _data.lookupError.context};
    }
    throw PlatformError{fallbackReason};
}

}
