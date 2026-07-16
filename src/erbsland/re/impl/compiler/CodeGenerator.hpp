// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../engine/Engine.hpp"
#include "../parser/PatternNode.hpp"

#include <utility>

namespace erbsland::re::impl {

class ProgramWriter;

// NOTES:
// What if we traverse leaf to root?
// - no front back, as groups and sequences are assembled on the fly.
// - each node assembles the whole code.
//   - sequence and group are able also generate the required JUMP's *between* the nodes.
// Negatives:
// - How to get the jump locations right?
//   is it a problem? leaf nodes only require relative jumps.
//   a sequence requires zero jumps between code segments.
//   a group only jumps to the end of the group - which can be calculated (size of all code segments + jumps between).
//   => This would require just relative jump offsets - no complicated front/back jumps.

/// Generates optimized code from a pattern node tree.
class CodeGenerator {
    /// Info about a group.
    struct GroupInfo {
        GroupFlags flags;     ///< The flags.
        PatternNodeId nodeId; ///< The node ID.
        int level;            ///< The level in the node tree.
    };

    /// The type for a relative jump
    using RelativeJump = int32_t;

public:
    /// Create a new instance of the code generator.
    explicit CodeGenerator(PatternNodePtr patternNode, EngineDataPtr engineData);

    /// Create an empty code generator, just for compatibility and tests.
    CodeGenerator() = default;

    // defaults: allow move, disallow copy.
    CodeGenerator(const CodeGenerator &) = delete;
    CodeGenerator(CodeGenerator &&) = default;
    auto operator=(const CodeGenerator &) -> CodeGenerator & = delete;
    auto operator=(CodeGenerator &&) -> CodeGenerator & = default;
    ~CodeGenerator() = default;

public:
    /// Generate optimized code from the pattern node tree.
    /// The generated program is stored in the engine data supplied to the constructor.
    void generateCode();

private:
    // Generate the code fragments for a given type of data.
    void generateCodeForData(const PatternNode &node, const node_data::Group &data);
    void generateCodeForData(const PatternNode &node, const node_data::Sequence &data);
    void generateCodeForData(const PatternNode &node, const node_data::Anchor &data);
    void generateCodeForData(const PatternNode &node, const node_data::CharacterCategory &data);
    void generateCodeForData(const PatternNode &node, const node_data::CharacterSequence &data);
    void generateCodeForData(const PatternNode &node, const node_data::CharacterClass &data);
    void generateCodeForData(const PatternNode &node, const node_data::Quantifier &data);

    void generateFixedCount(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    void generateZeroOrOne(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    void generateZeroOrMore(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    void generateOneOrMore(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    void generateZeroToMaximum(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    void generateOneToMaximum(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    void generateMinimumMaximum(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    void generateMinimumToMany(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);

    void resolveJumps();

    /// Create a new segment for the given node.
    [[nodiscard]] auto createSegment(const PatternNode &node) -> Program &;

    /// Access the currently active group.
    [[nodiscard]] auto currentGroup() const noexcept -> const GroupInfo & { return _groupStack.back(); }

    /// Create a relative jump location.
    [[nodiscard]] auto createDeltaJump(RelativeJump delta) -> ProgramCounter;

private:
    PatternNodePtr _rootNode;           ///< The root node from the parser.
    EngineDataPtr _engineData;          ///< The engine data with all collected sequences and character classes.
    std::vector<GroupInfo> _groupStack; ///< A stack of group info.
    std::unordered_map<PatternNodeId, Program> _segments; ///< Prepared program segments.
    /// In the first pass, any written program counter is just a relative jump location from this map.
    std::unordered_map<ProgramCounter, RelativeJump> _jumpLocations;
};

}
