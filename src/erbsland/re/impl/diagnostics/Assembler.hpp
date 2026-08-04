// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Argument.hpp"
#include "AssemblerToken.hpp"
#include "LabelTarget.hpp"
#include "LabelTargetWithSource.hpp"
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
/// @tested{AssemblerBasicTest AssemblerFlowTest}
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
        ProgramCounter programCounter{}; ///< Program position for the label.
        SequenceIndex sequenceIndex{};   ///< Sequence index for the label.
        bool sequenceValid{false};       ///< If the sequence index has been set.
        CharClassIndex charClassIndex{}; ///< Character-class index for the label.
        bool charClassValid{false};      ///< If the character-class index has been set.
    };

public:
    /// Create a new assembler instance.
    Assembler();

    // defaults
    ~Assembler() = default;

public:
    /// Compile the given lines into engine data.
    /// @return The engine data created by the assembler code.
    /// @throws RegExError on any parsing or compilation error.
    [[nodiscard]] auto compile(const text::StringList &lines) -> EngineDataPtr;

private:
    /// Patch all forward references to resolved label offsets.
    void patchLabelOffsets();
    /// Process one assembler source line.
    void processLine(const text::String &line);
    /// Tokenize one assembler source line.
    void tokenizeLine(const text::String &line);
    /// Test whether the current line has an unread token.
    [[nodiscard]] auto hasCurrentToken() const noexcept -> bool;
    /// Get the current unread token.
    [[nodiscard]] auto currentToken() const noexcept -> const AssemblerToken &;
    /// Advance to the next token in the current line.
    void nextToken() noexcept;
    /// Process an operation source line.
    void processOperationLine();

    /// Process an assembler command line.
    void processCommandLine();
    /// Process a section-selection command.
    void processSectionCommand();
    /// Process a data-definition command.
    void processDataCommand();
    /// Process a capture-group count command.
    void processGroupsCommand();
    /// Process a capture-group name command.
    void processGroupCommand();
    /// Process a character-class definition command.
    void processClassCommand();

    /// Close the current class and convert `_charRanges` to a new character class.
    void closeCurrentClass();

    /// Add the label declared on the current line.
    void addLabel();

    /// Process one operation and its arguments.
    void processOperation(Operation operation, const AssemblerTokens &arguments);

    /// Process a split operation.
    void processSplit(const AssemblerTokens &arguments);
    /// Process a jump operation.
    void processJump(const AssemblerTokens &arguments);
    /// Process an anchor operation.
    void processAnchor(const AssemblerTokens &arguments);
    /// Process a capture operation.
    void processCapture(const AssemblerTokens &arguments, bool isNegated);
    /// Process an atomic-group operation.
    void processAtomic(const AssemblerTokens &arguments, bool isNegated);
    /// Process a counter operation.
    void processCounter(const AssemblerTokens &arguments);
    /// Process an add-counter operation.
    void processAddCounter(const AssemblerTokens &arguments);
    /// Process a maximum-counter operation.
    void processMaximum(const AssemblerTokens &arguments);
    /// Process a conditional maximum-counter operation.
    void processSkipIfMaximum(const AssemblerTokens &arguments);
    /// Process a minimum-counter operation.
    void processMinimum(const AssemblerTokens &arguments);
    /// Process a character-matching operation.
    void processChar(const AssemblerTokens &arguments, bool isCaseInsensitive, bool isNegated);
    /// Process a sequence-matching operation.
    void processSequence(const AssemblerTokens &arguments, bool isCaseInsensitive);
    /// Process a character-category matching operation.
    void processCategory(const AssemblerTokens &arguments, bool isNegated);
    /// Process a character-category assertion operation.
    void processAssertCategory(const AssemblerTokens &arguments, bool isNegated);
    /// Process a character-class matching operation.
    void processClass(const AssemblerTokens &arguments, bool isCaseInsensitive, bool isNegated);

    /// Validate an operation's arguments against its definition.
    void validateArgumentTypes(Operation operation, const AssemblerTokens &arguments) const;
    /// Test whether an argument matches a definition.
    [[nodiscard]] static auto doesArgumentMatch(
        const AssemblerToken &argument, const ArgumentDefinition &definition) noexcept -> bool;

    /// Read an identifier argument or report an assembler error.
    [[nodiscard]] auto expectIdentifier(const AssemblerToken &argument) const -> text::String;
    /// Read a counter-index argument or report an assembler error.
    [[nodiscard]] auto expectCounterIndex(const AssemblerToken &argument) const -> ArgumentIndex;
    /// Read a counter-value argument or report an assembler error.
    [[nodiscard]] auto expectCounterValue(const AssemblerToken &argument) const -> uint16_t;
    /// Read a character argument or report an assembler error.
    [[nodiscard]] auto expectChar(const AssemblerToken &argument) const -> text::Char;
    /// Read a capture-group argument or report an assembler error.
    [[nodiscard]] auto expectCaptureGroup(const AssemblerToken &argument) const -> std::size_t;
    /// Read an atomic-group identifier or report an assembler error.
    [[nodiscard]] auto expectAtomicGroupId(const AssemblerToken &argument) const -> AtomicGroupId;
    /// Read a program-counter argument or report an assembler error.
    [[nodiscard]] auto expectProgramCounter(const AssemblerToken &argument, ArgumentIndex argumentIndex = 0)
        -> ProgramCounter;
    /// Read a character-class index or report an assembler error.
    [[nodiscard]] auto expectCharClassIndex(const AssemblerToken &argument) -> CharClassIndex;
    /// Read a sequence index or report an assembler error.
    [[nodiscard]] auto expectSequenceIndex(const AssemblerToken &argument) -> SequenceIndex;
    /// Read a sequence length or report an assembler error.
    [[nodiscard]] auto expectSequenceLength(const AssemblerToken &argument) -> SequenceLength;
    /// Throw an assembler error at the current source location.
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
