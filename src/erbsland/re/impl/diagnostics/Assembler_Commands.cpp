// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Assembler.hpp"

#include "../Limits.hpp"

#include "../../../text/StringFormat.hpp"

#include <set>

namespace erbsland::re::impl {

using namespace text::literals;

void Assembler::processCommandLine() {
    const auto &command = currentToken().getText();
    nextToken();
    if (command == "section"_el) {
        processSectionCommand();
    } else if (command == "data"_el) {
        processDataCommand();
    } else if (command == "groups"_el) {
        processGroupsCommand();
    } else if (command == "group"_el) {
        processGroupCommand();
    } else if (command == "class"_el) {
        processClassCommand();
    } else {
        throwAssemblerError(text::StringFormat{"Unknown command '{}'"}.build(command), currentToken().column());
    }
}

void Assembler::processSectionCommand() {
    // Switches the current section.
    // .section &program
    if (!hasCurrentToken() || !currentToken().isIdentifier()) {
        throwAssemblerError("Expected section identifier after the command"_el);
    }
    const auto &section = currentToken().getText();
    if (section != "program"_el && section != "sequence"_el && section != "class"_el) {
        throwAssemblerError(
            text::StringFormat{"Invalid section identifier '{}'"}.build(currentToken().getText()),
            currentToken().column());
    }
    closeCurrentClass(); // close any previously started character class.
    const auto dataSection = toDataSection(currentToken().getText());
    _currentSection = dataSection;
    nextToken();
    if (hasCurrentToken()) {
        throwAssemblerError("Unexpected token after section identifier"_el, currentToken().column());
    }
}

void Assembler::processDataCommand() {
    // Writes data into a sequence section.
    // .data "text" or .data 'a' or .data 123 or .data $12ab
    if (_currentSection != DataSection::Sequence && _currentSection != DataSection::Class) {
        throwAssemblerError("The '.data' command can only be used in a sequence section"_el);
    }
    if (!hasCurrentToken()) {
        throwAssemblerError("Expected the data after the '.data' command"_el);
    }
    if (!(currentToken().isText() || currentToken().isChar() || currentToken().isInteger() ||
            currentToken().isOffset())) {

        throwAssemblerError("Expected a text, char, integer or offset expression after the '.data' command"_el);
    }
    if (_currentSection == DataSection::Sequence) {
        if (currentToken().isText()) {
            static_cast<void>(currentToken().getText().forEach([this](const text::Char character) -> util::LoopStatus {
                _data->sequenceData.emplace_back(character);
                return util::LoopStatus::Continue;
            }));
        } else {
            _data->sequenceData.emplace_back(static_cast<char32_t>(currentToken().getInteger()));
        }
        if (_data->sequenceData.size() >= limits::maximumCharacterSequenceLength) {
            throwAssemblerError("Character sequence gets too long"_el);
        }
        nextToken();
    } else { // class
        if (currentToken().isText()) {
            static_cast<void>(currentToken().getText().forEach([this](const text::Char character) -> util::LoopStatus {
                _charRanges.emplace_back(character, character);
                return util::LoopStatus::Continue;
            }));
            nextToken();
        } else {
            const auto startCharacter = currentToken().getInteger();
            nextToken();
            if (hasCurrentToken()) {
                if (!currentToken().isMinus()) {
                    throwAssemblerError("Expected end of line or '-' after the character value"_el);
                }
                nextToken();
                if (!hasCurrentToken()) {
                    throwAssemblerError("Expected a second character after '-' in a range expression"_el);
                }
                if (!(currentToken().isChar() || currentToken().isInteger() || currentToken().isOffset())) {
                    throwAssemblerError(
                        "Expected a character, integer or offset expression after '-' in a range expression"_el);
                }
                const auto endCharacter = currentToken().getInteger();
                _charRanges.emplace_back(static_cast<char32_t>(startCharacter), static_cast<char32_t>(endCharacter));
                nextToken();
            } else {
                _charRanges.emplace_back(static_cast<char32_t>(startCharacter), static_cast<char32_t>(startCharacter));
            }
        }
    }
    if (hasCurrentToken()) {
        throwAssemblerError("Only one value is allowed after '.data' command"_el, currentToken().column());
    }
}

void Assembler::processGroupsCommand() {
    // Sets the number of capture groups.
    // .groups 10
    if (!hasCurrentToken() || !currentToken().isInteger()) {
        throwAssemblerError("Expected the number of groups after the '.groups' command"_el);
    }
    const auto groupCount = currentToken().getInteger();
    if (groupCount > limits::maximumCaptureGroupCount) {
        throwAssemblerError(
            text::StringFormat{"The maximum number of capture groups is {}"}.build(limits::maximumCaptureGroupCount));
    }
    nextToken();
    if (hasCurrentToken()) {
        throwAssemblerError("Unexpected token after the '.groups' command"_el, currentToken().column());
    }
    if (_captureGroupSizeSet) {
        throwAssemblerError("The '.groups' command can only be used once"_el);
    }
    if (_data->program.size() > 0) {
        throwAssemblerError("The '.groups' command must be used before any program code"_el);
    }
    _captureGroupNames.resize(groupCount);
    _captureGroupSizeSet = true;
}

void Assembler::processGroupCommand() {
    // Sets the name of a group.
    // Group index is one-based!
    // .group 1 "name"
    if (!_captureGroupSizeSet) {
        throwAssemblerError("The '.groups' command must be used before the '.group' command"_el);
    }
    if (!hasCurrentToken() || !currentToken().isInteger()) {
        throwAssemblerError("Expected the group index after the '.group' command"_el);
    }
    const auto groupIndex = currentToken().getInteger();
    if (groupIndex <= 0 || groupIndex > _captureGroupNames.size()) {
        throwAssemblerError(
            text::StringFormat{"Group index {} is out of range. Must be in the range 1 to {}"}.build(
                groupIndex, _captureGroupNames.size()));
    }
    nextToken();
    if (!hasCurrentToken() || !currentToken().isText()) {
        throwAssemblerError("Expected the group name after the '.group' command"_el);
    }
    const auto &name = currentToken().getText();
    if (!_captureGroupNames[groupIndex - 1].isEmpty()) {
        throwAssemblerError("Group name is already set"_el);
    }
    _captureGroupNames[groupIndex - 1] = name;
    nextToken();
    if (hasCurrentToken()) {
        throwAssemblerError("Unexpected token after the '.group' command"_el, currentToken().column());
    }
}

void Assembler::processClassCommand() {
    if (_currentSection != DataSection::Class) {
        throwAssemblerError("The '.class' command can only be used in a class section"_el);
    }
    if (hasCurrentToken()) {
        throwAssemblerError("Unexpected token after the '.class' command."_el);
    }
    closeCurrentClass(); // close the current class and create a new one.
    if (_data->charClassData.size() >= limits::maximumCharacterClassCount) {
        throwAssemblerError("Character class list gets too long"_el);
    }
    _isClassOpen = true;
}

void Assembler::closeCurrentClass() {
    if (!_isClassOpen) {
        _charRanges.clear();
        return; // Ignore any ranges if there is no open class.
    }
    if (_charRanges.empty()) {
        throwAssemblerError("The last class definition is empty"_el);
    }
    _data->charClassData.emplace_back(CharClass::createPrepared(_charRanges));
    // the range is validated in `processClassCommand()`.
    _labelOffsets.charClassIndex = static_cast<CharClassIndex>(_data->charClassData.size());
    _isClassOpen = false;
    _charRanges.clear();
}

void Assembler::addLabel() {
    if (_labels.contains(_currentLabel)) {
        const auto labelTarget = _labels.get(_currentLabel).value();
        throwAssemblerError(
            text::StringFormat{"Label '{}' is already defined in line {}. Duplicate"}.build(
                _currentLabel, labelTarget.sourceLine.toSizeT() + 1U));
    }
    switch (_currentSection) {
    case DataSection::Program:
        _labels.set(
            _currentLabel, LabelTargetWithSource(DataSection::Program, _labelOffsets.programCounter, _lineIndex));
        break;
    case DataSection::Sequence:
        if (!_labelOffsets.sequenceValid) {
            throwAssemblerError("The maximum number of character sequences has been reached"_el);
        }
        _labels.set(
            _currentLabel, LabelTargetWithSource(DataSection::Sequence, _labelOffsets.sequenceIndex, _lineIndex));
        break;
    case DataSection::Class:
        if (!_labelOffsets.charClassValid) {
            throwAssemblerError("The maximum number of character classes has been reached"_el);
        }
        _labels.set(_currentLabel, LabelTargetWithSource(DataSection::Class, _labelOffsets.charClassIndex, _lineIndex));
        break;
    }
}

}
