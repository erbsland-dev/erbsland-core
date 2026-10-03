// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/RuntimeError.hpp"

namespace erbsland::cterm {

/// A terminal operation cannot be performed with the available terminal capabilities.
/// @tested{ReadLineTest}
class TerminalError final : public err::RuntimeError {
public:
    using err::RuntimeError::RuntimeError;

    // defaults
    ~TerminalError() override = default;
};

}
