// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../error/InternalError.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace erbsland::re::impl {

class Program;
using ProgramPtr = std::shared_ptr<Program>;
using ConstProgramPtr = std::shared_ptr<const Program>;
using ProgramCounter = uint16_t;

/// Represents a program in the VM.
class Program {
public:
    using Code = uint32_t;

public:
    Program() = default;
    ~Program() = default;

public: // Low-level peek, read and write.
    /// Get the size of the program.
    [[nodiscard]] auto size() const noexcept -> std::size_t { return _data.size(); }
    /// Access the raw data of the program (for unit tests).
    [[nodiscard]] auto data() const noexcept -> const std::vector<Code> & { return _data; }
    /// Peek at ta code point, without increasing the program counter.
    [[nodiscard]] auto peekCode(const ProgramCounter programCounter) const -> Code {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            programCounter < _data.size(), "While reading program code, program counter out of bounds"_el);
        return _data[programCounter];
    }
    /// Read a code point from the program.
    [[nodiscard]] auto readCode(ProgramCounter &programCounter) const -> Code {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            programCounter < _data.size(), "While reading program code, program counter out of bounds"_el);
        const auto code = _data[programCounter];
        programCounter += 1;
        return code;
    }
    /// Skip a code point.
    void skipCode(ProgramCounter &programCounter) const { programCounter += 1; }
    /// Reserve memory for the program.
    void reserve(const std::size_t size) { _data.reserve(size); }
    /// Write a code point to the program.
    void writeCode(const Code code, ProgramCounter &programCounter) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            programCounter <= _data.size(), "While writing program code, program counter out of bounds"_el);
        if (programCounter == _data.size()) {
            _data.push_back(code);
        } else {
            _data[programCounter] = code;
        }
        programCounter += 1;
    }
    /// Clear this program.
    void clear() noexcept { _data.clear(); }

private:
    std::vector<Code> _data;
};

}
