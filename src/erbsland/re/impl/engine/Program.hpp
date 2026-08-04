// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Program_fwd.hpp"

#include "../error/InternalError.hpp"

#include <array>
#include <span>
#include <vector>

namespace erbsland::re::impl {

/// Represents a program in the VM.
class Program {
public:
    using Code = uint32_t;
    static constexpr std::size_t cInlineCapacity = 8U;

public: // Code encoding and decoding.
    /// Extract the lower 16 bits from a program code word.
    [[nodiscard]] static constexpr auto extractLowerWord(const Code code) -> uint16_t {
        return static_cast<uint16_t>(code & 0x0000FFFFU);
    }
    /// Encode a value in the lower 16 bits of a program code word.
    [[nodiscard]] static constexpr auto codeLowerWord(const uint16_t value) -> Code {
        return static_cast<Code>(value & 0x0000FFFFU);
    }
    /// Extract a program counter from a program code word.
    [[nodiscard]] static constexpr auto extractProgramCounter(const Code code) -> ProgramCounter {
        return static_cast<ProgramCounter>(extractLowerWord(code));
    }
    /// Encode a program counter in the lower 16 bits of a program code word.
    [[nodiscard]] static constexpr auto codeProgramCounter(const ProgramCounter value) -> Code {
        return codeLowerWord(static_cast<uint16_t>(value));
    }
    /// Extract a byte at the given byte position from a program code word.
    template <std::size_t N>
    [[nodiscard]] static constexpr auto extractByte(const Code code) -> uint8_t {
        constexpr auto shift = (N * 8);
        constexpr auto mask = (static_cast<Code>(0xFFU) << shift);
        return static_cast<uint8_t>((code & mask) >> shift);
    }
    /// Extract the third byte from a program code word.
    [[nodiscard]] static constexpr auto extractHigherByte(const Code code) -> uint8_t { return extractByte<2>(code); }
    /// Encode a value in the third byte of a program code word.
    [[nodiscard]] static constexpr auto codeHigherByte(const uint8_t value) -> Code {
        return static_cast<Code>(value << 16U);
    }
    /// Extract the lower 24 bits from a program code word.
    [[nodiscard]] static constexpr auto extractLower24bits(const Code code) -> uint32_t { return (code & 0x00FFFFFFU); }
    /// Encode a value in the lower 24 bits of a program code word.
    [[nodiscard]] static constexpr auto codeLower24bits(const uint32_t value) -> Code {
        return static_cast<Code>(value & 0x00FFFFFFU);
    }

public:
    // defaults
    Program() = default;
    ~Program() = default;
    Program(const Program &) = default;
    Program(Program &&other) noexcept :
        _inlineData{other._inlineData},
        _extendedData{std::move(other._extendedData)},
        _size{other._size},
        _usesExtendedData{other._usesExtendedData} {
        other._size = 0U;
        other._usesExtendedData = false;
    }
    // defaults
    auto operator=(const Program &) -> Program & = default;
    /// Move program storage into this instance and leave the source empty.
    auto operator=(Program &&other) noexcept -> Program & {
        if (this != &other) {
            _inlineData = other._inlineData;
            _extendedData = std::move(other._extendedData);
            _size = other._size;
            _usesExtendedData = other._usesExtendedData;
            other._size = 0U;
            other._usesExtendedData = false;
        }
        return *this;
    }

public: // Low-level peek, read and write.
    /// Get the size of the program.
    [[nodiscard]] auto size() const noexcept -> std::size_t { return _size; }
    /// Access the raw data of the program (for unit tests).
    [[nodiscard]] auto data() const noexcept -> std::span<const Code> {
        return _usesExtendedData ? std::span<const Code>{_extendedData}
                                 : std::span<const Code>{_inlineData.data(), _size};
    }
    /// Peek at ta code point, without increasing the program counter.
    [[nodiscard]] auto peekCode(const ProgramCounter programCounter) const -> Code {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            programCounter < _size, "While reading program code, program counter out of bounds"_el);
        return data()[programCounter];
    }
    /// Read a code point from the program.
    [[nodiscard]] auto readCode(ProgramCounter &programCounter) const -> Code {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            programCounter < _size, "While reading program code, program counter out of bounds"_el);
        const auto code = data()[programCounter];
        programCounter += 1;
        return code;
    }
    /// Skip a code point.
    void skipCode(ProgramCounter &programCounter) const { programCounter += 1; }
    /// Reserve memory for the program.
    void reserve(const std::size_t size) {
        if (!_usesExtendedData && size > cInlineCapacity) {
            _extendedData.reserve(size);
            _extendedData.insert(_extendedData.end(), _inlineData.begin(), _inlineData.begin() + _size);
            _usesExtendedData = true;
        } else if (_usesExtendedData) {
            _extendedData.reserve(size);
        }
    }
    /// Write a code point to the program.
    void writeCode(const Code code, ProgramCounter &programCounter) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            programCounter <= _size, "While writing program code, program counter out of bounds"_el);
        if (programCounter == _size) {
            reserve(_size + 1U);
            if (_usesExtendedData) {
                _extendedData.push_back(code);
            } else {
                _inlineData[_size] = code;
            }
            ++_size;
        } else if (_usesExtendedData) {
            _extendedData[programCounter] = code;
        } else {
            _inlineData[programCounter] = code;
        }
        programCounter += 1;
    }
    /// Clear this program.
    void clear() noexcept {
        _extendedData.clear();
        _size = 0U;
    }
    /// Append another program without decoding its instructions.
    /// @param other The program to append.
    void append(const Program &other) {
        const auto newSize = _size + other._size;
        reserve(newSize);
        if (_usesExtendedData) {
            const auto otherData = other.data();
            _extendedData.insert(_extendedData.end(), otherData.begin(), otherData.end());
        } else {
            for (const auto code : other.data()) {
                _inlineData[_size++] = code;
            }
            return;
        }
        _size = newSize;
    }

private:
    std::array<Code, cInlineCapacity> _inlineData{};
    std::vector<Code> _extendedData;
    std::size_t _size{};
    bool _usesExtendedData{};
};

}
