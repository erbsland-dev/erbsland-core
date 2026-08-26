// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../err/RuntimeError.hpp"

namespace erbsland::text::render::impl {

/// An internal or runtime bytecode execution failure.
/// @tested{RenderProgramTest RenderEnvironmentTest}
class ProgramError final : public err::RuntimeError {
public:
    using RuntimeError::RuntimeError;
};

}
