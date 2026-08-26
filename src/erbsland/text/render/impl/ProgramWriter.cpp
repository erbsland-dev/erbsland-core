// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ProgramWriter.hpp"

#include "Opcode.hpp"
#include "ProgramError.hpp"

#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteIntegerFormat.hpp"
#include "../../../text/Literals.hpp"

#include <algorithm>
#include <bit>

namespace erbsland::text::render::impl {

using namespace text::literals;

void ProgramWriter::writeEmitText(const std::size_t constantIndex, const unit::CodeLocation location) {
    writeOpcode(Opcode::EmitText, constantIndex, location);
}

void ProgramWriter::writeLoadName(const std::size_t constantIndex, const unit::CodeLocation location) {
    writeOpcode(Opcode::LoadName, constantIndex, location);
}

void ProgramWriter::writeGetMember(const std::size_t constantIndex, const unit::CodeLocation location) {
    writeOpcode(Opcode::GetMember, constantIndex, location);
}

void ProgramWriter::writePushTrue(const unit::CodeLocation location) {
    writeOpcode(Opcode::PushTrue, location);
}

void ProgramWriter::writePushFalse(const unit::CodeLocation location) {
    writeOpcode(Opcode::PushFalse, location);
}

void ProgramWriter::writePushNull(const unit::CodeLocation location) {
    writeOpcode(Opcode::PushNull, location);
}

void ProgramWriter::writePushInteger(const int64_t value, const unit::CodeLocation location) {
    writeFixedOpcode(Opcode::PushInteger, std::bit_cast<uint64_t>(value), location);
}

void ProgramWriter::writePushFloat(const double value, const unit::CodeLocation location) {
    writeFixedOpcode(Opcode::PushFloat, std::bit_cast<uint64_t>(value), location);
}

void ProgramWriter::writePushText(const std::size_t constantIndex, const unit::CodeLocation location) {
    writeOpcode(Opcode::PushText, constantIndex, location);
}

void ProgramWriter::writeLogicalNot(const unit::CodeLocation location) {
    writeOpcode(Opcode::LogicalNot, location);
}

void ProgramWriter::writeComparison(const Opcode opcode, const unit::CodeLocation location) {
    switch (opcode) {
    case Opcode::Equal:
    case Opcode::NotEqual:
    case Opcode::Greater:
    case Opcode::GreaterEqual:
    case Opcode::Less:
    case Opcode::LessEqual:
        writeOpcode(opcode, location);
        return;
    default:
        throw ProgramError{"A non-comparison opcode was passed to writeComparison."_el};
    }
}

void ProgramWriter::writeApplicationFilter(
    const std::size_t constantIndex, const std::size_t argumentCount, const unit::CodeLocation location) {
    writeFilterOpcode(Opcode::ApplyApplicationFilter, constantIndex, argumentCount, location);
}

void ProgramWriter::writeBuiltInFilter(
    const BuiltInFilter filter, const std::size_t argumentCount, const unit::CodeLocation location) {
    writeFilterOpcode(Opcode::ApplyBuiltInFilter, static_cast<std::size_t>(filter), argumentCount, location);
}

void ProgramWriter::writeFilterOpcode(
    const Opcode opcode,
    const std::size_t identifier,
    const std::size_t argumentCount,
    const unit::CodeLocation location) {
    if (argumentCount > 2U) {
        throw ProgramError{"A layout filter cannot have more than two arguments."_el};
    }
    if (opcode != Opcode::ApplyApplicationFilter && opcode != Opcode::ApplyBuiltInFilter) {
        throw ProgramError{"A non-filter opcode was passed to the filter writer."_el};
    }
    writeOpcode(opcode, (identifier << 2U) | argumentCount, location);
}

void ProgramWriter::writeUnary(const Opcode opcode, const unit::CodeLocation location) {
    if (opcode != Opcode::UnaryPlus && opcode != Opcode::UnaryMinus) {
        throw ProgramError{"A non-unary opcode was passed to writeUnary."_el};
    }
    writeOpcode(opcode, location);
}

void ProgramWriter::writeBinary(const Opcode opcode, const unit::CodeLocation location) {
    switch (opcode) {
    case Opcode::Add:
    case Opcode::Subtract:
    case Opcode::Multiply:
    case Opcode::Divide:
    case Opcode::Concatenate:
    case Opcode::In:
    case Opcode::NotIn:
        writeOpcode(opcode, location);
        return;
    default:
        throw ProgramError{"A non-binary opcode was passed to writeBinary."_el};
    }
}

void ProgramWriter::writeTest(const Opcode opcode, const ValueTest test, const unit::CodeLocation location) {
    if (opcode != Opcode::Is && opcode != Opcode::IsNot) {
        throw ProgramError{"A non-test opcode was passed to writeTest."_el};
    }
    writeOpcode(opcode, static_cast<std::size_t>(test), location);
}

void ProgramWriter::writeBuildList(const std::size_t count, const unit::CodeLocation location) {
    writeOpcode(Opcode::BuildList, count, location);
}

void ProgramWriter::writeBuildMap(const std::size_t count, const unit::CodeLocation location) {
    writeOpcode(Opcode::BuildMap, count, location);
}

void ProgramWriter::writeStoreName(const std::size_t constantIndex, const unit::CodeLocation location) {
    writeOpcode(Opcode::StoreName, constantIndex, location);
}

auto ProgramWriter::writeJump(const unit::CodeLocation location) -> JumpHandle {
    return writeJumpOpcode(Opcode::Jump, location);
}

auto ProgramWriter::writeJumpIfFalse(const unit::CodeLocation location) -> JumpHandle {
    return writeJumpOpcode(Opcode::JumpIfFalse, location);
}

auto ProgramWriter::writeJumpIfFalseOrPop(const unit::CodeLocation location) -> JumpHandle {
    return writeJumpOpcode(Opcode::JumpIfFalseOrPop, location);
}

auto ProgramWriter::writeJumpIfTrueOrPop(const unit::CodeLocation location) -> JumpHandle {
    return writeJumpOpcode(Opcode::JumpIfTrueOrPop, location);
}

auto ProgramWriter::markLabel() const noexcept -> Label {
    return Label{_writer.position()};
}

void ProgramWriter::writeJumpTo(const Label label, const unit::CodeLocation location) {
    if (!label.isValid() || label._position >= _writer.position() ||
        std::ranges::none_of(_sourceMap, [label](const Program::SourceMapEntry &entry) -> bool {
            return entry.first == label._position;
        })) {
        throw ProgramError{"A layout-program label is invalid or is not a preceding instruction boundary."_el};
    }
    writeFixedOpcode(Opcode::Jump, label._position.toSizeT(), location);
}

void ProgramWriter::patchJump(const JumpHandle handle) {
    const auto entry = std::find(_openJumps.begin(), _openJumps.end(), handle._operandPosition);
    if (!handle.isValid() || entry == _openJumps.end()) {
        throw ProgramError{"A layout-program jump handle is invalid or was already patched."_el};
    }
    const auto end = _writer.position();
    _writer.setPosition(handle._operandPosition);
    _writer.writeUInt64(end.toSizeT());
    _writer.setPosition(end);
    _openJumps.erase(entry);
}

void ProgramWriter::writeBeginListIteration(const unit::CodeLocation location) {
    writeOpcode(Opcode::BeginListIteration, location);
}

void ProgramWriter::writeBeginMapIteration(const unit::CodeLocation location) {
    writeOpcode(Opcode::BeginMapIteration, location);
}

auto ProgramWriter::writeNextIteration(const unit::CodeLocation location) -> JumpHandle {
    return writeJumpOpcode(Opcode::NextIteration, location);
}

void ProgramWriter::writeStoreScopedName(const std::size_t constantIndex, const unit::CodeLocation location) {
    writeOpcode(Opcode::StoreScopedName, constantIndex, location);
}

void ProgramWriter::writeEndIteration(const unit::CodeLocation location) {
    writeOpcode(Opcode::EndIteration, location);
}

auto ProgramWriter::writeEndIterationIfNotEmpty(const unit::CodeLocation location) -> JumpHandle {
    return writeJumpOpcode(Opcode::EndIterationIfNotEmpty, location);
}

void ProgramWriter::writeInclude(const std::size_t includeIndex, const unit::CodeLocation location) {
    writeOpcode(Opcode::Include, includeIndex, location);
}

void ProgramWriter::writeRenderBlock(const std::size_t nameConstantIndex, const unit::CodeLocation location) {
    writeOpcode(Opcode::RenderBlock, nameConstantIndex, location);
}

void ProgramWriter::writeLoadSuper(const std::size_t depth, const unit::CodeLocation location) {
    writeOpcode(Opcode::LoadSuper, depth, location);
}

void ProgramWriter::writeEmitValue(const EscapeFormat format, const unit::CodeLocation location) {
    if (format == EscapeFormat::None) {
        writeOpcode(Opcode::EmitValue, location);
    } else {
        writeOpcode(Opcode::EmitEscapedValue, static_cast<std::size_t>(format.toRawValue()), location);
    }
}

void ProgramWriter::writeEnd(const unit::CodeLocation location) {
    writeOpcode(Opcode::End, location);
}

auto ProgramWriter::takeProgram() -> Program {
    if (!_openJumps.empty()) {
        throw ProgramError{"A layout program contains unpatched forward jumps."_el};
    }
    auto result = Program{mem::ByteBlock{_writer.takeByteBlockEditor()}, std::move(_sourceMap)};
    _sourceMap.clear();
    return result;
}

void ProgramWriter::writeOpcode(const Opcode opcode, const unit::CodeLocation location) {
    _sourceMap.emplace_back(_writer.position(), location);
    _writer.writeByte(mem::Byte{static_cast<uint8_t>(opcode)});
}

void ProgramWriter::writeOpcode(const Opcode opcode, const std::size_t operand, const unit::CodeLocation location) {
    writeOpcode(opcode, location);
    _writer.writeIntegerOrThrow<uint64_t>(operand, mem::ByteIntegerFormat::UnsignedVariableLength);
}

void ProgramWriter::writeFixedOpcode(const Opcode opcode, const uint64_t operand, const unit::CodeLocation location) {
    writeOpcode(opcode, location);
    _writer.writeUInt64(operand);
}

auto ProgramWriter::writeJumpOpcode(const Opcode opcode, const unit::CodeLocation location) -> JumpHandle {
    writeOpcode(opcode, location);
    const auto operandPosition = _writer.position();
    _writer.writeUInt64(0U);
    _openJumps.emplace_back(operandPosition);
    return JumpHandle{operandPosition};
}

}
