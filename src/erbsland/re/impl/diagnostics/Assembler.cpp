// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Assembler.hpp"

#include "AssemblerTokenizer.hpp"

#include "../engine/EngineData.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::re::impl {

Assembler::Assembler() : _data{std::make_shared<EngineData>()}, _writer{_data->program, _programCounter} {
    _data->counterCount = 0U;
}

auto Assembler::compile(const text::StringList &lines) -> EngineDataPtr {
    _lineIndex = unit::LineIndex::zero();
    for (const auto &line : lines) {
        processLine(line);
        ++_lineIndex;
    }
    patchLabelOffsets();
    _data->captureGroupNames = std::move(_captureGroupNames);
    return _data;
}

void Assembler::patchLabelOffsets() {
    for (const auto &patch : _patches) {
        if (!_labels.contains(patch.label)) {
            _lineIndex = patch.sourceLine;
            throwAssemblerError(text::StringFormat{"Label '{}' not found"}.build(patch.label));
        }
        const auto labelTarget = _labels.get(patch.label).value();
        if (labelTarget.section != patch.section) {
            _lineIndex = patch.sourceLine;
            throwAssemblerError(
                text::StringFormat{"Label '{}' points to a '{}' section. The operation requires a '{}' location"}.build(
                    patch.label, toString(labelTarget.section), toString(patch.section)));
        }
        _writer.setProgramCounter(patch.operationPosition);
        _writer.patchOffset(static_cast<ProgramCounter>(labelTarget.offset), patch.argumentIndex);
    }
}

void Assembler::processLine(const text::String &line) {
    _currentLabel = {};
    _modifiers.clear();
    tokenizeLine(line);
    if (!hasCurrentToken()) {
        return;
    }
    if (currentToken().isLabel()) {
        _currentLabel = currentToken().getText();
        _labelOffsets.programCounter = _programCounter;
        _labelOffsets.sequenceValid = (_data->sequenceData.size() < (sizeof(SequenceIndex) * 8));
        _labelOffsets.sequenceIndex = static_cast<SequenceIndex>(_data->sequenceData.size());
        _labelOffsets.charClassValid = (_data->charClassData.size() < (sizeof(CharClassIndex) * 8));
        _labelOffsets.charClassIndex = static_cast<CharClassIndex>(_data->charClassData.size());
        nextToken();
    }
    if (!hasCurrentToken()) {
        return;
    }
    while (currentToken().isModifier()) {
        const auto modifier = currentToken().getModifier();
        if (_currentSection != DataSection::Program) {
            throwAssemblerError(
                text::StringFormat{"The modifier '{}' can only be used in the program section"}.build(
                    toString(modifier)),
                currentToken().column());
        }
        if (_modifiers.contains(modifier)) {
            throwAssemblerError(
                text::StringFormat{"Modifier '{}' is already defined"}.build(toString(modifier)),
                currentToken().column());
        }
        _modifiers.insert(modifier);
        nextToken();
    }
    if (!hasCurrentToken()) {
        throwAssemblerError("Unexpected end of line."_el);
    }
    if (currentToken().isCommand()) {
        processCommandLine();
    } else if (currentToken().isOperation()) {
        if (_currentSection == DataSection::Program) {
            processOperationLine();
        } else {
            throwAssemblerError("Program operation outside program section"_el, currentToken().column());
        }
    } else {
        throwAssemblerError("Unexpected token"_el, currentToken().column());
    }
    if (!_currentLabel.isEmpty()) {
        addLabel();
    }
}

void Assembler::tokenizeLine(const text::String &line) {
    try {
        _tokens = AssemblerTokenizer{line}.tokens();
        _tokenIndex = 0;
    } catch (const RegExError &error) {
        throw error.withLineNumber(_lineIndex);
    }
}

auto Assembler::hasCurrentToken() const noexcept -> bool {
    return _tokenIndex < _tokens.size();
}

auto Assembler::currentToken() const noexcept -> const AssemblerToken & {
    return _tokens[_tokenIndex];
}

void Assembler::nextToken() noexcept {
    _tokenIndex += 1;
}

}
