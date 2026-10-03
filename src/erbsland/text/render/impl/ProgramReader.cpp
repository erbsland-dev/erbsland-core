// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProgramReader.hpp"

#include "ProgramError.hpp"

#include "../../../mem/ByteIntegerFormat.hpp"
#include "../../Literals.hpp"

namespace erbsland::text::render::impl {

using namespace text::literals;

auto ProgramReader::read() -> ProgramInstruction {
    const auto position = _reader.position();
    if (_reader.isAtEnd()) {
        throw ProgramError{"The program contains a truncated opcode."_el};
    }
    const auto opcode = static_cast<Opcode>(_reader.readByte().toUInt8());
    switch (opcode) {
    case Opcode::EmitText:
    case Opcode::LoadName:
    case Opcode::GetMember:
    case Opcode::PushText:
    case Opcode::ApplyApplicationFilter:
    case Opcode::ApplyBuiltInFilter:
    case Opcode::StoreName:
    case Opcode::StoreScopedName:
    case Opcode::Include:
    case Opcode::RenderBlock:
    case Opcode::LoadSuper:
    case Opcode::EmitEscapedValue:
    case Opcode::BuildList:
    case Opcode::BuildMap:
    case Opcode::Is:
    case Opcode::IsNot:
        return ProgramInstruction{opcode, readOperand(), _program.locationAt(position, _nextSourceMapEntry)};
    case Opcode::PushInteger:
    case Opcode::PushFloat:
    case Opcode::Jump:
    case Opcode::JumpIfFalse:
    case Opcode::JumpIfFalseOrPop:
    case Opcode::JumpIfTrueOrPop:
    case Opcode::NextIteration:
    case Opcode::EndIterationIfNotEmpty:
        return ProgramInstruction{opcode, readFixedOperand(), _program.locationAt(position, _nextSourceMapEntry)};
    case Opcode::PushTrue:
    case Opcode::EmitValue:
    case Opcode::PushFalse:
    case Opcode::PushNull:
    case Opcode::LogicalNot:
    case Opcode::UnaryPlus:
    case Opcode::UnaryMinus:
    case Opcode::Add:
    case Opcode::Subtract:
    case Opcode::Multiply:
    case Opcode::Divide:
    case Opcode::Concatenate:
    case Opcode::In:
    case Opcode::NotIn:
    case Opcode::Equal:
    case Opcode::NotEqual:
    case Opcode::Greater:
    case Opcode::GreaterEqual:
    case Opcode::Less:
    case Opcode::LessEqual:
    case Opcode::BeginListIteration:
    case Opcode::BeginMapIteration:
    case Opcode::EndIteration:
    case Opcode::End:
        return ProgramInstruction{opcode, 0U, _program.locationAt(position, _nextSourceMapEntry)};
    }
    throw ProgramError{"The program contains an unknown opcode."_el};
}

void ProgramReader::jumpTo(const uint64_t target) {
    const auto position = unit::ByteIndex::fromSizeT(static_cast<std::size_t>(target));
    if (position.toSizeT() != target || !_program.hasInstructionAt(position)) {
        throw ProgramError{"A layout program jump target is not an instruction boundary."_el};
    }
    _reader.setPosition(position);
    _nextSourceMapEntry = 0U;
}

auto ProgramReader::readOperand() -> uint64_t {
    const auto value = _reader.readInteger<uint64_t>(mem::ByteIntegerFormat::UnsignedVariableLength);
    if (!value.has_value()) {
        throw ProgramError{"The program contains a malformed or truncated operand."_el};
    }
    return *value;
}

auto ProgramReader::readFixedOperand() -> uint64_t {
    auto value = uint64_t{};
    if (!_reader.readIntegerInto(value)) {
        throw ProgramError{"The program contains a malformed or truncated fixed operand."_el};
    }
    return value;
}

}
