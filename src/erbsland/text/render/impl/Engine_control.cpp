// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Engine.hpp"

#include "BuiltInFilters.hpp"
#include "CompiledBlock.hpp"
#include "CompiledExtends.hpp"
#include "CompiledInclude.hpp"
#include "CompiledLayout.hpp"
#include "ProgramError.hpp"
#include "ValueComparison.hpp"
#include "ValueData.hpp"
#include "ValueTest.hpp"

#include "../../../math/SaturatingMath.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"

#include <algorithm>
#include <bit>
#include <cmath>

namespace erbsland::text::render::impl {

using namespace text::literals;

void Engine::beginIteration(ExecutionFrame &frame, const bool expectMap, const unit::CodeLocation location) {
    if (_loops.size() >= _limits.nestingDepth()) {
        throw error(
            RenderErrorCategory::Limit,
            "Layout nesting limit exceeded"_el,
            "The render exceeded the configured nesting depth."_el,
            frame.layout,
            location);
    }
    auto iterable = takeValue(frame, expectMap ? Opcode::BeginMapIteration : Opcode::BeginListIteration, location);
    if (expectMap && !iterable.isMap()) {
        throw error(
            RenderErrorCategory::Runtime,
            "Map iteration requires a map"_el,
            "A two-target 'for' statement can iterate only an ordered map."_el,
            frame.layout,
            location);
    }
    if (!expectMap && !iterable.isList()) {
        throw error(
            RenderErrorCategory::Runtime,
            "List iteration requires a list"_el,
            "A one-target 'for' statement can iterate only a list."_el,
            frame.layout,
            location);
    }
    _loops.emplace_back(std::move(iterable));
}

void Engine::nextIteration(ExecutionFrame &frame, const uint64_t exitTarget, const unit::CodeLocation location) {
    if (_loops.size() <= frame.loopBase) {
        throw ProgramError{"An iteration advance has no active iteration frame in this program."_el};
    }
    auto &loop = _loops.back();
    removeIterationScope(loop, frame);
    if (!loop.advance()) {
        frame.reader.jumpTo(exitTarget);
        return;
    }
    if (_scopes.size() >= _limits.lexicalScopeDepth()) {
        throw error(
            RenderErrorCategory::Limit,
            "Layout scope limit exceeded"_el,
            "The render exceeded the configured lexical scope depth."_el,
            frame.layout,
            location);
    }
    _scopes.emplace_back();
    loop.setActiveScope(true);
    _scopes.back().set("loop"_el, loop.metadata());
    if (loop.isList()) {
        push(frame, loop.listValue(), location);
    } else {
        push(frame, Value{loop.mapKey()}, location);
        push(frame, loop.mapValue(), location);
    }
}

void Engine::endIteration(const ExecutionFrame &frame) {
    if (_loops.size() <= frame.loopBase) {
        throw ProgramError{"An iteration end has no active iteration frame in this program."_el};
    }
    if (_loops.back().hasActiveScope()) {
        throw ProgramError{"An iteration ended before its current scope was advanced."_el};
    }
    _loops.pop_back();
}

void Engine::removeIterationScope(LoopState &loop, const ExecutionFrame &frame) {
    if (!loop.hasActiveScope()) {
        return;
    }
    if (_scopes.size() <= frame.scopeBase) {
        throw ProgramError{"An active iteration has no matching execution scope in this program."_el};
    }
    _scopes.pop_back();
    loop.setActiveScope(false);
}

void Engine::executeInclude(ExecutionFrame &frame, const uint64_t index, const unit::CodeLocation location) {
    if (_counters != nullptr) {
        ++_counters->includeExecutions;
    }
    const auto dependency = frame.layout->include(static_cast<std::size_t>(index));
    if (dependency == nullptr) {
        throw ProgramError{"An include operand is outside the dependency table."_el};
    }
    if (dependency->layout() == nullptr) {
        if (!dependency->ignoreMissing()) {
            throw ProgramError{"A required include has no bound layout generation."_el};
        }
        return;
    }
    if (_depth >= _limits.staticDependencyDepth()) {
        throw error(
            RenderErrorCategory::Limit,
            "Layout include depth exceeded"_el,
            "The render exceeded the configured static dependency depth."_el,
            frame.layout,
            location);
    }
    try {
        auto child = Engine{
            dependency->layout(),
            _globalContext,
            _localContext,
            *this,
            dependency->withContext(),
            _depth + 1U,
            _counters};
        child.executeLayout(frame.output);
    } catch (const RenderError &error) {
        auto context = error.context();
        context.addOuterFrame(StringFormat{"{}:{}"_el}.build(frame.layout->name(), location.toString()));
        throw RenderError{std::move(context), error.cause()};
    }
}

void Engine::executeBlock(ExecutionFrame &frame, const String &name, const unit::CodeLocation location) {
    const auto chain = _rootLayout->blockChain(name);
    if (chain == nullptr) {
        throw ProgramError{"A block dispatch references an unknown implementation."_el};
    }
    executeBlockImplementation(frame, name, chain, 0U, frame.output, location);
}

void Engine::executeSuper(
    ExecutionFrame &frame,
    const std::size_t depth,
    StringList &output,
    const unit::CodeLocation location,
    const bool captured) {
    if (!frame.blockCall.has_value()) {
        throw ProgramError{"A super request occurred outside a block program."_el};
    }
    const auto target = frame.blockCall->chainIndex + depth;
    if (target >= frame.blockCall->chain->size()) {
        throw error(
            RenderErrorCategory::Runtime,
            "No parent block implementation"_el,
            "The requested super block depth has no implementation."_el,
            frame.layout,
            location);
    }
    if (_counters != nullptr) {
        ++_counters->superExecutions;
        if (captured) {
            ++_counters->capturedSuperExecutions;
        }
    }
    executeBlockImplementation(frame, frame.blockCall->name, frame.blockCall->chain, target, output, location);
}

void Engine::executeBlockImplementation(
    ExecutionFrame &caller,
    const String &name,
    const CompiledLayout::ConstBlockChainPtr &chain,
    const std::size_t chainIndex,
    StringList &output,
    const unit::CodeLocation location) {
    if (chain == nullptr || chainIndex >= chain->size()) {
        throw ProgramError{"A block dispatch references an unknown implementation."_el};
    }
    if (_activeBlocks.size() >= _limits.nestingDepth()) {
        throw error(
            RenderErrorCategory::Limit,
            "Layout nesting limit exceeded"_el,
            "The render exceeded the configured nesting depth."_el,
            caller.layout,
            location);
    }
    if (std::ranges::any_of(_activeBlocks, [&](const BlockCall &call) -> bool {
            return call.chain == chain && call.chainIndex == chainIndex;
        })) {
        throw error(
            RenderErrorCategory::Runtime,
            "Recursive block dispatch"_el,
            "The resolved nested block graph contains a recursive dispatch cycle."_el,
            caller.layout,
            location);
    }
    const auto &implementation = (*chain)[chainIndex];
    const auto layout = layoutAtLevel(implementation.layoutLevel);
    if (_counters != nullptr) {
        ++_counters->blockExecutions;
    }
    _activeBlocks.emplace_back(BlockCall{name, chain, chainIndex});
    try {
        executeProgram(layout, implementation.block->program(), output, BlockCall{name, chain, chainIndex});
        _activeBlocks.pop_back();
    } catch (const RenderError &error) {
        _activeBlocks.pop_back();
        auto context = error.context();
        context.addOuterFrame(StringFormat{"{}:{}"_el}.build(caller.layout->name(), location.toString()));
        throw RenderError{std::move(context), error.cause()};
    } catch (...) {
        _activeBlocks.pop_back();
        throw;
    }
}

}
