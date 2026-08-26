// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompiledBlock_fwd.hpp"
#include "CompiledExtends_fwd.hpp"
#include "CompiledInclude_fwd.hpp"
#include "CompiledLayout_fwd.hpp"
#include "ProgramWriter.hpp"
#include "Tokenizer.hpp"

#include "../EnvironmentOptions.hpp"
#include "../LayoutSource.hpp"
#include "../Value_fwd.hpp"

#include "../../../text/StringList.hpp"
#include "../../../text/StringMap.hpp"

#include <functional>
#include <optional>
#include <variant>
#include <vector>

namespace erbsland::text::render::impl {

/// Compile the supported layout-language subset into bytecode.
/// @tested{RenderCompilerTest RenderExpressionTest RenderStatementTest RenderInheritanceTest}
class Compiler final {
public:
    /// Resolve one static layout dependency while compiling a graph.
    using DependencyResolverFn = std::function<ConstCompiledLayoutPtr(const String &, bool)>;

private:
    /// Compile-time state for one open conditional statement.
    struct IfBlock {
        unit::CodeLocation location;                           ///< Location of the opening `if`.
        std::optional<ProgramWriter::JumpHandle> pendingFalse; ///< Jump to the next branch.
        std::vector<ProgramWriter::JumpHandle> endJumps;       ///< Selected branches jumping to the end.
        bool hasElse{false};                                   ///< Whether an `else` was already seen.
    };
    /// Compile-time state for one open iteration statement.
    struct ForBlock {
        unit::CodeLocation location;                      ///< Location of the opening `for`.
        ProgramWriter::Label iterationLabel;              ///< Label of the iteration-advance instruction.
        ProgramWriter::JumpHandle pendingExit;            ///< Jump from exhausted iteration to `EndIteration`.
        std::optional<ProgramWriter::JumpHandle> elseEnd; ///< Non-empty loops skipping the else body.
        bool hasElse{false};                              ///< Whether an `else` was already seen.
    };
    using ControlBlock = std::variant<IfBlock, ForBlock>; ///< One structurally nested control block.
    /// One currently compiled root or named-block program.
    struct ProgramContext {
        String name;                        ///< Empty for the root body, otherwise the block name.
        unit::CodeLocation location;        ///< Declaration location for a named block.
        ProgramWriter writer;               ///< Program bytecode writer.
        std::vector<ControlBlock> controls; ///< Open condition and iteration structures.
    };

public:
    /// Create a compiler for one loaded source.
    Compiler(
        String layout,
        LayoutSource source,
        EnvironmentOptions options,
        StringMap<FilterFn> filters = {},
        DependencyResolverFn dependencyResolver = {});

public:
    /// Compile the source into an immutable generation.
    [[nodiscard]] auto compile() -> ConstCompiledLayoutPtr;

private:
    /// Compile one output expression, leaving the tokenizer at the tag end.
    void compileExpression(Tokenizer &tokenizer);
    /// Compile one value expression into the active program.
    void compileValueExpression(Tokenizer &tokenizer);
    /// Compile one value expression into an explicit program.
    void compileValueExpression(Tokenizer &tokenizer, ProgramWriter &writer, bool allowSuper);
    /// Compile one control statement, leaving the tokenizer at the tag end.
    void compileStatement(Tokenizer &tokenizer);
    /// Compile a render-local assignment, hoisting a layout-level assignment into setup.
    void compileSet(Tokenizer &tokenizer);
    /// Compile the opening of a list or ordered-map iteration.
    void compileFor(Tokenizer &tokenizer, unit::CodeLocation statementLocation);
    /// Compile the closing of an iteration.
    void compileEndFor(Tokenizer &tokenizer, unit::CodeLocation statementLocation);
    /// Compile one static layout include.
    void compileInclude(Tokenizer &tokenizer, unit::CodeLocation statementLocation);
    /// Compile one static parent declaration.
    void compileExtends(Tokenizer &tokenizer, unit::CodeLocation statementLocation);
    /// Begin compiling a separately stored named block.
    void compileBlock(Tokenizer &tokenizer, unit::CodeLocation statementLocation);
    /// Finish the active named block.
    void compileEndBlock(Tokenizer &tokenizer, unit::CodeLocation statementLocation);
    /// Access the active program writer.
    [[nodiscard]] auto writer() noexcept -> ProgramWriter & { return _programs.back().writer; }
    /// Access the active program's control stack.
    [[nodiscard]] auto controls() noexcept -> std::vector<ControlBlock> & { return _programs.back().controls; }
    /// Access the active program's control stack.
    [[nodiscard]] auto controls() const noexcept -> const std::vector<ControlBlock> & {
        return _programs.back().controls;
    }
    /// Test whether compilation is in the root-body program.
    [[nodiscard]] auto isRootProgram() const noexcept -> bool { return _programs.size() == 1U; }
    /// Test whether compilation is at layout level outside control structures.
    [[nodiscard]] auto isLayoutTopLevel() const noexcept -> bool {
        return isRootProgram() && _programs.front().controls.empty();
    }
    /// Test if compilation is currently inside an iteration body.
    [[nodiscard]] auto isInsideFor() const noexcept -> bool;
    /// Record content forbidden outside blocks in an extending layout.
    void noteExtendingContent(unit::CodeLocation location);
    /// Validate a loop target and reject reserved names.
    void validateForTarget(const String &name, unit::CodeLocation location) const;
    /// Emit one non-empty raw-text token.
    void emitText(String text, unit::CodeLocation location);
    /// Add a constant and return its zero-based index.
    [[nodiscard]] auto addConstant(String value) -> std::size_t;
    /// Throw the error for an unclosed active control structure.
    [[noreturn]] void throwUnclosedControl(const ControlBlock &block) const;
    /// Throw a syntax error for this source.
    [[noreturn]] void throwSyntax(String description, unit::CodeLocation location) const;

private:
    String _layout;                                             ///< Logical layout name.
    LayoutSource _source;                                       ///< Loaded source generation.
    EnvironmentOptions _options;                                ///< Validated syntax options.
    StringMap<FilterFn> _applicationFilters;                    ///< Setup-time application filter registry.
    DependencyResolverFn _dependencyResolver;                   ///< Static dependency resolver.
    StringList _constants;                                      ///< Shared program constant pool.
    ProgramWriter _setupWriter;                                 ///< Hoisted layout-level assignments.
    std::vector<ProgramContext> _programs;                      ///< Root followed by active nested block programs.
    StringMap<ConstCompiledBlockPtr> _compiledBlocks;           ///< Completed uniquely named block programs.
    StringMap<bool> _blockNames;                                ///< All declared or active block names.
    std::vector<ConstCompiledIncludePtr> _includes;             ///< Bound static include dependencies.
    ConstCompiledExtendsPtr _extends;                           ///< Optional bound static parent dependency.
    std::optional<unit::CodeLocation> _invalidExtendingContent; ///< First forbidden root content.
};

}
