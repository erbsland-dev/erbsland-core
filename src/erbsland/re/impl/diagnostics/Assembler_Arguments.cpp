// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Assembler.hpp"

#include "../Limits.hpp"

#include "../../../text/StringFormat.hpp"

namespace erbsland::re::impl {

void Assembler::validateArgumentTypes(const Operation operation, const AssemblerTokens &arguments) const {
    const auto &data = dataForOperation(operation);
    if (data.argumentCount() != arguments.size()) {
        throwAssemblerError(
            text::StringFormat{"Operation '{}' requires {} arguments"}.build(
                toString(operation), data.argumentCount()));
    }
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        const auto &argument = arguments[i];
        const auto &definition = data.arguments[i];
        if (!doesArgumentMatch(argument, definition)) {
            throwAssemblerError(
                text::StringFormat{"Argument {} of operation '{}' must be a {}."}.build(
                    i + 1, toString(operation), toString(definition.kind)));
        }
    }
}

auto Assembler::doesArgumentMatch(const AssemblerToken &argument, const ArgumentDefinition &definition) noexcept
    -> bool {

    switch (definition.kind) {
    case ArgumentKind::ProgramCounter:
        return argument.isInteger() || argument.isOffset() || argument.isLabel();
    case ArgumentKind::Char:
        return argument.isInteger() || argument.isOffset() || argument.isChar();
    case ArgumentKind::CaptureGroup:
        return argument.isInteger();
    case ArgumentKind::AtomicGroupId:
        return argument.isInteger();
    case ArgumentKind::Anchor:
        return argument.isIdentifier();
    case ArgumentKind::Category:
        return argument.isIdentifier();
    case ArgumentKind::SequenceIndex:
        return argument.isInteger() || argument.isOffset() || argument.isLabel();
    case ArgumentKind::SequenceLength:
        return argument.isInteger() || argument.isOffset();
    case ArgumentKind::CharClassIndex:
        return argument.isInteger() || argument.isOffset() || argument.isLabel();
    case ArgumentKind::CounterIndex:
        return argument.isInteger() || argument.isOffset();
    case ArgumentKind::CounterValue:
        return argument.isInteger() || argument.isOffset();
    default:
        return false;
    }
}

auto Assembler::expectIdentifier(const AssemblerToken &argument) const -> text::String {
    if (argument.isIdentifier()) {
        return argument.getText();
    }
    throwAssemblerError("Expected an identifier"_el);
}

auto Assembler::expectCounterIndex(const AssemblerToken &argument) const -> ArgumentIndex {
    if (argument.isInteger() || argument.isOffset()) {
        const auto value = argument.getInteger();
        if (value >= limits::maximumCounterCount) {
            throwAssemblerError("Counter index is out of range"_el);
        }
        return static_cast<ArgumentIndex>(value);
    }
    throwAssemblerError("Expected a counter index as integer expression"_el);
}

auto Assembler::expectCounterValue(const AssemblerToken &argument) const -> uint16_t {
    if (argument.isInteger() || argument.isOffset()) {
        const auto value = argument.getInteger();
        if (value > 0xffff) {
            throwAssemblerError("Counter value is out of range"_el);
        }
        return static_cast<uint16_t>(value);
    }
    throwAssemblerError("Expected a counter value as integer expression"_el);
}

auto Assembler::expectChar(const AssemblerToken &argument) const -> text::Char {
    if (argument.isInteger() || argument.isOffset() || argument.isChar()) {
        const auto character = text::Char{static_cast<char32_t>(argument.getInteger())};
        if (!character.isValidUnicode()) {
            throwAssemblerError("Character code is out of the valid Unicode range"_el);
        }
        return character;
    }
    throwAssemblerError("Expected a character as integer or char expression"_el);
}

auto Assembler::expectCaptureGroup(const AssemblerToken &argument) const -> std::size_t {
    if (!(argument.isInteger() || argument.isOffset())) {
        throwAssemblerError("Expected a capture group index as integer expression"_el);
    }
    const auto captureGroup = static_cast<std::size_t>(argument.getInteger());
    if (captureGroup > limits::maximumCaptureGroupCount) {
        throwAssemblerError(text::StringFormat{"Capture group '{}' is out of range"}.build(captureGroup));
    }
    return captureGroup;
}

auto Assembler::expectAtomicGroupId(const AssemblerToken &argument) const -> AtomicGroupId {
    if (!(argument.isInteger() || argument.isOffset())) {
        throwAssemblerError("Expected an atomic group ID as integer expression"_el);
    }
    const auto atomicGroupId = static_cast<AtomicGroupId>(argument.getInteger());
    if (atomicGroupId > limits::maximumAtomicGroupCount) {
        throwAssemblerError(text::StringFormat{"Atomic group ID '{}' is out of range"}.build(atomicGroupId));
    }
    return atomicGroupId;
}

auto Assembler::expectProgramCounter(const AssemblerToken &argument, ArgumentIndex argumentIndex) -> ProgramCounter {
    if (argument.isInteger() || argument.isOffset()) {
        const auto value = argument.getInteger();
        if (value >= limits::maximumProgramLength) {
            throwAssemblerError("Program counter out of range"_el);
        }
        return static_cast<ProgramCounter>(value);
    }
    if (argument.isLabel()) {
        const auto label = argument.getText();
        if (_labels.contains(label)) {
            const auto labelTarget = _labels.get(label).value();
            if (labelTarget.section != DataSection::Program) {
                throwAssemblerError(text::StringFormat{"Label '{}' does not point to a program location"}.build(label));
            }
            return static_cast<ProgramCounter>(labelTarget.offset);
        }
        _patches.emplace_back(
            _lineIndex, label, DataSection::Program, _programCounter, static_cast<ArgumentIndex>(argumentIndex));
        return 0x0000fffeU;
    }
    throwAssemblerError("Expected a program counter as integer or label expression"_el);
}

auto Assembler::expectCharClassIndex(const AssemblerToken &argument) -> CharClassIndex {
    if (argument.isInteger() || argument.isOffset()) {
        const auto value = argument.getInteger();
        if (value >= limits::maximumCharacterClassCount) {
            throwAssemblerError("Character class index out of range"_el);
        }
        return static_cast<CharClassIndex>(value);
    }
    if (argument.isLabel()) {
        const auto label = argument.getText();
        if (_labels.contains(label)) {
            const auto labelTarget = _labels.get(label).value();
            if (labelTarget.section != DataSection::Class) {
                throwAssemblerError(text::StringFormat{"Label '{}' does not point to a character class"}.build(label));
            }
            return static_cast<CharClassIndex>(labelTarget.offset);
        }
        _patches.emplace_back(_lineIndex, label, DataSection::Class, _programCounter, static_cast<ArgumentIndex>(0));
        return 0x0000fffcU;
    }
    throwAssemblerError("Expected a character class index as integer or label expression"_el);
}

auto Assembler::expectSequenceIndex(const AssemblerToken &argument) -> SequenceIndex {
    if (argument.isInteger() || argument.isOffset()) {
        const auto value = argument.getInteger();
        if (value >= limits::maximumCharacterSequenceLength) {
            throwAssemblerError("Character class index out of range"_el);
        }
        return static_cast<SequenceIndex>(value);
    }
    if (argument.isLabel()) {
        const auto label = argument.getText();
        if (_labels.contains(label)) {
            const auto labelTarget = _labels.get(label).value();
            if (labelTarget.section != DataSection::Sequence) {
                throwAssemblerError(
                    text::StringFormat{"Label '{}' does not point to a character sequence"}.build(label));
            }
            return static_cast<SequenceIndex>(labelTarget.offset);
        }
        _patches.emplace_back(_lineIndex, label, DataSection::Class, _programCounter, static_cast<ArgumentIndex>(0));
        return 0x0000fffbU;
    }
    throwAssemblerError("Expected a sequence index as integer or label expression"_el);
}

auto Assembler::expectSequenceLength(const AssemblerToken &argument) -> SequenceLength {
    if (argument.isInteger() || argument.isOffset()) {
        const auto value = argument.getInteger();
        if (value == 0 || value > 0x000000ffU) {
            throwAssemblerError("Sequence length is out of range (1-255)"_el);
        }
        return static_cast<SequenceLength>(value);
    }
    throwAssemblerError("Expected a sequence length as integer expression"_el);
}

}
