// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Argument.hpp"
#include "AssemblerToken.hpp"
#include "LabelTarget.hpp"
#include "OperationData.hpp"

#include "../engine/EngineData.hpp"
#include "../engine/Program.hpp"
#include "../engine/ProgramWriter.hpp"

#include "../../../text/Literals.hpp"
#include "../../../text/String.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringHashMap.hpp"
#include "../../../text/StringList.hpp"
#include "../../../unit/CodeLocation.hpp"
#include "../../RegExError.hpp"

#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

namespace erbsland::re::impl {

using namespace text::literals;

/// The implementation of the assembler.
///
class Assembler {
    /// The maximum length of labels (and keywords).
    constexpr static std::size_t maxLabelLength = 16U;

    /// A patch that needs to be applied to the program after parsing.
    struct LabelOffsetPath {
        unit::LineIndex sourceLine;       ///< The zero-based source line index.
        text::String label;               ///< The expected label name.
        DataSection section;              ///< The expected label section.
        ProgramCounter operationPosition; ///< The position of the operation to patch.
        ArgumentIndex argumentIndex;      ///< The argument index for patching.
    };

    /// All relevant positions for a label.
    struct LabelOffsets {
        ProgramCounter programCounter{};
        SequenceIndex sequenceIndex{};
        bool sequenceValid{false};
        CharClassIndex charClassIndex{};
        bool charClassValid{false};
    };

public:
    /// Create a new assembler instance.
    Assembler();
    ~Assembler() = default;

public:
    /// Compile the given lines into engine data.
    /// @return The engine data created by the assembler code.
    /// @throws RegExError on any parsing or compilation error.
    [[nodiscard]] auto compile(const text::StringList &lines) -> EngineDataPtr;

private:
    void patchLabelOffsets();
    void processLine(const text::String &line);
    void tokenizeLine(const text::String &line);
    [[nodiscard]] auto hasCurrentToken() const noexcept -> bool;
    [[nodiscard]] auto currentToken() const noexcept -> const AssemblerToken &;
    void nextToken() noexcept;
    void processOperationLine();

    void processCommandLine();
    void processSectionCommand();
    void processDataCommand();
    void processGroupsCommand();
    void processGroupCommand();
    void processClassCommand();

    /// Close the current class and convert `_charRanges` to a new character class.
    void closeCurrentClass();

    void addLabel();

    void processOperation(Operation operation, const AssemblerTokens &arguments);

    void processSplit(const AssemblerTokens &arguments);
    void processJump(const AssemblerTokens &arguments);
    void processAnchor(const AssemblerTokens &arguments);
    void processCapture(const AssemblerTokens &arguments, bool isNegated);
    void processAtomic(const AssemblerTokens &arguments, bool isNegated);
    void processCounter(const AssemblerTokens &arguments);
    void processAddCounter(const AssemblerTokens &arguments);
    void processMaximum(const AssemblerTokens &arguments);
    void processSkipIfMaximum(const AssemblerTokens &arguments);
    void processMinimum(const AssemblerTokens &arguments);
    void processChar(const AssemblerTokens &arguments, bool isCaseInsensitive, bool isNegated);
    void processSequence(const AssemblerTokens &arguments, bool isCaseInsensitive);
    void processCategory(const AssemblerTokens &arguments, bool isNegated);
    void processAssertCategory(const AssemblerTokens &arguments, bool isNegated);
    void processClass(const AssemblerTokens &arguments, bool isCaseInsensitive, bool isNegated);

    void validateArgumentTypes(Operation operation, const AssemblerTokens &arguments) const;
    [[nodiscard]] static auto doesArgumentMatch(
        const AssemblerToken &argument, const ArgumentDefinition &definition) noexcept -> bool;

    [[nodiscard]] auto expectIdentifier(const AssemblerToken &argument) const -> text::String;
    [[nodiscard]] auto expectCounterIndex(const AssemblerToken &argument) const -> ArgumentIndex;
    [[nodiscard]] auto expectCounterValue(const AssemblerToken &argument) const -> uint16_t;
    [[nodiscard]] auto expectChar(const AssemblerToken &argument) const -> text::Char;
    [[nodiscard]] auto expectCaptureGroup(const AssemblerToken &argument) const -> std::size_t;
    [[nodiscard]] auto expectAtomicGroupId(const AssemblerToken &argument) const -> AtomicGroupId;
    [[nodiscard]] auto expectProgramCounter(const AssemblerToken &argument, ArgumentIndex argumentIndex = 0)
        -> ProgramCounter;
    [[nodiscard]] auto expectCharClassIndex(const AssemblerToken &argument) -> CharClassIndex;
    [[nodiscard]] auto expectSequenceIndex(const AssemblerToken &argument) -> SequenceIndex;
    [[nodiscard]] auto expectSequenceLength(const AssemblerToken &argument) -> SequenceLength;
    [[noreturn]] void throwAssemblerError(
        text::String description, unit::ColumnIndex column = unit::ColumnIndex::noIndex()) const {
        throw RegExError{
            ErrorCategory::Assembler,
            "Failed to assemble regular expression"_el,
            std::move(description),
            unit::CodeLocation{_lineIndex, column}};
    }

private:
    EngineDataPtr _data;                                ///< The engine data that is being created by the assembler.
    ProgramCounter _programCounter{0};                  ///< The program counter for the program section.
    ProgramWriter _writer;                              ///< The program writer.

    unit::LineIndex _lineIndex;                         ///< The current zero-based line index in the source code.
    AssemblerTokens _tokens;                            ///< The tokens for the current line.
    std::size_t _tokenIndex{0};                         ///< The index of the current read token.

    text::String _currentLabel;                         ///< The label on this line.
    LabelOffsets _labelOffsets;                         ///< The label offsets for the current label.
    text::StringHashMap<LabelTargetWithSource> _labels; ///< Labels that need to be resolved after parsing.
    std::vector<LabelOffsetPath> _patches;              ///< Label offsets that need to be patched after parsing.

    DataSection _currentSection{DataSection::Program};  ///< The current data section that is being processed.
    std::set<OperationModifier> _modifiers;             ///< Modifiers in the current line.
    CaptureGroupNames _captureGroupNames;               ///< Defines the number of capture groups and their names.
    bool _captureGroupSizeSet{false};                   ///< Indicates if the capture group size has been set.
    bool _isClassOpen{false};                           ///< If we got a `.class` command.
    std::vector<CharRange> _charRanges;                 ///< The collected char ranges.
};

}
