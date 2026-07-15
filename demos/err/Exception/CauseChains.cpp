// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <stdexcept>

namespace demo {

/// Preserve a caught failure with `std::current_exception()` when translating it into a domain-level exception.
/// The outer exception explains the failed operation, while `cause()` retains the original technical failure.
void causeChains() {
    try {
        try {
            throw std::runtime_error{"audio device disconnected"};
        } catch (...) {
            throw el::RuntimeError{"Shakuhachi recording could not be started."_el, std::current_exception()};
        }
    } catch (const el::RuntimeError &error) {
        el::io::printLine("Outer reason: "_el, error.reason());
        el::io::printLine("Has cause: "_el, error.hasCause());
        try {
            std::rethrow_exception(error.cause());
        } catch (const std::exception &cause) {
            el::io::printLine("Original cause: "_el, cause.what());
        }
    }
}

}
