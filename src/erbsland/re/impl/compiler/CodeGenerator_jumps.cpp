// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CodeGenerator.hpp"

#include "../engine/ProgramReader.hpp"
#include "../engine/ProgramWriter.hpp"

namespace erbsland::re::impl {

void CodeGenerator::resolveJumps() {
    const auto programSize = static_cast<ProgramCounter>(_engineData->program.size());
    ProgramCounter readCounter = 0U;
    ProgramReader reader(_engineData->program);
    while (readCounter < programSize) {
        const auto operation = reader.peekOperation(readCounter);
        const auto opLocation = readCounter;
        if (operation == Operation::Jump) {
            auto jumpLocation = reader.readJump(readCounter); // advances by 1 code unit
            const auto absoluteLocation = static_cast<ProgramCounter>(opLocation + _jumpLocations.at(jumpLocation));
            ProgramCounter patchCounter = opLocation;
            ProgramWriter writer(_engineData->program, patchCounter);
            writer.patchOffset(absoluteLocation, 0U);
        } else if (operation == Operation::Split) {
            auto [programCounterA, programCounterB] = reader.readSplit(readCounter); // advances by 2 code units
            const auto absoluteLocationA = static_cast<ProgramCounter>(opLocation + _jumpLocations.at(programCounterA));
            const auto absoluteLocationB = static_cast<ProgramCounter>(opLocation + _jumpLocations.at(programCounterB));
            ProgramCounter patchCounterA = opLocation;
            ProgramWriter writerA(_engineData->program, patchCounterA);
            writerA.patchOffset(absoluteLocationA, 0U);
            ProgramCounter patchCounterB = opLocation;
            ProgramWriter writerB(_engineData->program, patchCounterB);
            writerB.patchOffset(absoluteLocationB, 1U);
        } else {
            reader.skipOperation(readCounter);
        }
    }
}

}
