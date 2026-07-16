// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeGenerator.hpp"

#include "../error/InternalError.hpp"

namespace erbsland::re::impl {

using namespace text::literals;

CodeGenerator::CodeGenerator(PatternNodePtr patternNode, EngineDataPtr engineData) :
    _rootNode{std::move(patternNode)}, _engineData{std::move(engineData)} {
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
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(_segments.size() == 1, "Expected exactly one segment"_el);
    _engineData->program = _segments.at(_rootNode->id());
    if (_engineData->program.size() > limits::maximumProgramLength) {
        throw RegExError{
            ErrorCategory::Limit,
            "Failed to compile regular expression"_el,
            "The generated program exceeds the maximum program length."_el};
    }
    resolveJumps();
}

auto CodeGenerator::createSegment(const PatternNode &node) -> Program & {
    auto [it, inserted] = _segments.try_emplace(node.id());
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(inserted, "Segment already exists"_el);
    return it->second;
}

auto CodeGenerator::createDeltaJump(int32_t delta) -> ProgramCounter {
    auto result = static_cast<ProgramCounter>(_jumpLocations.size());
    _jumpLocations.emplace(result, delta);
    return result;
}

}
