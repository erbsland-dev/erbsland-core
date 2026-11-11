// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConfigurationBatch.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorAction.hpp>

#include <exception>
#include <utility>

namespace demo {

ConfigurationBatch::ConfigurationBatch(el::PathList paths, const el::ItemCount maximumErrors) :
    _paths{std::move(paths)}, _maximumErrors{maximumErrors} {
}

void ConfigurationBatch::schedule(el::EventLoop &loop) {
    installErrorLimit(loop);
    for (const auto &path : _paths) {
        loop.invoke([this, path]() -> void { checkFile(path); });
    }
    // Store continues after a failed check, so normal completion always reaches this callback.
    loop.invoke([&loop]() -> void { loop.quit(); });
}

void ConfigurationBatch::checkFile(const el::Path &path) {
    ++_checkedCount;
    // Each file is independent. Parser errors retain the source location and any nested cause.
    (void)el::conf::Parser{}.parseFileOrThrow(path);
    _passedPaths.append(path);
}

void ConfigurationBatch::installErrorLimit(el::EventLoop &loop) {
    if (_maximumErrors.isZero()) {
        return; // Keep the configured Store policy when every file should be checked.
    }
    loop.setErrorHandler([this](std::exception_ptr) noexcept -> el::EventLoopErrorAction {
        ++_failureCount;
        if (_failureCount >= _maximumErrors) {
            return el::EventLoopErrorAction::Stop;
        }
        return el::EventLoopErrorAction::Continue;
    });
}

}
