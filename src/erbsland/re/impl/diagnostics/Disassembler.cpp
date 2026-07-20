// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Disassembler.hpp"

#include "GenericProgramReader.hpp"
#include "OperationData.hpp"

#include "../error/InternalError.hpp"

#include "../../../bgeo/Alignment.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringFormat.hpp"

namespace erbsland::re::impl {

using namespace text::literals;

using namespace text;

void Disassembler::setLabel(const LabelTarget target, String label) {
    _labels[target] = std::move(label);
}

auto Disassembler::disassemble() -> StringList {
    if (_data == nullptr) {
        return {};
    }
    _lines.clear();
    writeSequence();
    writeClasses();
    writeProgram();
    return _lines;
}

auto Disassembler::disassembleSequence() -> StringList {
    _lines.clear();
    writeSequence();
    return _lines;
}

auto Disassembler::disassembleClasses() -> StringList {
    _lines.clear();
    writeClasses();
    return _lines;
}

auto Disassembler::disassembleProgram() -> StringList {
    _lines.clear();
    writeProgram();
    return _lines;
}

void Disassembler::writeSequence() {
    if (_data == nullptr || _data->sequenceData.empty()) {
        return;
    }
    // scan for all references into the sequence.
    std::set<SequenceIndex> referencedSequenceIndexes;
    const GenericProgramReader reader{_data->program};
    ProgramCounter programCounter = 0;
    while (programCounter < _data->program.size()) {
        const auto operation = reader.peekOperation(programCounter);
        if (operation == Operation::Sequence) {
            const auto [sequenceIndex, sequenceLength] = reader.readSequence(programCounter);
            referencedSequenceIndexes.insert(sequenceIndex);
        } else {
            reader.skipOperation(programCounter);
        }
    }
    writeTitle("Sequence"_el);
    _lines.append(".section &sequence"_el);
    SequenceIndex sequenceIndex = 0U;
    auto groupStart = sequenceIndex;
    StringEditor groupedSequence;
    auto writeGroup = [&]() -> void {
        writeLineLayout(
            createTarget(LabelTarget{DataSection::Sequence, groupStart}),
            {},
            StringFormat{".data \"{}\""}.build(groupedSequence),
            {});
        groupedSequence.clear();
    };
    for (const auto &character : _data->sequenceData) {
        if ((referencedSequenceIndexes.contains(sequenceIndex) || !character.isSafeUnicode()) &&
            !groupedSequence.isEmpty()) {
            writeGroup();
        }
        if (!character.isSafeUnicode()) {
            writeLineLayout(
                createTarget(LabelTarget{DataSection::Sequence, sequenceIndex}),
                {},
                StringFormat{".data ${:06X}"}.build(static_cast<uint32_t>(character.toRawValue())),
                {});
        } else {
            if (groupedSequence.isEmpty()) {
                groupStart = sequenceIndex;
            }
            groupedSequence.append(character);
        }
        sequenceIndex += 1;
    }
    if (!groupedSequence.isEmpty()) {
        writeGroup();
    }
}

void Disassembler::writeClasses() {
    if (_data == nullptr || _data->charClassData.empty()) {
        return;
    }
    writeTitle("Character classes"_el);
    _lines.append(".section &class"_el);
    CharClassIndex index = 0U;
    for (const auto &classData : _data->charClassData) {
        auto comment = StringEditor{StringFormat{"; [{}]"}.build(classData.toString())};
        if (comment.characterLength() > unit::CpLength{40U}) {
            comment = comment.slice(StringSide::Front, unit::CpLength{37U});
            comment.append("..."_el);
        }
        writeLineLayout(createTarget(LabelTarget{DataSection::Class, index}), {}, ".class"_el, comment);
        for (const auto &range : classData.ranges()) {
            auto dataStr = StringFormat{".data ${:06X}-${:06X}"}.build(
                static_cast<uint32_t>(range.first().toRawValue()), static_cast<uint32_t>(range.last().toRawValue()));
            writeLineLayout(createTarget(LabelTarget{DataSection::Class, index}), {}, dataStr, {});
        }
        ++index;
    }
}

void Disassembler::writeProgram() {
    if (_data == nullptr) {
        return;
    }
    // 0123456789012345678901234567890123456789012345678901234567890123456789
    // long_label_name1: 000000000 000000000   CAPTURE START, 1
    const GenericProgramReader reader{_data->program};
    ProgramCounter programCounter = 0;
    writeTitle("Program"_el);
    if (!_data->charClassData.empty() || !_data->sequenceData.empty()) {
        _lines.append(".section &program"_el);
    }
    while (programCounter < _data->program.size()) {
        auto targetStr = createTarget(LabelTarget{DataSection::Program, programCounter});
        // read the operation.
        const auto operationStart = programCounter;
        auto [operation, arguments] = reader.readOperation(programCounter);
        // read the raw codes.
        const auto operationSize = static_cast<std::size_t>(programCounter - operationStart);
        StringEditor codesStr;
        programCounter = operationStart;
        for (std::size_t i = 0; i < operationSize; ++i) {
            if (i > 0) {
                codesStr.append(U' ');
            }
            codesStr.append(StringFormat{"{:08x}"}.build(_data->program.readCode(programCounter)));
        }
        const auto operationData = dataForOperation(operation);
        auto operationStr = StringEditor{operationData.displayName};
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            operationData.argumentCount() == arguments.size(), "Invalid number of arguments"_el);
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            if (i == 0) {
                operationStr.append(U' ');
            }
            const auto &definition = operationData.arguments[i];
            const auto &value = arguments[i];
            if (i > 0) {
                operationStr.append(", "_el);
            }
            ERBSLAND_CORE_RE_REQUIRE_SAFETY(
                definition.type == argumentTypeFromValue(value), "Unexpected argument type"_el);
            operationStr.append(formatArgument(operation, definition.kind, value));
        }
        writeLineLayout(targetStr, codesStr, operationStr, {});
    }
}

void Disassembler::writeTitle(const String &title) {
    if (_flags.isSet(DisassemblerFlag::TestOutput)) {
        return;
    }
    _lines.append(StringFormat{"; {}"}.build(title));
    _lines.append(String::fromJoined({"; "_el, String::fromCharacter(U'=', unit::CpLength{76U})}));
}

void Disassembler::writeLineLayout(
    const String &location, const String &code, const String &operation, const String &comment) {

    if (_flags.isSet(DisassemblerFlag::TestOutput)) {
        _lines.append(StringFormat{"{} {}"}.build(location, operation));
        return;
    }
    using bgeo::Alignment;
    const auto prefix = String::fromJoined(
        {location.aligned(unit::CpLength{17U}, Alignment::Left),
            " "_el,
            code.aligned(unit::CpLength{21U}, Alignment::Left),
            " "_el});
    if (comment.isEmpty()) {
        _lines.append(String::fromJoined({prefix, operation}));
        return;
    }
    _lines.append(
        String::fromJoined({prefix, operation.aligned(unit::CpLength{19U}, Alignment::Left), " "_el, comment}));
}

auto Disassembler::createTarget(LabelTarget target) const -> String {
    if (_flags.isCleared(DisassemblerFlag::TestOutput) && _labels.contains(target)) {
        return String::fromJoined({_labels.at(target), ":"_el});
    }
    return StringFormat{"${:04X}:"}.build(target.offset);
}

auto Disassembler::createLabel(LabelTarget target) const -> String {
    if (_flags.isCleared(DisassemblerFlag::TestOutput) && _labels.contains(target)) {
        return String::fromJoined({"%"_el, _labels.at(target)});
    }
    return StringFormat{"${:04X}"}.build(target.offset);
}

auto Disassembler::formatArgument(Operation operation, ArgumentKind argumentKind, ArgumentValue value) const -> String {
    StringEditor result;
    switch (argumentKind) {
    case ArgumentKind::ProgramCounter:
        switch (operation.raw()) {
        case Operation::Sequence:
        case Operation::CiSequence:
            return createLabel(LabelTarget{DataSection::Sequence, std::get<uint32_t>(value)});
        case Operation::Class:
        case Operation::CiClass:
        case Operation::NotClass:
        case Operation::NotCiClass:
            return createLabel(LabelTarget{DataSection::Class, std::get<uint32_t>(value)});
        case Operation::Jump:
        case Operation::Split:
        default:
            return createLabel(LabelTarget{DataSection::Program, std::get<uint32_t>(value)});
        }
    case ArgumentKind::Char: {
        const auto unicodeValue = std::get<uint32_t>(value);
        const auto character = Char(unicodeValue);
        if (character.isSafeUnicode()) {
            result.append(U'\'');
            appendToSafeString(result, character);
            result.append(U'\'');
        } else {
            return StringFormat{"${:06X}"}.build(unicodeValue);
        }
        break;
    }
    case ArgumentKind::CaptureGroup:
        return String::fromInteger(std::get<uint32_t>(value));
    case ArgumentKind::Anchor: {
        const auto anchor = TextAnchor(static_cast<TextAnchor::Value>(std::get<uint32_t>(value)));
        result.append(U'&');
        result.append(anchor.toString());
        break;
    }
    case ArgumentKind::Category: {
        const auto category = Category(static_cast<Category::Value>(std::get<uint32_t>(value)));
        result.append(U'&');
        result.append(category.toLongString());
        break;
    }
    case ArgumentKind::SequenceIndex:
        return StringFormat{"${:04x}"}.build(std::get<uint32_t>(value));
    case ArgumentKind::SequenceLength:
        return StringFormat{"${:02x}"}.build(std::get<uint32_t>(value));
    case ArgumentKind::CharClassIndex:
        return StringFormat{"${:04x}"}.build(std::get<uint32_t>(value));
    case ArgumentKind::CounterIndex:
    case ArgumentKind::CounterValue:
    case ArgumentKind::AtomicGroupId:
        return String::fromInteger(std::get<uint32_t>(value));
    default:
        break;
    }
    return result;
}

}
