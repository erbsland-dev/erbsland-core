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

auto Engine::layoutAtLevel(const std::size_t level) const -> ConstCompiledLayoutPtr {
    if (level == 0U) {
        return _rootLayout;
    }
    if (level > _rootLayout->ancestors().size()) {
        throw ProgramError{"An inheritance layout level is outside the resolved ancestry."_el};
    }
    return _rootLayout->ancestors()[level - 1U];
}

void Engine::appendOutput(ExecutionFrame &frame, String text, const unit::CodeLocation location) {
    const auto byteCount = static_cast<uint64_t>(text.length().toSizeT());
    if (byteCount > _budget->remainingOutputBytes) {
        throw error(
            RenderErrorCategory::Limit,
            "Layout output limit exceeded"_el,
            "Generated and captured output exceeded the configured byte limit."_el,
            frame.layout,
            location);
    }
    _budget->remainingOutputBytes -= byteCount;
    frame.output.append(std::move(text));
}

void Engine::push(ExecutionFrame &frame, Value value, const unit::CodeLocation location) {
    if (frame.stack.size() >= _limits.valueStackDepth()) {
        throw error(
            RenderErrorCategory::Limit,
            "Layout value stack limit exceeded"_el,
            "The compiled layout exceeded the maximum value stack size."_el,
            frame.layout,
            location);
    }
    frame.stack.emplace_back(std::move(value));
}

void Engine::pushSuper(ExecutionFrame &frame, const std::size_t depth, const unit::CodeLocation location) {
    if (depth == 0U) {
        throw ProgramError{"A super request must skip at least one block implementation."_el};
    }
    if (!frame.blockCall.has_value()) {
        throw ProgramError{"A super request occurred outside a block program."_el};
    }
    if (frame.stack.size() >= _limits.valueStackDepth()) {
        throw error(
            RenderErrorCategory::Limit,
            "Layout value stack limit exceeded"_el,
            "The compiled layout exceeded the maximum value stack size."_el,
            frame.layout,
            location);
    }
    frame.stack.emplace_back(PendingSuper{depth});
}

auto Engine::takeTop(ExecutionFrame &frame, const Opcode opcode) -> StackEntry {
    if (frame.stack.empty()) {
        throw ProgramError{
            StringFormat{"The value stack is empty for opcode {}."_el}.build(static_cast<uint8_t>(opcode))};
    }
    auto result = std::move(frame.stack.back());
    frame.stack.pop_back();
    return result;
}

auto Engine::takeValue(ExecutionFrame &frame, const Opcode opcode, const unit::CodeLocation location) -> Value {
    auto entry = takeTop(frame, opcode);
    if (const auto *pending = std::get_if<PendingSuper>(&entry)) {
        return captureSuper(frame, pending->depth, location);
    }
    return resolve(std::get<Value>(std::move(entry)), frame.layout, location);
}

auto Engine::resolvedTop(ExecutionFrame &frame, const unit::CodeLocation location) -> Value & {
    if (frame.stack.empty()) {
        throw ProgramError{"The value stack is empty for a unary operation."_el};
    }
    if (const auto *pending = std::get_if<PendingSuper>(&frame.stack.back())) {
        frame.stack.back() = captureSuper(frame, pending->depth, location);
    }
    auto &value = std::get<Value>(frame.stack.back());
    value = resolve(std::move(value), frame.layout, location);
    return value;
}

auto Engine::captureSuper(ExecutionFrame &frame, const std::size_t depth, const unit::CodeLocation location) -> Value {
    auto output = StringList{};
    executeSuper(frame, depth, output, location, true);
    return Value{output.join()};
}

auto Engine::constant(const ExecutionFrame &frame, const uint64_t index, const String &kind) -> String {
    if (index >= frame.layout->constantCount().toSizeT()) {
        throw ProgramError{StringFormat{"A {} constant index is outside the constant pool."_el}.build(kind)};
    }
    return frame.layout->constant(unit::ItemIndex::fromSizeT(static_cast<std::size_t>(index)));
}

auto Engine::lookup(const ExecutionFrame &frame, const String &name, const unit::CodeLocation location) const -> Value {
    auto found = false;
    auto value = findVisible(name, found);
    return resolve(std::move(value), frame.layout, location);
}

auto Engine::findVisible(const String &name, bool &found) const -> Value {
    for (auto iterator = _scopes.rbegin(); iterator != _scopes.rend(); ++iterator) {
        if (iterator->contains(name)) {
            found = true;
            return iterator->get(name);
        }
    }
    if (_renderContext.contains(name)) {
        found = true;
        return _renderContext.get(name);
    }
    if (_parent != nullptr && _withParentContext) {
        auto value = _parent->findVisible(name, found);
        if (found) {
            return value;
        }
    } else if (_parent == nullptr && _localContext.contains(name)) {
        found = true;
        return _localContext.get(name);
    }
    if (_globalContext.contains(name)) {
        found = true;
        return _globalContext.get(name);
    }
    return {};
}

auto Engine::resolve(Value value, const ConstCompiledLayoutPtr &layout, const unit::CodeLocation location) const
    -> Value {
    for (auto count = std::size_t{0U}; value.isCallback(); ++count) {
        if (count >= _limits.callbackDepth()) {
            throw error(
                RenderErrorCategory::Limit,
                "Layout callback limit exceeded"_el,
                "The callback resolution depth exceeded the configured maximum."_el,
                layout,
                location);
        }
        try {
            value = value.evaluate();
        } catch (const RenderError &) {
            throw;
        } catch (...) {
            throw error(
                RenderErrorCategory::Runtime,
                "Layout value callback failed"_el,
                "A lazy value callback raised an exception."_el,
                layout,
                location,
                std::current_exception());
        }
    }
    return value;
}

auto Engine::error(
    const RenderErrorCategory category,
    String title,
    String description,
    const ConstCompiledLayoutPtr &layout,
    const unit::CodeLocation location,
    const std::exception_ptr cause) const -> RenderError {
    return RenderError{
        RenderErrorContext{category, std::move(title), std::move(description)}
            .setLayout(layout->name())
            .setOrigin(layout->source().origin())
            .setLocation(location),
        cause};
}

}
