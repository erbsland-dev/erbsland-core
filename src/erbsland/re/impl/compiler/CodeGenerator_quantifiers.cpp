// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeGenerator.hpp"

#include "../engine/ProgramWriter.hpp"
#include "../error/InternalError.hpp"

namespace erbsland::re::impl {

using node_data::Quantifier;

void CodeGenerator::generateCodeForData(const PatternNode &node, const Quantifier &data) {
    auto &segment = createSegment(node);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        node.children().size() == 1, "Quantifiers can only be applied to single child node"_el);
    const auto segmentId = node.children()[0]->id();
    const auto &childProgram = _segments.at(segmentId);
    if (childProgram.size() == 0) { // sanity logic / should not happen
        _segments.erase(segmentId);
        return;
    }
    ProgramCounter programCounter = 0;
    ProgramWriter writer(segment, programCounter);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        data.maximum >= data.minimum, "Maximum must be greater than or equal to minimum"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(data.maximum > 0, "Maximum must be greater than zero"_el);
    if (data.mode == Quantifier::Mode::Possessive) {
        writer.writeStartAtomic(data.atomicGroupId);
    }
    if (data.minimum == data.maximum) {
        generateFixedCount(writer, data, childProgram);
    } else if (data.minimum == 0) {
        if (data.maximum == 1) {
            generateZeroOrOne(writer, data, childProgram);
        } else if (data.maximum == Quantifier::infinitelyMany()) {
            generateZeroOrMore(writer, data, childProgram);
        } else {
            generateZeroToMaximum(writer, data, childProgram); // using MAXIMUM
        }
    } else if (data.minimum == 1) {
        if (data.maximum == Quantifier::infinitelyMany()) {
            generateOneOrMore(writer, data, childProgram);
        } else {
            generateOneToMaximum(writer, data, childProgram);
        }
    } else {
        if (data.maximum == Quantifier::infinitelyMany()) {
            generateMinimumToMany(writer, data, childProgram);
        } else {
            generateMinimumMaximum(writer, data, childProgram); // using MAXIMUM/MINIMUM
        }
    }
    if (data.mode == Quantifier::Mode::Possessive) {
        writer.writeStopAtomic(data.atomicGroupId);
    }
    _segments.erase(segmentId);
}

void CodeGenerator::generateFixedCount(ProgramWriter &writer, const Quantifier &data, const Program &childProgram) {

    // The parser should already resolve the special case 1-1.
    // A fixed count must have at least two repetitions and min==max.
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(data.minimum >= 2 && data.maximum >= 2, "Invalid fixed count quantifier"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        data.minimum == data.maximum, "Fixed count quantifier must have equal minimum and maximum"_el);
    // Ignore all modes for fixed count.
    // loop:                ... ; child program
    //                      SKIP MAXIMUM <counter>, <maximum> - 1
    //                      JUMP %loop
    // end:                 ...
    const auto jumpDelta = -(static_cast<RelativeJump>(childProgram.size()) + 1);
    writer.writeProgram(childProgram);
    writer.writeSkipIfMaximum(data.counterIndex, data.maximum - 1);
    writer.writeJump(createDeltaJump(jumpDelta));
}

void CodeGenerator::generateZeroOrOne(ProgramWriter &writer, const Quantifier &data, const Program &childProgram) {

    constexpr RelativeJump childProgramDelta = 2; // SPLIT = 2 ops
    const auto skipDelta = childProgramDelta + static_cast<RelativeJump>(childProgram.size());
    if (data.mode != Quantifier::Mode::Lazy) {    // greedy and possessive
        //                  SPLIT %childProgram, %skip
        // childProgram:    ; ...
        //                  ; ...
        // skip:            ; ...
        writer.writeSplit(createDeltaJump(childProgramDelta), createDeltaJump(skipDelta));
    } else { // lazy
        //                  SPLIT %skip, %childProgram
        // childProgram:    ; ...
        //                  ; ...
        // skip:            ; ...
        writer.writeSplit(createDeltaJump(skipDelta), createDeltaJump(childProgramDelta));
    }
    writer.writeProgram(childProgram);
}

void CodeGenerator::generateZeroOrMore(ProgramWriter &writer, const Quantifier &data, const Program &childProgram) {

    constexpr RelativeJump childProgramDelta = 2; // SPLIT = 2 ops
    const auto skipDelta = childProgramDelta + static_cast<RelativeJump>(childProgram.size()) + 1;
    const auto loopDelta = -(static_cast<RelativeJump>(childProgram.size()) + 2);
    if (data.mode != Quantifier::Mode::Lazy) { // greedy and possessive
        // loop:            SPLIT %childProgram, %skip
        // childProgram:    ; ...
        //                  ; ...
        //                  JUMP %loop
        // skip:            ; ...
        writer.writeSplit(createDeltaJump(childProgramDelta), createDeltaJump(skipDelta));
    } else { // lazy
        // loop:            SPLIT %skip, %childProgram
        // childProgram:    ; ...
        //                  ; ...
        //                  JUMP %loop
        // skip:            ; ...
        writer.writeSplit(createDeltaJump(skipDelta), createDeltaJump(childProgramDelta));
    }
    writer.writeProgram(childProgram);
    writer.writeJump(createDeltaJump(loopDelta));
}

void CodeGenerator::generateOneOrMore(ProgramWriter &writer, const Quantifier &data, const Program &childProgram) {

    const auto loopDelta = -static_cast<RelativeJump>(childProgram.size());
    constexpr auto endDelta = 2;               // SPLIT = 2 ops
    writer.writeProgram(childProgram);
    if (data.mode != Quantifier::Mode::Lazy) { // greedy and possessive
        // loop:            ; ... child program ...
        //                  ; ...
        //                  SPLIT %loop, %end
        // end:             ; ...
        writer.writeSplit(createDeltaJump(loopDelta), createDeltaJump(endDelta));
    } else { // lazy
        // loop:            ; ... child program ...
        //                  ; ...
        //                  SPLIT %end, %loop
        // end:             ; ...
        writer.writeSplit(createDeltaJump(endDelta), createDeltaJump(loopDelta));
    }
}

void CodeGenerator::generateZeroToMaximum(ProgramWriter &writer, const Quantifier &data, const Program &childProgram) {

    ERBSLAND_CORE_RE_REQUIRE_SAFETY(data.minimum == 0, "Minimum must be zero"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        data.maximum != Quantifier::infinitelyMany(), "Maximum must be less than infinity"_el);

    const auto loopDelta = -(static_cast<RelativeJump>(childProgram.size()) + 3);
    const auto endDelta = static_cast<RelativeJump>(childProgram.size()) + 4;
    const auto nextDelta = 2U;
    if (data.mode != Quantifier::Mode::Lazy) { // greedy and possessive
        // loop:            SPLIT %next, %end
        // next:            ; ... child program ...
        //                  ; ...
        //                  MAXIMUM <counter>, <maximum>
        //                  JUMP %loop
        // end:             COUNTER <counter>, 0
        //                  ; ...
        writer.writeSplit(createDeltaJump(nextDelta), createDeltaJump(endDelta));
        writer.writeProgram(childProgram);
        writer.writeMaximum(data.counterIndex, data.maximum);
        writer.writeJump(createDeltaJump(loopDelta));
        writer.writeCounter(data.counterIndex, 0);
    } else { // lazy
        // loop:            SPLIT %end, %next
        // next:            ; ... child program ...
        //                  ; ...
        //                  MAXIMUM <counter>, <maximum>
        //                  JUMP %loop
        // end:             COUNTER <counter>, 0
        //                  ; ...
        writer.writeSplit(createDeltaJump(endDelta), createDeltaJump(nextDelta));
        writer.writeProgram(childProgram);
        writer.writeMaximum(data.counterIndex, data.maximum);
        writer.writeJump(createDeltaJump(loopDelta));
        writer.writeCounter(data.counterIndex, 0);
    }
}

void CodeGenerator::generateOneToMaximum(ProgramWriter &writer, const Quantifier &data, const Program &childProgram) {

    ERBSLAND_CORE_RE_REQUIRE_SAFETY(data.minimum == 1, "Minimum must be one"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        data.maximum != Quantifier::infinitelyMany(), "Maximum must be less than infinity"_el);

    const auto loopDelta = -(static_cast<RelativeJump>(childProgram.size()) + 1);
    const auto endDelta = 2U;
    if (data.mode != Quantifier::Mode::Lazy) { // greedy and possessive
        // loop:            ; ... child program ...
        //                  ; ...
        //                  MAXIMUM <counter>, <maximum>
        //                  SPLIT %loop, %end
        // end:             COUNTER <counter>, 0
        //                  ; ...
        writer.writeProgram(childProgram);
        writer.writeMaximum(data.counterIndex, data.maximum);
        writer.writeSplit(createDeltaJump(loopDelta), createDeltaJump(endDelta));
        writer.writeCounter(data.counterIndex, 0);
    } else { // lazy
        // loop:            ; ... child program ...
        //                  ; ...
        //                  MAXIMUM <counter>, <maximum>
        //                  SPLIT %end, %loop
        // end:             COUNTER <counter>, 0
        //                  ; ...
        writer.writeProgram(childProgram);
        writer.writeMaximum(data.counterIndex, data.maximum);
        writer.writeSplit(createDeltaJump(endDelta), createDeltaJump(loopDelta));
        writer.writeCounter(data.counterIndex, 0);
    }
}

void CodeGenerator::generateMinimumMaximum(ProgramWriter &writer, const Quantifier &data, const Program &childProgram) {

    ERBSLAND_CORE_RE_REQUIRE_SAFETY(data.minimum > 1, "Minimum must be greater than one"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        data.maximum != Quantifier::infinitelyMany(), "Maximum must be less than infinity"_el);

    if (data.mode != Quantifier::Mode::Lazy) { // greedy and possessive
        const auto loopDelta = -(static_cast<RelativeJump>(childProgram.size()) + 1);
        constexpr auto endDelta = 2;
        // loop:            ; ... child program ...
        //                  ; ...
        //                  MAXIMUM <counter>, <maximum>
        //                  SPLIT %loop, %end
        // end:             MINIMUM <counter>, <minimum> ; resets counter
        writer.writeProgram(childProgram);
        writer.writeMaximum(data.counterIndex, data.maximum);
        writer.writeSplit(createDeltaJump(loopDelta), createDeltaJump(endDelta));
    } else { // lazy
        const auto loopDelta = -(static_cast<RelativeJump>(childProgram.size()) + 3);
        const auto endDelta = static_cast<RelativeJump>(childProgram.size()) + 4;
        constexpr auto repeatDelta = 2;
        // loop:            SPLIT %end, %repeat
        // repeat:          ; ... child program ...
        //                  ; ...
        //                  MAXIMUM <counter>, <maximum>
        //                  JUMP %loop
        // end:             MINIMUM <counter>, <minimum> ; resets counter
        writer.writeSplit(createDeltaJump(endDelta), createDeltaJump(repeatDelta));
        writer.writeProgram(childProgram);
        writer.writeMaximum(data.counterIndex, data.maximum);
        writer.writeJump(createDeltaJump(loopDelta));
    }
    writer.writeMinimum(data.counterIndex, data.minimum);
}

void CodeGenerator::generateMinimumToMany(ProgramWriter &writer, const Quantifier &data, const Program &childProgram) {

    ERBSLAND_CORE_RE_REQUIRE_SAFETY(data.minimum > 1, "Minimum must be greater than one"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(data.maximum == Quantifier::infinitelyMany(), "Maximum must be infinity"_el);

    const auto loopDelta = -(static_cast<RelativeJump>(childProgram.size()) + 1);
    constexpr auto endDelta = 2;
    if (data.mode != Quantifier::Mode::Lazy) { // greedy and possessive
        // loop:            ; ... child program ...
        //                  ; ...
        //                  ADD COUNTER <counter>, 1
        //                  SPLIT %loop, %end
        // end:             MINIMUM <counter>, <minimum> ; resets counter
        writer.writeProgram(childProgram);
        writer.writeAddCounter(data.counterIndex, 1);
        writer.writeSplit(createDeltaJump(loopDelta), createDeltaJump(endDelta));
    } else { // lazy
        // loop:            ; ... child program ...
        //                  ; ...
        //                  ADD COUNTER <counter>, 1
        //                  SPLIT %end, %loop
        // end:             MINIMUM <counter>, <minimum> ; resets counter
        writer.writeProgram(childProgram);
        writer.writeAddCounter(data.counterIndex, 1);
        writer.writeSplit(createDeltaJump(endDelta), createDeltaJump(loopDelta));
    }
    writer.writeMinimum(data.counterIndex, data.minimum);
}

}
