// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VerificationSession.hpp"

#include <erbsland/RuntimeError.hpp>
#include <erbsland/StandardStreams.hpp>

namespace demo {

using namespace el::text::literals;

VerificationSession::VerificationSession(const el::Path &manifest, const bool failFast) :
    _directory{manifest.parent()},
    _entries{ManifestEntry::read(manifest)},
    _failFast{failFast},
    _owner{el::EventLoop::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Quit))},
    _worker{el::UnmanagedEventThread::create()} {
}

auto VerificationSession::run() -> bool {
    _worker->start();
    try {
        schedule(el::ItemIndex{});
        _owner->run();
    } catch (...) {
        _cancellation.requestStop();
        _worker->quit();
        _worker->join();
        throw;
    }
    _worker->quit();
    _worker->join();
    if (const auto error = _owner->takeError()) {
        std::rethrow_exception(error);
    }
    if (_error) {
        std::rethrow_exception(_error);
    }
    return _mismatches.isZero();
}

void VerificationSession::schedule(const el::ItemIndex index) {
    _worker->loop()->invoke([this, index, stop = _cancellation.stopToken()]() -> void {
        try {
            check(index, stop);
        } catch (const el::RuntimeError &) {
            _owner->invoke([this, error = std::current_exception()]() -> void { finish(error); });
        }
    });
}

void VerificationSession::check(const el::ItemIndex index, const el::StopToken &stop) {
    if (stop.isStopRequested() || !index.isWithin(_entries.count())) {
        _owner->invoke([this]() -> void { finish({}); });
        return;
    }
    const auto &entry = _entries.getRefOrThrow(index);
    const auto matches = entry.matchesFile(_directory);
    _owner->invoke([this, index, matches]() -> void { record(index, matches); });
}

void VerificationSession::record(const el::ItemIndex index, const bool matches) {
    ++_checked;
    const auto &name = _entries.getRefOrThrow(index).name();
    if (matches) {
        el::io::printLine("OK        "_el, name);
    } else {
        ++_mismatches;
        el::io::printLine("MISMATCH  "_el, name);
        if (_failFast) {
            _cancellation.requestStop();
        }
    }
    schedule(index.incremented());
}

void VerificationSession::finish(std::exception_ptr error) {
    _error = std::move(error);
    _owner->invoke([this]() -> void { printSummary(); });
    _owner->quit();
}

void VerificationSession::printSummary() const {
    el::io::printLine(
        "Checked: "_el, _checked, ", mismatches: "_el, _mismatches, ", unchecked: "_el, _entries.count() - _checked);
}

}
