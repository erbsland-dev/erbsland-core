// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ExecutionFrame.hpp"

namespace erbsland::text::render::impl {

ExecutionFrame::ExecutionFrame(
    ConstCompiledLayoutPtr layoutValue,
    const Program &program,
    StringList &outputValue,
    std::optional<BlockCall> blockCallValue,
    const std::size_t loopBaseValue,
    const std::size_t scopeBaseValue) :
    layout{std::move(layoutValue)},
    reader{program},
    output{outputValue},
    blockCall{std::move(blockCallValue)},
    loopBase{loopBaseValue},
    scopeBase{scopeBaseValue} {
    stack.reserve(32U);
}

}
