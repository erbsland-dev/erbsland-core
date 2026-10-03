// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/Char.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>

#include <functional>

namespace demo {

using namespace el::text::literals;

/// Assemble bounded ASCII CRLF lines across arbitrary network data blocks.
class SmtpLineBuffer {
public: // input
    /// Deliver complete lines, without CRLF, until the callback requests a stop.
    /// @param data A newly received block.
    /// @param receiveLine Returns false when no further input should be processed.
    /// @throws el::ApplicationError If input is not ASCII CRLF text or a line is too long.
    void receive(const el::ByteBlock &data, const std::function<bool(const el::String &)> &receiveLine) {
        for (const auto byte : data.span()) {
            if (_carriageReturn) {
                if (byte != '\n') {
                    throw el::ApplicationError{"SMTP requires CRLF line endings."_el};
                }
                _carriageReturn = false;
                const auto line = el::String{_line};
                _line.clear();
                if (!receiveLine(line)) {
                    return;
                }
            } else if (byte == '\r') {
                _carriageReturn = true;
            } else {
                if (byte != '\t' && (byte < 32 || byte > 126)) {
                    throw el::ApplicationError{"SMTP input must be printable ASCII text."_el};
                }
                // Allow one extra dot for SMTP transparency on a 998-character data line.
                if (_line.length() >= el::ByteCount{999}) {
                    throw el::ApplicationError{"SMTP line exceeds the demo's line limit."_el};
                }
                _line.append(el::Char{byte.toUInt8()});
            }
        }
    }

private:
    el::StringEditor _line;      ///< Incomplete line retained between callbacks.
    bool _carriageReturn{false}; ///< CR at the end of the preceding input.
};

}
