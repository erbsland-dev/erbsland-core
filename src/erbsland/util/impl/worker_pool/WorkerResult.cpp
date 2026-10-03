// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "WorkerResult.hpp"

namespace erbsland::util::impl {

auto WorkerResult<void>::future() -> std::future<void> {
    return _promise.emplace().get_future();
}

auto WorkerResult<void>::publish(std::exception_ptr error) -> event::EventCallback {
    if (_promise) {
        if (error) {
            _promise->set_exception(error);
        } else {
            _promise->set_value();
        }
        return {};
    }
    if (error) {
        return {};
    }
    return std::move(_completed);
}

}
