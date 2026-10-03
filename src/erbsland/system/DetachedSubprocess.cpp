// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DetachedSubprocess.hpp"

#include "PlatformError.hpp"

#include "impl/subprocess/SubprocessBackend.hpp"
#include "impl/subprocess/SubprocessValidation.hpp"

#include <thread>

namespace erbsland::system {

auto DetachedSubprocess::start(
    const path::Path &executable, const text::StringList &arguments, const SubprocessOptions &options)
    -> DetachedSubprocessPtr {
    impl::validateSubprocess(executable, arguments, options, true);
    auto backend =
        std::shared_ptr<impl::SubprocessBackend>{impl::SubprocessBackend::create(executable, arguments, options)};
    auto result = DetachedSubprocessPtr{new DetachedSubprocess{backend->processId()}};
    try {
        std::thread{[backend]() -> void {
            try {
                backend->wait();
            } catch (const PlatformError &) { // No observer exists for detached wait errors.
            }
        }}.detach();
    } catch (...) {
        try {
            backend->kill();
            backend->wait();
        } catch (const PlatformError &) { // Preserve the original failure after attempted native cleanup.
        }
        throw;
    }
    return result;
}

}
