// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Program_fwd.hpp"

#include "../../../mem/ByteBlock.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/CodeLocation.hpp"

#include <utility>
#include <vector>

namespace erbsland::text::render::impl {

/// Immutable bytecode and source mapping for one compiled layout program.
/// @tested{RenderProgramTest RenderEnvironmentTest}
class Program final {
public:
    /// One bytecode-to-source mapping entry.
    using SourceMapEntry = std::pair<unit::ByteIndex, unit::CodeLocation>;

public:
    // defaults
    Program() = default;
    ~Program() = default;
    Program(const Program &) = default;
    Program(Program &&) noexcept = default;
    auto operator=(const Program &) -> Program & = default;
    auto operator=(Program &&) noexcept -> Program & = default;

public:
    /// Create a program from bytecode and its source map.
    Program(mem::ByteBlock data, std::vector<SourceMapEntry> sourceMap) noexcept :
        _data{std::move(data)}, _sourceMap{std::move(sourceMap)} {}

public: // accessors
    /// Access the complete bytecode.
    [[nodiscard]] auto data() const noexcept -> const mem::ByteBlock & { return _data; }
    /// Find the closest source location at or before a bytecode position.
    [[nodiscard]] auto locationAt(unit::ByteIndex programCounter) const noexcept -> unit::CodeLocation;
    /// Find a source location while advancing a caller-owned sequential lookup hint.
    [[nodiscard]] auto locationAt(unit::ByteIndex programCounter, std::size_t &nextSourceMapEntry) const noexcept
        -> unit::CodeLocation;
    /// Test whether `programCounter` points to the start of an instruction.
    [[nodiscard]] auto hasInstructionAt(unit::ByteIndex programCounter) const noexcept -> bool;

private:
    mem::ByteBlock _data;                   ///< The byte-code of the program.
    std::vector<SourceMapEntry> _sourceMap; ///< Ordered bytecode-to-source mappings.
};

}
