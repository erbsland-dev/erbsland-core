// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompiledLayout.hpp"
#include "ProgramReader.hpp"

#include "../Value.hpp"

#include "../../../text/StringList.hpp"

#include <optional>
#include <variant>
#include <vector>

namespace erbsland::text::render::impl {

/// A lazy super request that is rendered directly unless a value operation consumes it.
/// @tested{RenderInheritanceTest}
struct PendingSuper {
    std::size_t depth{}; ///< Number of implementations to skip in the active block chain.
};

/// One value-stack entry in an execution frame.
using StackEntry = std::variant<Value, PendingSuper>;

/// The selected implementation within one resolved block chain.
/// @tested{RenderInheritanceTest}
struct BlockCall {
    String name;                              ///< Dispatched block name.
    CompiledLayout::ConstBlockChainPtr chain; ///< Immutable implementation chain.
    std::size_t chainIndex{};                 ///< Selected implementation index.
};

/// Independent reader and value stack for one setup, body, or block program invocation.
/// @tested{RenderProgramTest RenderInheritanceTest}
struct ExecutionFrame {
    /// Create an execution frame for one program invocation.
    ExecutionFrame(
        ConstCompiledLayoutPtr layout,
        const Program &program,
        StringList &output,
        std::optional<BlockCall> blockCall,
        std::size_t loopBase,
        std::size_t scopeBase);

    ConstCompiledLayoutPtr layout;      ///< Generation owning the executed program.
    ProgramReader reader;               ///< Independent bytecode reader.
    std::vector<StackEntry> stack;      ///< Independent expression value stack.
    StringList &output;                 ///< Selected shared or temporary output sink.
    std::optional<BlockCall> blockCall; ///< Active block call, if this is a block program.
    std::size_t loopBase{};             ///< Loop depth at frame entry.
    std::size_t scopeBase{};            ///< Scope depth at frame entry.
};

}
