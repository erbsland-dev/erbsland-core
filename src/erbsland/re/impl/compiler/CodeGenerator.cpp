// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeGenerator.hpp"

#include "../error/InternalError.hpp"

namespace erbsland::re::impl {

using namespace text::literals;

CodeGenerator::CodeGenerator(PatternNodePtr patternNode, EngineDataPtr engineData) :
    _rootNode{std::move(patternNode)}, _engineData{std::move(engineData)} {

    // Most expressions fit into these buffers. Keeping their capacity avoids repeated growth while
    // leaf segments and nested groups are discovered during traversal.
    _groupStack.reserve(16U);
    _segments.reserve(32U);
    _activeSegments.reserve(32U);
    _jumpLocations.reserve(16U);
}

void CodeGenerator::generateCode() {
    _rootNode->traverse(
        [this](const PatternNode &node, int level) -> void {
            // root-to-leaf: maintain the group stack for all children.
            if (node.isGroup()) {
                const auto &data = std::get<node_data::Group>(node.data());
                _groupStack.emplace_back(GroupInfo{data.flags, node.id(), level});
            }
        },
        [this](const PatternNode &node, int level) -> void {
            // leaf-to-root: generate code.
            std::visit(
                [this, &node]<typename T>(const T &data) -> void { generateCodeForData(node, data); }, node.data());

            // unwind the group stack.
            if (node.isGroup()) {
                ERBSLAND_CORE_RE_REQUIRE_SAFETY(!_groupStack.empty(), "Group stack underflow"_el);
                ERBSLAND_CORE_RE_REQUIRE_SAFETY(_groupStack.back().level == level, "Unexpected group stack level"_el);
                _groupStack.pop_back();
            }
        });
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(_activeSegmentCount == 1, "Expected exactly one segment"_el);
    _engineData->program = std::move(getSegment(_rootNode->id()));
    if (_engineData->program.size() > limits::maximumProgramLength) {
        throw RegExError{
            ErrorCategory::Limit,
            "Failed to compile regular expression"_el,
            "The generated program exceeds the maximum program length."_el};
    }
    resolveJumps();
}

auto CodeGenerator::createSegment(const PatternNode &node) -> Program & {
    const auto index = static_cast<std::size_t>(node.id());
    if (index >= _segments.size()) {
        _segments.resize(index + 1U);
        _activeSegments.resize(index + 1U, false);
    }
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(!_activeSegments[index], "Segment already exists"_el);
    _activeSegments[index] = true;
    ++_activeSegmentCount;
    return _segments[index];
}

auto CodeGenerator::getSegment(const PatternNodeId nodeId) -> Program & {
    const auto index = static_cast<std::size_t>(nodeId);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(index < _segments.size() && _activeSegments[index], "Segment does not exist"_el);
    return _segments[index];
}

void CodeGenerator::releaseSegment(const PatternNodeId nodeId) {
    const auto index = static_cast<std::size_t>(nodeId);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(index < _segments.size() && _activeSegments[index], "Segment does not exist"_el);
    _activeSegments[index] = false;
    --_activeSegmentCount;
}

auto CodeGenerator::createDeltaJump(int32_t delta) -> ProgramCounter {
    auto result = static_cast<ProgramCounter>(_jumpLocations.size());
    _jumpLocations.emplace_back(delta);
    return result;
}

}
