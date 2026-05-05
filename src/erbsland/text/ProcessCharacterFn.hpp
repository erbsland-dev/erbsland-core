// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Char_fwd.hpp"

#include "../util/LoopStatus.hpp"

#include <functional>

namespace erbsland::text {

/// A function that processes one decoded character.
/// Return `LoopStatus::Continue` to continue iteration, `LoopStatus::Stop` to stop early, or `LoopStatus::Error`
/// to report an error.
/// @tested{U8StringModifierTest U16StringTest U32StringTest}
using ProcessCharacterFn = std::function<util::LoopStatus(Char character)>;

}
