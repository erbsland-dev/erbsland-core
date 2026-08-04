// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Assembler.hpp"

#include "../Limits.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../text/StringFormat.hpp"

#include <algorithm>

namespace erbsland::re::impl {

using namespace text::literals;

void Assembler::processOperationLine() {
    auto operation = currentToken().getOperation();
    const auto baseOperation = operation;
    if (!_modifiers.empty()) {
        try {
            operation = modifiedOperation(operation, _modifiers);
        } catch (const err::ParameterError &) {
            text::StringEditor modifierStr;
            for (const auto &modifier : _modifiers) {
                if (!modifierStr.isEmpty()) {
                    modifierStr.append(", "_el);
                }
                modifierStr.append(toString(modifier));
            }
            if (_modifiers.size() == 1) {
                throwAssemblerError(
                    text::StringFormat{"Modifier '{}' is not valid for the operation '{}'"}.build(
                        modifierStr, toBaseName(baseOperation)));
            } else {
                throwAssemblerError(
                    text::StringFormat{"Modifiers '{}' are not valid for the operation '{}'"}.build(
                        modifierStr, toBaseName(baseOperation)));
            }
        }
    }
    nextToken();
    AssemblerTokens arguments;
    while (hasCurrentToken()) {
        if (arguments.size() >= 2) {
            throwAssemblerError("Too many arguments"_el, currentToken().column());
        }
        if (currentToken().isArgument()) {
            arguments.emplace_back(currentToken());
            nextToken();
            if (!hasCurrentToken()) {
                break;
            }
            if (!currentToken().isComma()) {
                throwAssemblerError("Expected comma after the argument"_el, currentToken().column());
            }
            nextToken();
            if (!hasCurrentToken()) {
                throwAssemblerError("Unexpected end of line after comma"_el);
            }
        } else {
            throwAssemblerError("Expected argument, got something else"_el, currentToken().column());
        }
    }
    validateArgumentTypes(operation, arguments);
    processOperation(operation, arguments);
}

void Assembler::processOperation(const Operation operation, const AssemblerTokens &arguments) {
    switch (operation.raw()) {
    case Operation::None:
        _writer.writeNone();
        break;
    case Operation::Split:
        processSplit(arguments);
        break;
    case Operation::Jump:
        processJump(arguments);
        break;
    case Operation::Match:
        _writer.writeMatch();
        break;
    case Operation::NotMatch:
        _writer.writeNotMatch();
        break;
    case Operation::Anchor:
        processAnchor(arguments);
        break;
    case Operation::StartCapture:
        processCapture(arguments, false);
        break;
    case Operation::StopCapture:
        processCapture(arguments, true);
        break;
    case Operation::StartAtomic:
        processAtomic(arguments, false);
        break;
    case Operation::StopAtomic:
        processAtomic(arguments, true);
        break;
    case Operation::Counter:
        processCounter(arguments);
        break;
    case Operation::AddCounter:
        processAddCounter(arguments);
        break;
    case Operation::Maximum:
        processMaximum(arguments);
        break;
    case Operation::SkipIfMaximum:
        processSkipIfMaximum(arguments);
        break;
    case Operation::Minimum:
        processMinimum(arguments);
        break;
    case Operation::Success:
        _writer.writeSuccess();
        break;
    case Operation::Failure:
        _writer.writeFailure();
        break;
    case Operation::Char:
        processChar(arguments, false, false);
        break;
    case Operation::CiChar:
        processChar(arguments, true, false);
        break;
    case Operation::NotChar:
        processChar(arguments, false, true);
        break;
    case Operation::NotCiChar:
        processChar(arguments, true, true);
        break;
    case Operation::Sequence:
        processSequence(arguments, false);
        break;
    case Operation::CiSequence:
        processSequence(arguments, true);
        break;
    case Operation::Category:
        processCategory(arguments, false);
        break;
    case Operation::NotCategory:
        processCategory(arguments, true);
        break;
    case Operation::AssertCategory:
        processAssertCategory(arguments, false);
        break;
    case Operation::NotAssertCategory:
        processAssertCategory(arguments, true);
        break;
    case Operation::Class:
        processClass(arguments, false, false);
        break;
    case Operation::CiClass:
        processClass(arguments, true, false);
        break;
    case Operation::NotClass:
        processClass(arguments, false, true);
        break;
    case Operation::NotCiClass:
        processClass(arguments, true, true);
        break;
    case Operation::Any:
        _writer.writeAny();
        break;
    default:
        throwAssemblerError(text::StringFormat{"Operation '{}' is not supported"_el}.build(toString(operation)));
    }
}

void Assembler::processSplit(const AssemblerTokens &arguments) {
    const auto programCounterA = expectProgramCounter(arguments[0], 0);
    const auto programCounterB = expectProgramCounter(arguments[1], 1);
    _writer.writeSplit(programCounterA, programCounterB);
}

void Assembler::processJump(const AssemblerTokens &arguments) {
    const auto programCounter = expectProgramCounter(arguments[0], 0);
    _writer.writeJump(programCounter);
}

void Assembler::processAnchor(const AssemblerTokens &arguments) {
    const auto anchorName = expectIdentifier(arguments[0]);
    try {
        const auto anchor = TextAnchor::fromString(anchorName);
        _writer.writeAnchor(anchor);
    } catch (const err::ParameterError &) {
        throwAssemblerError(text::StringFormat{"Invalid anchor '{}'"}.build(anchorName));
    }
}

void Assembler::processCapture(const AssemblerTokens &arguments, const bool isNegated) {
    const auto captureGroup = expectCaptureGroup(arguments[0]);
    if (captureGroup >= _captureGroupNames.size()) {
        throwAssemblerError(
            text::StringFormat{"The capture group {} exceeds the defined number of {} groups. "
                               "Use '.groups' to increase the number of capture groups"}
                .build(captureGroup, _captureGroupNames.size()));
    }
    if (isNegated) {
        _writer.writeStopCapture(captureGroup);
    } else {
        _writer.writeStartCapture(captureGroup);
    }
}

void Assembler::processAtomic(const AssemblerTokens &arguments, const bool isNegated) {
    const auto atomicGroupId = expectAtomicGroupId(arguments[0]);
    if (isNegated) {
        _writer.writeStopAtomic(atomicGroupId);
    } else {
        _writer.writeStartAtomic(atomicGroupId);
    }
}

void Assembler::processCounter(const AssemblerTokens &arguments) {
    const auto counterIndex = expectCounterIndex(arguments[0]);
    const auto counterValue = expectCounterValue(arguments[1]);
    _data->counterCount = std::max(_data->counterCount, static_cast<std::size_t>(counterIndex) + 1U);
    _writer.writeCounter(counterIndex, counterValue);
}

void Assembler::processAddCounter(const AssemblerTokens &arguments) {
    const auto counterIndex = expectCounterIndex(arguments[0]);
    const auto counterValue = expectCounterValue(arguments[1]);
    _data->counterCount = std::max(_data->counterCount, static_cast<std::size_t>(counterIndex) + 1U);
    _writer.writeAddCounter(counterIndex, counterValue);
}

void Assembler::processMaximum(const AssemblerTokens &arguments) {
    const auto counterIndex = expectCounterIndex(arguments[0]);
    const auto counterValue = expectCounterValue(arguments[1]);
    _data->counterCount = std::max(_data->counterCount, static_cast<std::size_t>(counterIndex) + 1U);
    _writer.writeMaximum(counterIndex, counterValue);
}

void Assembler::processSkipIfMaximum(const AssemblerTokens &arguments) {
    const auto counterIndex = expectCounterIndex(arguments[0]);
    const auto counterValue = expectCounterValue(arguments[1]);
    _data->counterCount = std::max(_data->counterCount, static_cast<std::size_t>(counterIndex) + 1U);
    _writer.writeSkipIfMaximum(counterIndex, counterValue);
}

void Assembler::processMinimum(const AssemblerTokens &arguments) {
    const auto counterIndex = expectCounterIndex(arguments[0]);
    const auto counterValue = expectCounterValue(arguments[1]);
    _data->counterCount = std::max(_data->counterCount, static_cast<std::size_t>(counterIndex) + 1U);
    _writer.writeMinimum(counterIndex, counterValue);
}

void Assembler::processChar(const AssemblerTokens &arguments, const bool isCaseInsensitive, const bool isNegated) {

    const auto character = expectChar(arguments[0]);
    if (isCaseInsensitive) {
        if (isNegated) {
            _writer.writeNotCiChar(character);
        } else {
            _writer.writeCiChar(character);
        }
    } else {
        if (isNegated) {
            _writer.writeNotChar(character);
        } else {
            _writer.writeChar(character);
        }
    }
}

void Assembler::processSequence(const AssemblerTokens &arguments, const bool isCaseInsensitive) {
    const auto offset = expectSequenceIndex(arguments[0]);
    const auto length = expectSequenceLength(arguments[1]);
    if (isCaseInsensitive) {
        _writer.writeCiSequence(offset, length);
    } else {
        _writer.writeSequence(offset, length);
    }
}

void Assembler::processCategory(const AssemblerTokens &arguments, const bool isNegated) {
    const auto categoryName = expectIdentifier(arguments[0]);
    try {
        const auto category = Category::fromUnprocessedString(categoryName);
        if (isNegated) {
            _writer.writeNotCategory(category);
        } else {
            _writer.writeCategory(category);
        }
    } catch (const err::ParameterError &) {
        throwAssemblerError(text::StringFormat{"Invalid category '{}'"}.build(categoryName));
    }
}

void Assembler::processAssertCategory(const AssemblerTokens &arguments, const bool isNegated) {
    const auto categoryName = expectIdentifier(arguments[0]);
    try {
        const auto category = Category::fromUnprocessedString(categoryName);
        if (isNegated) {
            _writer.writeNotAssertCategory(category);
        } else {
            _writer.writeAssertCategory(category);
        }
    } catch (const err::ParameterError &) {
        throwAssemblerError(text::StringFormat{"Invalid category '{}'"}.build(categoryName));
    }
}

void Assembler::processClass(const AssemblerTokens &arguments, const bool isCaseInsensitive, const bool isNegated) {
    const auto classIndex = expectCharClassIndex(arguments[0]);
    if (classIndex >= limits::maximumCharacterClassCount) {
        throwAssemblerError("Class index is out of range"_el);
    }
    if (isCaseInsensitive) {
        if (isNegated) {
            _writer.writeNotCiClass(classIndex);
        } else {
            _writer.writeCiClass(classIndex);
        }
    } else {
        if (isNegated) {
            _writer.writeNotClass(classIndex);
        } else {
            _writer.writeClass(classIndex);
        }
    }
}

}
