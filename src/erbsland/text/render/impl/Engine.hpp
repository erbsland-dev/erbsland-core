// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompiledLayout.hpp"
#include "ExecutionFrame.hpp"
#include "LoopState.hpp"
#include "Opcode.hpp"
#include "ProgramReader.hpp"
#include "RenderCounters.hpp"

#include "../Context.hpp"
#include "../RenderError.hpp"
#include "../RenderErrorCategory.hpp"
#include "../RenderLimits.hpp"
#include "../Value.hpp"

#include "../../../text/StringList.hpp"

#include <compare>
#include <exception>
#include <optional>
#include <vector>

namespace erbsland::text::render::impl {

/// Mutable execution state for one immutable compiled layout generation.
/// @tested{RenderLanguageCompletionTest}
class Engine final {
public:
    /// Create state for one render invocation.
    Engine(
        ConstCompiledLayoutPtr layout,
        const Context &globalContext,
        const Context &localContext,
        RenderLimits limits = {});
    /// Create instrumented state for renderer profiling.
    Engine(
        ConstCompiledLayoutPtr layout,
        const Context &globalContext,
        const Context &localContext,
        RenderCounters &counters,
        RenderLimits limits = {});

public:
    /// Execute the complete inheritance render and return its output.
    [[nodiscard]] auto render() -> String;

private:
    /// Create one nested include state sharing its parent's visible context.
    Engine(
        ConstCompiledLayoutPtr layout,
        const Context &globalContext,
        const Context &localContext,
        const Engine &parent,
        bool withParentContext,
        std::size_t depth,
        RenderCounters *counters);
    /// Run setup base-to-derived and render the ultimate base body.
    void executeLayout(StringList &output);
    /// Execute one independent bytecode program.
    void executeProgram(
        const ConstCompiledLayoutPtr &layout,
        const Program &program,
        StringList &output,
        std::optional<BlockCall> blockCall = {});
    /// Access this render root or one of its immutable ancestors by derived-to-base level.
    [[nodiscard]] auto layoutAtLevel(std::size_t level) const -> ConstCompiledLayoutPtr;
    /// Append generated text while enforcing the shared output limit.
    void appendOutput(ExecutionFrame &frame, String text, unit::CodeLocation location);
    /// Push a value while enforcing the VM stack limit.
    void push(ExecutionFrame &frame, Value value, unit::CodeLocation location);
    /// Push a lazy super request.
    void pushSuper(ExecutionFrame &frame, std::size_t depth, unit::CodeLocation location);
    /// Pop one raw stack entry or reject malformed stack state.
    [[nodiscard]] auto takeTop(ExecutionFrame &frame, Opcode opcode) -> StackEntry;
    /// Pop and materialize one value, capturing a pending super request when necessary.
    [[nodiscard]] auto takeValue(ExecutionFrame &frame, Opcode opcode, unit::CodeLocation location) -> Value;
    /// Resolve and access the top stack value.
    [[nodiscard]] auto resolvedTop(ExecutionFrame &frame, unit::CodeLocation location) -> Value &;
    /// Capture one pending super request as text.
    [[nodiscard]] auto captureSuper(ExecutionFrame &frame, std::size_t depth, unit::CodeLocation location) -> Value;
    /// Read a checked string constant from the active program owner.
    [[nodiscard]] static auto constant(const ExecutionFrame &frame, uint64_t index, const String &kind) -> String;
    /// Resolve a name using render, local, then global precedence.
    [[nodiscard]] auto lookup(const ExecutionFrame &frame, const String &name, unit::CodeLocation location) const
        -> Value;
    /// Find one visible value without resolving callbacks.
    [[nodiscard]] auto findVisible(const String &name, bool &found) const -> Value;
    /// Resolve lazy callback values with a bounded recursion depth.
    [[nodiscard]] auto resolve(Value value, const ConstCompiledLayoutPtr &layout, unit::CodeLocation location) const
        -> Value;
    /// Create a source-aware public render error.
    [[nodiscard]] auto error(
        RenderErrorCategory category,
        String title,
        String description,
        const ConstCompiledLayoutPtr &layout,
        unit::CodeLocation location,
        std::exception_ptr cause = {}) const -> RenderError;
    /// Execute one scalar comparison instruction.
    void executeComparison(ExecutionFrame &frame, Opcode opcode, unit::CodeLocation location);
    /// Execute one unary numeric instruction.
    void executeUnary(ExecutionFrame &frame, Opcode opcode, unit::CodeLocation location);
    /// Execute one binary numeric instruction.
    void executeArithmetic(ExecutionFrame &frame, Opcode opcode, unit::CodeLocation location);
    /// Execute renderer string concatenation.
    void executeConcatenation(ExecutionFrame &frame, unit::CodeLocation location);
    /// Build a collection from stack values.
    void buildCollection(ExecutionFrame &frame, Opcode opcode, uint64_t count, unit::CodeLocation location);
    /// Execute strict membership.
    void executeMembership(ExecutionFrame &frame, bool negate, unit::CodeLocation location);
    /// Execute an argument-free value test.
    void executeTest(ExecutionFrame &frame, uint64_t test, bool negate, unit::CodeLocation location);
    /// Compare two scalar values for equality.
    [[nodiscard]] auto valuesEqual(const Value &left, const Value &right) const -> bool;
    /// Order two compatible scalar values.
    [[nodiscard]] auto compareValues(
        const ConstCompiledLayoutPtr &layout, const Value &left, const Value &right, unit::CodeLocation location) const
        -> std::partial_ordering;
    /// Apply one registered filter to the top value.
    void applyFilter(ExecutionFrame &frame, uint64_t operand, bool builtIn, unit::CodeLocation location);
    /// Access a list stored in a checked value.
    [[nodiscard]] static auto list(const Value &value) -> const ValueList &;
    /// Access a map stored in a checked value.
    [[nodiscard]] static auto map(const Value &value) -> const ValueMap &;
    /// Open a checked list or map iteration frame.
    void beginIteration(ExecutionFrame &frame, bool expectMap, unit::CodeLocation location);
    /// Advance the innermost iteration and bind its values.
    void nextIteration(ExecutionFrame &frame, uint64_t exitTarget, unit::CodeLocation location);
    /// Close the innermost exhausted iteration.
    void endIteration(const ExecutionFrame &frame);
    /// Remove the current item scope before advancing an iteration.
    void removeIterationScope(LoopState &loop, const ExecutionFrame &frame);
    /// Execute one statically bound include dependency.
    void executeInclude(ExecutionFrame &frame, uint64_t index, unit::CodeLocation location);
    /// Dispatch one named block to its most-derived implementation.
    void executeBlock(ExecutionFrame &frame, const String &name, unit::CodeLocation location);
    /// Execute a super request relative to the active block implementation.
    void executeSuper(
        ExecutionFrame &frame, std::size_t depth, StringList &output, unit::CodeLocation location, bool captured);
    /// Execute one selected block-chain implementation.
    void executeBlockImplementation(
        ExecutionFrame &caller,
        const String &name,
        const CompiledLayout::ConstBlockChainPtr &chain,
        std::size_t chainIndex,
        StringList &output,
        unit::CodeLocation location);

private:
    /// Shared remaining resource counts for one complete render operation.
    struct RuntimeBudget {
        uint64_t remainingInstructions{}; ///< Instructions still available to this complete render.
        uint64_t remainingOutputBytes{};  ///< Generated and captured output bytes still available.
    };

    ConstCompiledLayoutPtr _rootLayout;    ///< Most-derived generation rendered by this state.
    const Context &_globalContext;         ///< Global context snapshot.
    const Context &_localContext;          ///< Caller-provided local context.
    RenderLimits _limits;                  ///< Configured runtime limits.
    const Engine *_parent{};               ///< Parent include state, if this is nested.
    bool _withParentContext{false};        ///< Whether parent-visible values participate in lookup.
    std::size_t _depth{};                  ///< Number of include edges below the root.
    Context _renderContext;                ///< Values assigned by this inheritance render.
    std::vector<Context> _scopes;          ///< Innermost-last private iteration scopes.
    std::vector<LoopState> _loops;         ///< Innermost-last active iteration frames.
    std::vector<BlockCall> _activeBlocks;  ///< Active implementations used for recursion detection.
    StringList _rootOutput;                ///< Output owner used only by the root state.
    RenderCounters *_counters{};           ///< Optional profiler-owned work counters.
    RuntimeBudget _ownedBudget;            ///< Remaining budget storage owned by the root state.
    RuntimeBudget *_budget{&_ownedBudget}; ///< Shared budget for includes and captures.
    std::size_t _callFrameDepth{};         ///< Active setup, body, and block program calls.
};

}
