// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../engine/Engine.hpp"
#include "../engine/ProgramWriter_fwd.hpp"
#include "../parser/PatternNode.hpp"

#include <utility>

namespace erbsland::re::impl {

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

    // defaults/deletions
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
    /// Generate code for a group node.
    void generateCodeForData(const PatternNode &node, const node_data::Group &data);
    /// Generate code for a sequence node.
    void generateCodeForData(const PatternNode &node, const node_data::Sequence &data);
    /// Generate code for an anchor node.
    void generateCodeForData(const PatternNode &node, const node_data::Anchor &data);
    /// Generate code for a character category node.
    void generateCodeForData(const PatternNode &node, const node_data::CharacterCategory &data);
    /// Generate code for a character sequence node.
    void generateCodeForData(const PatternNode &node, const node_data::CharacterSequence &data);
    /// Generate code for a character class node.
    void generateCodeForData(const PatternNode &node, const node_data::CharacterClass &data);
    /// Generate code for a quantifier node.
    void generateCodeForData(const PatternNode &node, const node_data::Quantifier &data);

    /// Generate code for a fixed quantifier count.
    void generateFixedCount(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    /// Generate code for an optional child program.
    void generateZeroOrOne(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    /// Generate code for an unbounded zero-minimum quantifier.
    void generateZeroOrMore(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    /// Generate code for an unbounded one-minimum quantifier.
    void generateOneOrMore(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    /// Generate code for a zero-to-maximum quantifier.
    void generateZeroToMaximum(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    /// Generate code for a one-to-maximum quantifier.
    void generateOneToMaximum(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    /// Generate code for a finite minimum-to-maximum quantifier.
    void generateMinimumMaximum(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);
    /// Generate code for an unbounded minimum quantifier.
    void generateMinimumToMany(ProgramWriter &writer, const node_data::Quantifier &data, const Program &childProgram);

    /// Resolve relative jump locations in generated programs.
    void resolveJumps();

    /// Create a new segment for the given node.
    [[nodiscard]] auto createSegment(const PatternNode &node) -> Program &;
    /// Access an active node segment.
    [[nodiscard]] auto getSegment(PatternNodeId nodeId) -> Program &;
    /// Mark a merged node segment as inactive.
    void releaseSegment(PatternNodeId nodeId);

    /// Access the currently active group.
    [[nodiscard]] auto currentGroup() const noexcept -> const GroupInfo & { return _groupStack.back(); }

    /// Create a relative jump location.
    [[nodiscard]] auto createDeltaJump(RelativeJump delta) -> ProgramCounter;

private:
    PatternNodePtr _rootNode;           ///< The root node from the parser.
    EngineDataPtr _engineData;          ///< The engine data with all collected sequences and character classes.
    std::vector<GroupInfo> _groupStack; ///< A stack of group info.
    std::vector<Program> _segments;     ///< Prepared program segments, indexed by dense node ID.
    std::vector<bool> _activeSegments;  ///< Marks program segments that have not been merged yet.
    std::size_t _activeSegmentCount{};  ///< Number of active program segments.
    /// In the first pass, any written program counter is just a relative jump location from this map.
    std::vector<RelativeJump> _jumpLocations;
};

}
