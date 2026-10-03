// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../event/EventCallback.hpp"

#include <exception>
#include <functional>

namespace erbsland::util::impl {

/// One committed operation outcome awaiting owner-loop delivery.
/// @tested{WorkerPoolEventTest}
struct WorkerDelivery final {
    event::EventCallback completed;                 ///< Successful invocation with retained result storage.
    std::function<void(std::exception_ptr)> failed; ///< Optional operation-specific failure recipient.
    std::exception_ptr error;                       ///< Operational failure or cancellation, if any.
    bool cancelled{false};                          ///< Whether cancellation selected this outcome.
};

}
