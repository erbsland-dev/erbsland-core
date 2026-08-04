// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeGenerator.hpp"

#include "../engine/ProgramWriter.hpp"
#include "../error/InternalError.hpp"

namespace erbsland::re::impl {

void CodeGenerator::generateCodeForData(const PatternNode &node, const node_data::Group &data) {
    auto &segment = createSegment(node);
    // The group must never be empty, but the embedded sequence can be empty.
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        !data.children().empty(), "Group must have at least one child node (a sequence)"_el);
    const auto childrenCount = static_cast<int32_t>(data.children().size());
    const auto hasAdoptedChild = childrenCount == 1 && data.index == 0 && data.atomicGroupId == cNoAtomicGroupId;
    if (hasAdoptedChild) {
        const auto childNodeId = data.children().front()->id();
        segment = std::move(getSegment(childNodeId));
        releaseSegment(childNodeId);
    }
    ProgramCounter programCounter = static_cast<ProgramCounter>(segment.size());
    ProgramWriter writer(segment, programCounter);
    if (data.index != 0) {
        // The node index is 1-based (0 = no capture).
        // The VM uses 0-based indexing for actual capture groups.
        writer.writeStartCapture(static_cast<CaptureGroupIndex>(data.index - 1));
    }
    if (data.atomicGroupId != cNoAtomicGroupId) {
        writer.writeStartAtomic(data.atomicGroupId);
    }
    if (childrenCount == 1 && !hasAdoptedChild) {
        const auto childNodeId = data.children()[0]->id();
        const auto wrapperLength = static_cast<std::size_t>(data.index != 0) * 2U +
            static_cast<std::size_t>(data.atomicGroupId != cNoAtomicGroupId) * 2U +
            static_cast<std::size_t>(node.id() == _rootNode->id());
        segment.reserve(getSegment(childNodeId).size() + wrapperLength);
        writer.writeProgram(getSegment(childNodeId));
        releaseSegment(childNodeId);
    } else if (childrenCount > 1) {
        // Generate code for alternation using forward-chained SPLITs.
        //
        // split0:           SPLIT alt0, split1
        // split1:           SPLIT alt1, split2
        // ...
        // splitN-2:         SPLIT altN-2, altN-1
        // alt0:             ... ; program 0
        //                   JUMP exit
        // alt1:             ... ; program 1
        //                   JUMP exit
        // ...
        // altN-2:           ...
        //                   JUMP exit
        // altN-1:           ... ; program N-1
        // exit:             ... ; MATCH for the root node
        //
        // IMPORTANT: Deltas are relative to the operation location in code units.

        const auto childCount = static_cast<RelativeJump>(data.children().size());
        const auto splitCount = childCount - 1;
        const auto splitLength = splitCount * 2; // each SPLIT = 2 code units

        // Pre-calculate absolute program counters for each alternative (in the merged segment).
        struct ProgramData {
            RelativeJump begin;
            RelativeJump programEnd;
            PatternNodeId nodeId;
            const Program &program;
        };
        std::vector<ProgramData> programData;
        programData.reserve(static_cast<std::size_t>(childCount));
        auto currentAbsolute = splitLength;
        const auto &childNodes = data.children();
        for (auto i = 0; i < childCount; ++i) {
            const auto childNodeId = childNodes[static_cast<std::size_t>(i)]->id();
            const auto &program = getSegment(childNodeId);
            const auto begin = currentAbsolute;
            currentAbsolute += static_cast<RelativeJump>(program.size());
            const auto programEnd = currentAbsolute;
            if (i != (childCount - 1)) {
                currentAbsolute += 1; // JUMP after each alternative except the last.
            }
            programData.emplace_back(begin, programEnd, childNodeId, program);
        }
        const auto exitAbsolute = currentAbsolute;

        const auto wrapperLength = static_cast<std::size_t>(data.index != 0) * 2U +
            static_cast<std::size_t>(data.atomicGroupId != cNoAtomicGroupId) * 2U +
            static_cast<std::size_t>(node.id() == _rootNode->id());
        segment.reserve(static_cast<std::size_t>(exitAbsolute) + wrapperLength);

        // Write the SPLIT chain.
        for (auto i = 0; i < splitCount; ++i) {
            const auto splitAbsolute = i * 2;
            const auto deltaToAltA = programData[static_cast<std::size_t>(i)].begin - splitAbsolute;
            if (i == (splitCount - 1)) {
                const auto deltaToAltB = programData[static_cast<std::size_t>(i + 1)].begin - splitAbsolute;
                writer.writeSplit(createDeltaJump(deltaToAltA), createDeltaJump(deltaToAltB));
            } else {
                // Next SPLIT is the next operation, so delta = 2.
                writer.writeSplit(createDeltaJump(deltaToAltA), createDeltaJump(2));
            }
        }

        // Write the alternative programs.
        for (std::size_t i = 0; i < static_cast<std::size_t>(childCount); ++i) {
            const auto &d = programData[i];
            writer.writeProgram(d.program);
            if (i != static_cast<std::size_t>(childCount - 1)) {
                // Jump operation is located after the program.
                const auto jumpDelta = exitAbsolute - d.programEnd;
                writer.writeJump(createDeltaJump(jumpDelta));
            }
        }

        // Erase all included program segments.
        for (const auto &d : programData) {
            releaseSegment(d.nodeId);
        }
    }
    if (data.atomicGroupId != cNoAtomicGroupId) {
        writer.writeStopAtomic(data.atomicGroupId);
    }
    if (data.index != 0) {
        // The node index is 1-based (0 = no capture).
        // The VM uses 0-based indexing for actual capture groups.
        writer.writeStopCapture(static_cast<CaptureGroupIndex>(data.index - 1));
    }
    if (node.id() == _rootNode->id()) {
        writer.writeMatch();
    }
}

void CodeGenerator::generateCodeForData(const PatternNode &node, const node_data::Sequence &data) {
    auto &segment = createSegment(node);
    if (data.children().empty()) {
        return;
    }
    auto totalSize = std::size_t{};
    for (const auto &child : data.children()) {
        totalSize += getSegment(child->id()).size();
    }
    const auto firstChildNodeId = data.children().front()->id();
    segment = std::move(getSegment(firstChildNodeId));
    releaseSegment(firstChildNodeId);
    segment.reserve(totalSize);
    ProgramCounter programCounter = static_cast<ProgramCounter>(segment.size());
    ProgramWriter writer(segment, programCounter);
    for (const auto &child : data.children().subspan(1U)) {
        const auto childNodeId = child->id();
        writer.writeProgram(getSegment(childNodeId));
        releaseSegment(childNodeId);
    }
}

void CodeGenerator::generateCodeForData(const PatternNode &node, const node_data::Anchor &data) {
    auto &segment = createSegment(node);
    ProgramCounter programCounter = 0;
    ProgramWriter writer(segment, programCounter);
    writer.writeAnchor(data.textAnchor);
}

void CodeGenerator::generateCodeForData(const PatternNode &node, const node_data::CharacterCategory &data) {
    auto &segment = createSegment(node);
    ProgramCounter programCounter = 0;
    ProgramWriter writer(segment, programCounter);
    const auto categoryCount = data.categories.size();
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(categoryCount > 0, "No categories specified"_el);
    if (data.isNegated) {
        // Implement a negated class of categories as a sequence of `NOT CATEGORY`,
        // followed by `NOT ASSERT CATEGORY`.
        for (std::size_t i = 0; i < data.categories.size(); ++i) {
            const auto category = data.categories[i];
            if (i == 0) {
                writer.writeNotCategory(category);
            } else {
                writer.writeNotAssertCategory(category);
            }
        }
    } else {
        if (categoryCount == 1) {
            const auto category = data.categories.front();
            if (category == Category::Any) {
                writer.writeNotChar(U'\n');
            } else if (category == Category::AnyDotAll) {
                writer.writeAny();
            } else {
                writer.writeCategory(data.categories.front());
            }
        } else {
            // Code for 2 categories
            // start: SPLIT %cat + 0, %cat + 2
            // cat:   CATEGORY &xxx
            //        JUMP %end
            //        CATEGORY &yyy
            // end:   ...
            //
            // Code for 3 categories
            // start: SPLIT %cat + 0, %start + 2
            //        SPLIT %cat + 2, %cat + 4
            // cat:   CATEGORY &xxx
            //        JUMP %end
            //        CATEGORY &yyy
            //        JUMP %end
            //        CATEGORY &zzz
            // end:   ...
            //
            // Add another SPLIT for each additional category.
            //
            // Important: jump deltas are relative to the program counter where the operation is stored.
            // E.g. for a `SPLIT` (2 code units) a delta of `+2` points to the next operation.
            const auto splitCount = static_cast<RelativeJump>(categoryCount) - 1;
            const auto splitLength = splitCount * 2;                                  // each SPLIT = 2 ops.
            const auto endAbsolute =
                splitLength + (static_cast<RelativeJump>(categoryCount) - 1) * 2 + 1; // no jump for last category.

            const auto categoryStartAbsolute = [&](RelativeJump index) -> RelativeJump {
                return splitLength + index * 2; // CATEGORY + JUMP for all but the last.
            };

            // write the splits
            for (auto i = 0; i < splitCount; ++i) {
                const auto splitAbsolute = i * 2;
                const auto deltaToCategoryA = categoryStartAbsolute(i) - splitAbsolute;
                if (i == (splitCount - 1)) { // last split branches to the last 2 categories
                    const auto deltaToCategoryB = categoryStartAbsolute(i + 1) - splitAbsolute;
                    writer.writeSplit(createDeltaJump(deltaToCategoryA), createDeltaJump(deltaToCategoryB));
                } else {
                    // next split is exactly the next operation, so delta = 2 (SPLIT = 2 code units).
                    writer.writeSplit(createDeltaJump(deltaToCategoryA), createDeltaJump(2));
                }
            }

            // write the categories
            for (auto i = 0U; i < data.categories.size(); ++i) {
                writer.writeCategory(data.categories[i]);
                if (i != (data.categories.size() - 1)) {
                    const auto jumpAbsolute = categoryStartAbsolute(static_cast<RelativeJump>(i)) + 1;
                    writer.writeJump(createDeltaJump(endAbsolute - jumpAbsolute));
                }
            }
        }
    }
}

void CodeGenerator::generateCodeForData(const PatternNode &node, const node_data::CharacterSequence &data) {
    auto &segment = createSegment(node);
    const auto groupFlags = currentGroup().flags;
    ProgramCounter programCounter = 0;
    ProgramWriter writer(segment, programCounter);
    if (data.dataIndex >= 0) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            static_cast<std::size_t>(data.dataIndex) < _engineData->sequenceData.size(),
            "Invalid character sequence index"_el);

        const auto isIgnoreCase = groupFlags.isSet(GroupFlag::IgnoreCase);
        auto offset = static_cast<SequenceIndex>(data.dataIndex);
        auto remainingLength = static_cast<SequenceLength>(data.chars.size());

        while (remainingLength > 0) {
            const auto chunkLength = static_cast<SequenceLength>(std::min<std::size_t>(remainingLength, 0xFFU));
            if (isIgnoreCase) {
                writer.writeCiSequence(offset, chunkLength);
            } else {
                writer.writeSequence(offset, chunkLength);
            }
            offset += chunkLength;
            remainingLength -= chunkLength;
        }
    } else {
        if (groupFlags.isSet(GroupFlag::IgnoreCase)) {
            for (auto c : data.chars) {
                writer.writeCiChar(c);
            }
        } else {
            for (auto c : data.chars) {
                writer.writeChar(c);
            }
        }
    }
}

void CodeGenerator::generateCodeForData(const PatternNode &node, const node_data::CharacterClass &data) {
    auto &segment = createSegment(node);
    const auto groupFlags = currentGroup().flags;
    ProgramCounter programCounter = 0;
    ProgramWriter writer(segment, programCounter);
    if (data.dataIndex < 0 && data.characterClass.isSingleChar()) {
        auto character = data.characterClass.ranges().front().first();
        if (data.isNegated) {
            if (groupFlags.isSet(GroupFlag::IgnoreCase)) {
                writer.writeNotCiChar(character);
            } else {
                writer.writeNotChar(character);
            }
        } else {
            if (groupFlags.isSet(GroupFlag::IgnoreCase)) {
                writer.writeCiChar(character);
            } else {
                writer.writeChar(character);
            }
        }
        return;
    }
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(data.dataIndex >= 0, "Invalid character class index"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        data.dataIndex < static_cast<int>(_engineData->charClassData.size()), "Invalid character class index"_el);
    if (data.isNegated) {
        if (groupFlags.isSet(GroupFlag::IgnoreCase)) {
            writer.writeNotCiClass(static_cast<CharClassIndex>(data.dataIndex));
        } else {
            writer.writeNotClass(static_cast<CharClassIndex>(data.dataIndex));
        }
    } else {
        if (groupFlags.isSet(GroupFlag::IgnoreCase)) {
            writer.writeCiClass(static_cast<CharClassIndex>(data.dataIndex));
        } else {
            writer.writeClass(static_cast<CharClassIndex>(data.dataIndex));
        }
    }
}

}
