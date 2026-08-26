// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompiledBlock_fwd.hpp"
#include "Program.hpp"

#include "../../../unit/CodeLocation.hpp"

namespace erbsland::text::render::impl {

/// One block program declared by a compiled layout generation.
/// @tested{RenderInheritanceTest RenderProgramTest}
class CompiledBlock final {
public:
    /// Create an empty block descriptor.
    CompiledBlock() = default;
    /// Create one compiled block.
    CompiledBlock(Program program, unit::CodeLocation location) noexcept :
        _program{std::move(program)}, _location{location} {}

    // defaults
    ~CompiledBlock() = default;
    CompiledBlock(const CompiledBlock &) = default;
    CompiledBlock(CompiledBlock &&) noexcept = default;
    auto operator=(const CompiledBlock &) -> CompiledBlock & = default;
    auto operator=(CompiledBlock &&) noexcept -> CompiledBlock & = default;

public: // accessors
    /// Access the block bytecode.
    [[nodiscard]] auto program() const noexcept -> const Program & { return _program; }
    /// Access the declaration location.
    [[nodiscard]] auto location() const noexcept -> unit::CodeLocation { return _location; }

private:
    Program _program;             ///< Compiled block bytecode.
    unit::CodeLocation _location; ///< Block declaration location.
};

}
