// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompiledBlock_fwd.hpp"
#include "CompiledExtends_fwd.hpp"
#include "CompiledInclude_fwd.hpp"
#include "CompiledLayout_fwd.hpp"
#include "Program.hpp"

#include "../LayoutSource.hpp"
#include "../Value_fwd.hpp"

#include "../../StringList.hpp"
#include "../../StringMap.hpp"

#include <memory>
#include <vector>

namespace erbsland::text::render::impl {

/// One immutable compiled generation of a logical layout.
/// @tested{RenderEnvironmentTest RenderIncludeTest RenderInheritanceTest}
class CompiledLayout final {
public:
    /// One pre-resolved implementation in a derived-to-base block chain.
    struct BlockImplementation {
        std::size_t layoutLevel{};   ///< Derived-to-base layout level.
        ConstCompiledBlockPtr block; ///< Immutable compiled block program.
    };
    /// One immutable derived-to-base chain of block implementations.
    using BlockChain = std::vector<BlockImplementation>;
    /// One shared immutable block implementation chain.
    using ConstBlockChainPtr = std::shared_ptr<const BlockChain>;

public:
    /// Create a compiled layout generation with setup, body, blocks, and static dependencies.
    CompiledLayout(
        String name,
        LayoutSource source,
        StringList constants,
        Program setupProgram,
        Program bodyProgram,
        StringMap<FilterFn> applicationFilters,
        StringMap<ConstCompiledBlockPtr> blocks,
        std::vector<ConstCompiledIncludePtr> includes = {},
        ConstCompiledExtendsPtr extends = {}) noexcept;
    /// Create a legacy single-program generation used by focused malformed-program tests.
    CompiledLayout(
        String name,
        LayoutSource source,
        StringList constants,
        Program program,
        std::vector<ConstCompiledIncludePtr> includes = {}) noexcept;

    // defaults
    ~CompiledLayout() = default;
    CompiledLayout(const CompiledLayout &) = delete;
    CompiledLayout(CompiledLayout &&) noexcept = delete;
    auto operator=(const CompiledLayout &) -> CompiledLayout & = delete;
    auto operator=(CompiledLayout &&) noexcept -> CompiledLayout & = delete;

public: // accessors
    /// Access the logical layout name.
    [[nodiscard]] auto name() const noexcept -> const String & { return _name; }
    /// Access the loaded source.
    [[nodiscard]] auto source() const noexcept -> const LayoutSource & { return _source; }
    /// Access one constant, or an empty string if its index is invalid.
    [[nodiscard]] auto constant(unit::ItemIndex index) const -> String { return _constants.get(index, String{}); }
    /// Get the number of constants.
    [[nodiscard]] auto constantCount() const noexcept -> unit::ItemCount { return _constants.count(); }
    /// Access the hoisted setup program.
    [[nodiscard]] auto setupProgram() const noexcept -> const Program & { return _setupProgram; }
    /// Access the root-body program.
    [[nodiscard]] auto bodyProgram() const noexcept -> const Program & { return _bodyProgram; }
    /// Access the root-body program using the previous internal accessor.
    [[nodiscard]] auto program() const noexcept -> const Program & { return _bodyProgram; }
    /// Find one application filter retained by this generation.
    [[nodiscard]] auto applicationFilter(const String &name) const noexcept -> const FilterFn *;
    /// Find a block declared directly by this generation.
    [[nodiscard]] auto block(const String &name) const noexcept -> ConstCompiledBlockPtr;
    /// Access all blocks declared directly by this generation.
    [[nodiscard]] auto blocks() const noexcept -> const StringMap<ConstCompiledBlockPtr> & { return _blocks; }
    /// Find the derived-to-base implementations for one block.
    [[nodiscard]] auto blockChain(const String &name) const noexcept -> ConstBlockChainPtr;
    /// Access the direct parent dependency, if this layout extends another layout.
    [[nodiscard]] auto extendsDependency() const noexcept -> const ConstCompiledExtendsPtr & { return _extends; }
    /// Access the derived-to-base ancestors, excluding this generation.
    [[nodiscard]] auto ancestors() const noexcept -> const std::vector<ConstCompiledLayoutPtr> & { return _ancestors; }
    /// Access one static include dependency.
    [[nodiscard]] auto include(std::size_t index) const noexcept -> ConstCompiledIncludePtr {
        return index < _includes.size() ? _includes[index] : ConstCompiledIncludePtr{};
    }
    /// Access all static include dependencies.
    [[nodiscard]] auto includes() const noexcept -> const std::vector<ConstCompiledIncludePtr> & { return _includes; }

private:
    /// Build immutable ancestry and block-chain indexes from the direct parent.
    void buildInheritanceIndexes();

private:
    String _name;                                   ///< Logical layout name.
    LayoutSource _source;                           ///< Source text and revision.
    StringList _constants;                          ///< COW source slices and name constants.
    Program _setupProgram;                          ///< Hoisted layout-level assignments.
    Program _bodyProgram;                           ///< Root body with block dispatch instructions.
    StringMap<FilterFn> _applicationFilters;        ///< Immutable application filter snapshot.
    StringMap<ConstCompiledBlockPtr> _blocks;       ///< Programs declared by block name.
    std::vector<ConstCompiledIncludePtr> _includes; ///< Static include descriptors referenced by bytecode.
    ConstCompiledExtendsPtr _extends;               ///< Optional immutable direct parent dependency.
    std::vector<ConstCompiledLayoutPtr> _ancestors; ///< Direct parent followed by its ancestors.
    StringMap<ConstBlockChainPtr> _blockChains;     ///< Pre-resolved implementations by block name.
};

}
