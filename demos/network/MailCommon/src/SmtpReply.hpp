// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ApplicationError.hpp>
#include <erbsland/AsciiCategory.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringSide.hpp>

#include <optional>

namespace demo {

using namespace el::text::literals;

/// Recognize a complete SMTP reply, including replies split over several lines.
class SmtpReply {
public: // parsing
    /// Accept one reply line and return its code when the complete reply has arrived.
    /// @param line An ASCII line without CRLF.
    /// @return The final code, or no value while continuation lines are pending.
    /// @throws el::ApplicationError If reply syntax or continuation codes are inconsistent.
    auto receive(const el::String &line) -> std::optional<int> {
        const auto digits = line.slice(el::StringSide::Front, el::ByteCount{3});
        if (line.length() < el::ByteCount{3} || line.length() > el::ByteCount{510} ||
            !digits.containsOnly(el::AsciiCategory::Digit)) {
            throw el::ApplicationError{"Invalid SMTP reply."_el};
        }
        const auto code = digits.toIntegerOrThrow<int>();
        if (code < 200 || code > 599 || (_code && *_code != code)) {
            throw el::ApplicationError{"Inconsistent SMTP reply code."_el};
        }
        _code = code;
        if (++_lineCount > 100) {
            throw el::ApplicationError{"Too many SMTP continuation lines."_el};
        }
        if (line.length() > el::ByteCount{3}) {
            const auto separator = line.charAt(el::ByteIndex{3});
            if (separator == '-') {
                return {};
            }
            if (separator != ' ') {
                throw el::ApplicationError{"Invalid SMTP reply separator."_el};
            }
        }
        _code.reset();
        _lineCount = 0;
        return code;
    }

private:
    std::optional<int> _code; ///< Expected code of a continuation.
    int _lineCount{0};        ///< Bounded continuation count.
};

}
