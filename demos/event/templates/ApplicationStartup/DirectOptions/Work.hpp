// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/String.hpp>

#include <utility>

namespace demo {

/// Own the operations started on the application's main loop.
class Work final {
public:
    /// Borrow the application for this work object's lifetime.
    /// @param app Application whose lifetime encloses this object.
    explicit Work(el::Application &app) : _app{app} {}

public: // event work
    /// Start business operations in the running main loop.
    /// @param input Validated input retained for asynchronous operations.
    void begin(el::String input) {
        _input = std::move(input);
        // TODO: Create and retain your event sources here.
        // Connect their final successful completion to complete().
    }
    /// Finish after all required operations have completed on this loop.
    void complete() { _app.quit(); }

private:
    el::Application &_app; ///< Application outlives this owner.
    el::String _input;     ///< Input retained for callbacks.
};

}
