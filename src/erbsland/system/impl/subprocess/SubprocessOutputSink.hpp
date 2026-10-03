// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../PlatformErrorContext_fwd.hpp"

#include <functional>
#include <string_view>

namespace erbsland::system::impl {

/// Native pipe delivery; callbacks run only on pipe-reader threads.
/// @tested{SubprocessEventTest}
struct SubprocessOutputSink final {
    std::function<void(bool, std::string_view)> output;      ///< Original bytes; true selects stderr.
    std::function<void(PlatformErrorContextConstPtr)> error; ///< Native read failure.
};

}
