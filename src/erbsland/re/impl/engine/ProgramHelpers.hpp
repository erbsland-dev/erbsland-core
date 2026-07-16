// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Program.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::re::impl {

class ProgramHelpers {
protected:
    ProgramHelpers() = default;

protected:
    [[nodiscard]] static constexpr auto extractLowerWord(const Program::Code code) -> uint16_t {
        return static_cast<uint16_t>(code & 0x0000FFFFU);
    }
    [[nodiscard]] static constexpr auto codeLowerWord(const uint16_t value) -> Program::Code {
        return static_cast<Program::Code>(value & 0x0000FFFFU);
    }
    [[nodiscard]] static constexpr auto extractProgramCounter(const Program::Code code) -> ProgramCounter {
        return static_cast<ProgramCounter>(extractLowerWord(code));
    }
    [[nodiscard]] static constexpr auto codeProgramCounter(const ProgramCounter value) -> Program::Code {
        return codeLowerWord(static_cast<uint16_t>(value));
    }
    template <std::size_t N>
    [[nodiscard]] static constexpr auto extractByte(const Program::Code code) -> uint8_t {
        constexpr auto shift = (N * 8);
        constexpr auto mask = (static_cast<Program::Code>(0xFFU) << shift);
        return static_cast<uint8_t>((code & mask) >> shift);
    }
    [[nodiscard]] static constexpr auto extractHigherByte(const Program::Code code) -> uint8_t {
        return extractByte<2>(code);
    }
    [[nodiscard]] static constexpr auto codeHigherByte(const uint8_t value) -> Program::Code {
        return static_cast<Program::Code>(value << 16U);
    }
    [[nodiscard]] static constexpr auto extractLower24bits(const Program::Code code) -> uint32_t {
        return (code & 0x00FFFFFFU);
    }
    [[nodiscard]] static constexpr auto codeLower24bits(const uint32_t value) -> Program::Code {
        return static_cast<Program::Code>(value & 0x00FFFFFFU);
    }
};

}
