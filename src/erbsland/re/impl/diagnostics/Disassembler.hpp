// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DisassemblerFlags.hpp"
#include "LabelTarget.hpp"
#include "OperationData.hpp"

#include "../engine/EngineData.hpp"

#include "../../../text/String.hpp"
#include "../../../text/StringViewList.hpp"

namespace erbsland::re::impl {

/// A disassembler for the regular expression engine data.
class Disassembler {
public:
    /// Create a new instance for the given data.
    explicit Disassembler(ConstEngineDataPtr data, const DisassemblerFlags flags = DisassemblerFlag::None) :
        _data{std::move(data)}, _flags{flags} {}

public:
    /// Set a label
    void setLabel(LabelTarget target, text::String label);

    /// Disassemble the engine data into a human-readable listing.
    ///
    [[nodiscard]] auto disassemble() -> text::StringViewList;

    /// Disassemble the sequence into human-readable instructions.
    [[nodiscard]] auto disassembleSequence() -> text::StringViewList;

    /// Disassemble the character classes into human-readable instructions.
    [[nodiscard]] auto disassembleClasses() -> text::StringViewList;

    /// Disassemble the program into human-readable instructions.
    [[nodiscard]] auto disassembleProgram() -> text::StringViewList;

private:
    /// Write the lines for the sequence data.
    void writeSequence();

    /// Write the lines for the classes.
    void writeClasses();

    /// Write the lines for the program.
    void writeProgram();

    /// Write a title comment.
    void writeTitle(const text::StringView &title);

    /// Write a line with a given layout.
    void writeLineLayout(
        const text::StringView &location,
        const text::StringView &code,
        const text::StringView &operation,
        const text::StringView &comment);

    /// Get a string for a given target.
    /// @param target The target.
    /// @return The formatted location string.
    [[nodiscard]] auto createTarget(LabelTarget target) const -> text::String;

    /// Get a string for a label.
    /// @param target The target for the label.
    /// @return The formatted label string.
    [[nodiscard]] auto createLabel(LabelTarget target) const -> text::String;

    /// Get a string for the given argument.
    [[nodiscard]] auto formatArgument(Operation operation, ArgumentKind argumentKind, ArgumentValue value) const
        -> text::String;

private:
    // configuration
    ConstEngineDataPtr _data;                              ///< The engine data.
    DisassemblerFlags _flags;                              ///< The disassembler flags.
    std::unordered_map<LabelTarget, text::String> _labels; ///< Labels to use in disassembly.

    // runtime
    text::StringViewList _lines; ///< The disassembly lines.
};

}
