// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Compiler.hpp"

#include "CodeGenerator.hpp"
#include "DataCollector.hpp"

#include "../engine/ProgramWriter.hpp"
#include "../error/InternalError.hpp"
#include "../parser/Parser.hpp"

#include <cstddef>
#include <vector>

namespace erbsland::re::impl {

auto Compiler::buildEngine() -> EnginePtr {
    if (auto engine = tryBuildLiteralEngine()) {
        return engine;
    }
    parse();
    _engineData = std::make_shared<EngineData>();
    _engineData->counterCount = 0U;
    collectData();
    generateCode();
    return Engine::create(_engineData, _settings);
}

auto Compiler::tryBuildLiteralEngine() const -> EnginePtr {
    constexpr auto cMaximumSpeculativePatternLength = std::size_t{100U};
    if (_flags.isSet(GroupFlag::Verbose) || _flags.isSet(GroupFlag::Atomic)) {
        return {};
    }

    struct Alternative {
        std::size_t characterOffset;
        SequenceLength length;
        SequenceIndex sequenceOffset;
        ProgramCounter programLength;
    };

    // Scan a plain literal, a root alternative, or one non-capturing group with literal alternatives.
    auto reader = _reader;
    auto scannedCharacterCount = std::size_t{};
    auto scanLimitExceeded = false;
    const auto readCharacter = [&]() -> text::Char {
        auto character = reader.read();
        if (!character.isEndOfData()) {
            ++scannedCharacterCount;
            scanLimitExceeded = scannedCharacterCount > cMaximumSpeculativePatternLength ||
                scannedCharacterCount >= _settings.maximumPatternLength().toSizeT();
        }
        return character;
    };

    auto hasNonCapturingGroup = false;
    if (reader.peek() == U'(') {
        hasNonCapturingGroup = readCharacter() == U'(' && readCharacter() == U'?' && readCharacter() == U':';
        if (!hasNonCapturingGroup || scanLimitExceeded) {
            return {};
        }
    }

    auto characters = SequenceData{};
    auto alternatives = std::vector<Alternative>{};
    const auto finishAlternative = [&](const std::size_t offset) -> bool {
        const auto length = characters.size() - offset;
        if (length == 0U || alternatives.size() >= _settings.maximumAlternativeCount()) {
            return false;
        }
        alternatives.emplace_back(
            Alternative{
                .characterOffset = offset,
                .length = static_cast<SequenceLength>(length),
                .sequenceOffset = 0U,
                .programLength =
                    static_cast<ProgramCounter>(length <= limits::minimumCharacterSequenceLength ? length : 1U)});
        return true;
    };

    auto scanFinished = false;
    while (!scanFinished) {
        const auto alternativeOffset = characters.size();
        for (;;) {
            auto character = readCharacter();
            if (scanLimitExceeded) {
                return {};
            }
            if (character.isEndOfData()) {
                if (hasNonCapturingGroup || !finishAlternative(alternativeOffset)) {
                    return {};
                }
                scanFinished = true;
                break;
            }
            if (character == U'|') {
                if (!finishAlternative(alternativeOffset)) {
                    return {};
                }
                break;
            }
            if (hasNonCapturingGroup && character == U')') {
                if (!finishAlternative(alternativeOffset) || !readCharacter().isEndOfData()) {
                    return {};
                }
                scanFinished = true;
                break;
            }
            if (!character.isValidUnicode() || character.isNull()) {
                return {};
            }
            switch (character.toRawValue()) {
            case U'(':
            case U')':
            case U'\\':
            case U'[':
            case U']':
            case U'.':
            case U'^':
            case U'$':
            case U'*':
            case U'+':
            case U'?':
            case U'{':
            case U'}':
                return {};
            default:
                break;
            }
            if (_flags.isSet(GroupFlag::IgnoreCase)) {
                character = character.caseFolded();
            }
            characters.emplace_back(character);
        }
    }

    // Build sequence storage and calculate the program layout.
    auto data = EngineData{};
    data.counterCount = 0U;
    for (auto &alternative : alternatives) {
        if (alternative.length > limits::minimumCharacterSequenceLength) {
            alternative.sequenceOffset = static_cast<SequenceIndex>(data.sequenceData.size());
            const auto begin = characters.begin() + static_cast<std::ptrdiff_t>(alternative.characterOffset);
            data.sequenceData.insert(data.sequenceData.end(), begin, begin + alternative.length);
        }
    }

    const auto splitLength = (alternatives.size() - 1U) * 2U;
    auto branchStarts = std::vector<ProgramCounter>{};
    branchStarts.reserve(alternatives.size());
    auto programSize = splitLength;
    for (auto index = std::size_t{}; index < alternatives.size(); ++index) {
        branchStarts.emplace_back(static_cast<ProgramCounter>(programSize));
        programSize += alternatives[index].programLength + (index + 1U < alternatives.size() ? 1U : 0U);
    }
    const auto exit = static_cast<ProgramCounter>(programSize);

    // Generate the program for the collected alternatives.
    data.program.reserve(programSize + 1U);
    auto programCounter = ProgramCounter{};
    auto writer = ProgramWriter{data.program, programCounter};
    for (auto index = std::size_t{}; index + 1U < alternatives.size(); ++index) {
        const auto secondTarget = index + 2U == alternatives.size() ? branchStarts[index + 1U]
                                                                    : static_cast<ProgramCounter>(programCounter + 2U);
        writer.writeSplit(branchStarts[index], secondTarget);
    }
    for (auto index = std::size_t{}; index < alternatives.size(); ++index) {
        const auto &alternative = alternatives[index];
        if (alternative.length <= limits::minimumCharacterSequenceLength) {
            for (auto characterIndex = std::size_t{}; characterIndex < alternative.length; ++characterIndex) {
                const auto character = characters[alternative.characterOffset + characterIndex];
                if (_flags.isSet(GroupFlag::IgnoreCase)) {
                    writer.writeCiChar(character);
                } else {
                    writer.writeChar(character);
                }
            }
        } else if (_flags.isSet(GroupFlag::IgnoreCase)) {
            writer.writeCiSequence(alternative.sequenceOffset, alternative.length);
        } else {
            writer.writeSequence(alternative.sequenceOffset, alternative.length);
        }
        if (index + 1U < alternatives.size()) {
            writer.writeJump(exit);
        }
    }
    writer.writeMatch();
    return Engine::create(std::move(data), _settings);
}

void Compiler::parse() {
    Parser parser{_reader, _flags, _settings};
    _rootNode = parser.parse();
}

void Compiler::collectData() {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(_engineData != nullptr, "No engine data"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(_rootNode != nullptr, "No root node"_el);
    auto dataCollector = DataCollector{_rootNode, _engineData};
    dataCollector.collect();
}

void Compiler::generateCode() {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(_engineData != nullptr, "No engine data"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(_rootNode != nullptr, "No root node"_el);
    auto codeGenerator = CodeGenerator{_rootNode, _engineData};
    codeGenerator.generateCode();
}

}
