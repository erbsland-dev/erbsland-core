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

Engine::Engine(
    ConstCompiledLayoutPtr layout, const Context &globalContext, const Context &localContext, RenderLimits limits) :
    _rootLayout{std::move(layout)},
    _globalContext{globalContext},
    _localContext{localContext},
    _limits{std::move(limits)},
    _ownedBudget{_limits.executedInstructions(), _limits.generatedOutputBytes()} {
    if (_rootLayout == nullptr) {
        throw ProgramError{"A null compiled layout was passed to the engine."_el};
    }
}

Engine::Engine(
    ConstCompiledLayoutPtr layout,
    const Context &globalContext,
    const Context &localContext,
    RenderCounters &counters,
    RenderLimits limits) :
    Engine{std::move(layout), globalContext, localContext, std::move(limits)} {
    _counters = &counters;
}

Engine::Engine(
    ConstCompiledLayoutPtr layout,
    const Context &globalContext,
    const Context &localContext,
    const Engine &parent,
    const bool withParentContext,
    const std::size_t depth,
    RenderCounters *counters) :
    _rootLayout{std::move(layout)},
    _globalContext{globalContext},
    _localContext{localContext},
    _limits{parent._limits},
    _parent{&parent},
    _withParentContext{withParentContext},
    _depth{depth},
    _counters{counters},
    _budget{parent._budget} {
}

auto Engine::render() -> String {
    executeLayout(_rootOutput);
    return _rootOutput.join();
}

void Engine::executeLayout(StringList &output) {
    for (auto level = _rootLayout->ancestors().size() + 1U; level > 0U; --level) {
        const auto layout = layoutAtLevel(level - 1U);
        if (!layout->setupProgram().data().isEmpty()) {
            executeProgram(layout, layout->setupProgram(), output);
        }
    }
    const auto base = layoutAtLevel(_rootLayout->ancestors().size());
    executeProgram(base, base->bodyProgram(), output);
}

void Engine::executeProgram(
    const ConstCompiledLayoutPtr &layout,
    const Program &program,
    StringList &output,
    std::optional<BlockCall> blockCall) {
    if (_callFrameDepth >= _limits.callFrameDepth()) {
        throw error(
            RenderErrorCategory::Limit,
            "Layout call-frame limit exceeded"_el,
            "The render exceeded the configured call-frame depth."_el,
            layout,
            {});
    }
    ++_callFrameDepth;
    auto frame = ExecutionFrame{layout, program, output, std::move(blockCall), _loops.size(), _scopes.size()};
    try {
        while (!frame.reader.isAtEnd()) {
            const auto instruction = frame.reader.read();
            const auto opcode = instruction.opcode();
            const auto location = instruction.location();
            if (_budget->remainingInstructions == 0U) {
                throw error(
                    RenderErrorCategory::Limit,
                    "Layout instruction limit exceeded"_el,
                    "The render exceeded the configured instruction count."_el,
                    frame.layout,
                    location);
            }
            --_budget->remainingInstructions;
            switch (opcode) {
            case Opcode::EmitText:
                appendOutput(frame, constant(frame, instruction.operand(), "text"_el), location);
                break;
            case Opcode::LoadName:
                push(frame, lookup(frame, constant(frame, instruction.operand(), "name"_el), location), location);
                break;
            case Opcode::GetMember: {
                auto &value = resolvedTop(frame, location);
                value = resolve(value.get(constant(frame, instruction.operand(), "member"_el)), frame.layout, location);
                break;
            }
            case Opcode::EmitValue:
            case Opcode::EmitEscapedValue: {
                if (opcode == Opcode::EmitEscapedValue &&
                    instruction.operand() >= static_cast<uint64_t>(EscapeFormat::_valueCount)) {
                    throw ProgramError{"An EmitValue instruction contains an invalid escape format."_el};
                }
                const auto format = opcode == Opcode::EmitValue
                    ? EscapeFormat{EscapeFormat::None}
                    : EscapeFormat{static_cast<EscapeFormat::Value>(instruction.operand())};
                auto entry = takeTop(frame, opcode);
                if (const auto *pending = std::get_if<PendingSuper>(&entry)) {
                    executeSuper(frame, pending->depth, frame.output, location, false);
                    break;
                }
                auto value = resolve(std::get<Value>(std::move(entry)), frame.layout, location);
                if (value.isList() || value.isMap()) {
                    throw error(
                        RenderErrorCategory::Runtime,
                        "Container cannot be rendered"_el,
                        "Lists and maps cannot be rendered directly."_el,
                        frame.layout,
                        location);
                }
                auto text = value.toString();
                if (format != EscapeFormat::None) {
                    text = text.toEscaped(format);
                }
                appendOutput(frame, std::move(text), location);
                break;
            }
            case Opcode::PushTrue:
                push(frame, Value{true}, location);
                break;
            case Opcode::PushFalse:
                push(frame, Value{false}, location);
                break;
            case Opcode::PushNull:
                push(frame, Value{}, location);
                break;
            case Opcode::PushInteger:
                push(frame, Value{std::bit_cast<int64_t>(instruction.operand())}, location);
                break;
            case Opcode::PushFloat:
                push(frame, Value{std::bit_cast<double>(instruction.operand())}, location);
                break;
            case Opcode::PushText:
                push(frame, Value{constant(frame, instruction.operand(), "text"_el)}, location);
                break;
            case Opcode::LogicalNot: {
                auto &value = resolvedTop(frame, location);
                value = Value{!value.isTruthy()};
                break;
            }
            case Opcode::UnaryPlus:
            case Opcode::UnaryMinus:
                executeUnary(frame, opcode, location);
                break;
            case Opcode::Add:
            case Opcode::Subtract:
            case Opcode::Multiply:
            case Opcode::Divide:
                executeArithmetic(frame, opcode, location);
                break;
            case Opcode::Concatenate:
                executeConcatenation(frame, location);
                break;
            case Opcode::BuildList:
            case Opcode::BuildMap:
                buildCollection(frame, opcode, instruction.operand(), location);
                break;
            case Opcode::In:
            case Opcode::NotIn:
                executeMembership(frame, opcode == Opcode::NotIn, location);
                break;
            case Opcode::Is:
            case Opcode::IsNot:
                executeTest(frame, instruction.operand(), opcode == Opcode::IsNot, location);
                break;
            case Opcode::Equal:
            case Opcode::NotEqual:
            case Opcode::Greater:
            case Opcode::GreaterEqual:
            case Opcode::Less:
            case Opcode::LessEqual:
                executeComparison(frame, opcode, location);
                break;
            case Opcode::ApplyApplicationFilter:
            case Opcode::ApplyBuiltInFilter:
                applyFilter(frame, instruction.operand(), opcode == Opcode::ApplyBuiltInFilter, location);
                break;
            case Opcode::StoreName:
                if (_scopes.empty()) {
                    _renderContext.set(
                        constant(frame, instruction.operand(), "name"_el), takeValue(frame, opcode, location));
                } else {
                    _scopes.back().set(
                        constant(frame, instruction.operand(), "name"_el), takeValue(frame, opcode, location));
                }
                break;
            case Opcode::StoreScopedName:
                if (_scopes.size() <= frame.scopeBase || _loops.size() <= frame.loopBase ||
                    !_loops.back().hasActiveScope()) {
                    throw ProgramError{"A scoped-name store has no active iteration scope in this program."_el};
                }
                _scopes.back().set(
                    constant(frame, instruction.operand(), "name"_el), takeValue(frame, opcode, location));
                break;
            case Opcode::Jump:
                frame.reader.jumpTo(instruction.operand());
                break;
            case Opcode::JumpIfFalse: {
                const auto value = takeValue(frame, opcode, location);
                if (!value.isTruthy()) {
                    frame.reader.jumpTo(instruction.operand());
                }
                break;
            }
            case Opcode::JumpIfFalseOrPop: {
                auto &value = resolvedTop(frame, location);
                if (!value.isTruthy()) {
                    frame.reader.jumpTo(instruction.operand());
                } else {
                    frame.stack.pop_back();
                }
                break;
            }
            case Opcode::JumpIfTrueOrPop: {
                auto &value = resolvedTop(frame, location);
                if (value.isTruthy()) {
                    frame.reader.jumpTo(instruction.operand());
                } else {
                    frame.stack.pop_back();
                }
                break;
            }
            case Opcode::BeginListIteration:
                beginIteration(frame, false, location);
                break;
            case Opcode::BeginMapIteration:
                beginIteration(frame, true, location);
                break;
            case Opcode::NextIteration:
                nextIteration(frame, instruction.operand(), location);
                break;
            case Opcode::EndIteration:
                endIteration(frame);
                break;
            case Opcode::EndIterationIfNotEmpty:
                if (_loops.size() <= frame.loopBase || _loops.back().hasActiveScope()) {
                    throw ProgramError{"A loop-else instruction has no exhausted iteration frame."_el};
                }
                {
                    const auto hadItems = _loops.back().hadItems();
                    _loops.pop_back();
                    if (hadItems) {
                        frame.reader.jumpTo(instruction.operand());
                    }
                }
                break;
            case Opcode::Include:
                executeInclude(frame, instruction.operand(), location);
                break;
            case Opcode::RenderBlock:
                executeBlock(frame, constant(frame, instruction.operand(), "block name"_el), location);
                break;
            case Opcode::LoadSuper:
                pushSuper(frame, static_cast<std::size_t>(instruction.operand()), location);
                break;
            case Opcode::End:
                if (!frame.stack.empty()) {
                    throw ProgramError{"The value stack is not empty at the end of the program."_el};
                }
                if (!frame.reader.isAtEnd()) {
                    throw ProgramError{"The program contains data after its End instruction."_el};
                }
                if (_loops.size() != frame.loopBase || _scopes.size() != frame.scopeBase) {
                    throw ProgramError{"The program ended with unbalanced iteration state."_el};
                }
                --_callFrameDepth;
                return;
            }
        }
        throw ProgramError{"The program ended without an End instruction."_el};
    } catch (const RenderError &error) {
        --_callFrameDepth;
        if (error.context().layout() != layout->name()) {
            throw;
        }
        auto layoutLevel = std::size_t{};
        while (layoutAtLevel(layoutLevel) != layout) {
            ++layoutLevel;
            if (layoutLevel > _rootLayout->ancestors().size()) {
                throw;
            }
        }
        auto context = error.context();
        for (auto level = layoutLevel; level > 0U; --level) {
            const auto owner = layoutAtLevel(level - 1U);
            if (owner->extendsDependency() == nullptr) {
                throw ProgramError{"A resolved inheritance edge has no parent dependency."_el};
            }
            context.addOuterFrame(
                StringFormat{"{}:{}"_el}.build(owner->name(), owner->extendsDependency()->location().toString()));
        }
        throw RenderError{std::move(context), error.cause()};
    } catch (...) {
        --_callFrameDepth;
        throw;
    }
}

}
