// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/ReadNumberStatus.hpp"
#include "../text/StringView_fwd.hpp"

#include <cstddef>
#include <limits>
#include <string_view>

namespace erbsland::err {

/// Throw an encoding error with the given reason.
/// This method exists to resolve circular dependency issues and as convenience.
/// @param reason The reason for the encoding error.
[[noreturn]] void throwEncodingError(std::string_view reason);

/// Throw a UTF-8 encoding error with the given reason.
/// This method exists to resolve circular dependency issues and as convenience.
/// @param reason The reason for the encoding error.
/// @param index The index of the character that caused the error.
[[noreturn]] void throwU8EncodingError(std::string_view reason, std::size_t index);

/// Throw a UTF-16 encoding error with the given reason.
/// This method exists to resolve circular dependency issues and as convenience.
/// @param reason The reason for the encoding error.
/// @param index The index of the character that caused the error.
[[noreturn]] void throwU16EncodingError(std::string_view reason, std::size_t index);

/// Throw a UTF-32 encoding error with the given reason.
/// This method exists to resolve circular dependency issues and as convenience.
/// @param reason The reason for the encoding error.
/// @param index The index of the character that caused the error.
[[noreturn]] void throwU32EncodingError(std::string_view reason, std::size_t index);

/// Throw a format error with the given reason.
/// This method exists to resolve circular dependency issues and as convenience.
/// @param reason The reason for the format error.
/// @tested{U8FormatTest}
[[noreturn]] void throwFormatError(std::string_view reason);
/// @overload
[[noreturn]] void throwFormatError(text::StringView reason);

/// Throw an out-of-range error with the given reason.
/// This method exists to resolve circular dependency issues and as convenience.
/// @param reason The reason for the out-of-range error.
[[noreturn]] void throwOutOfRange(std::string_view reason);
/// @overload
[[noreturn]] void throwOutOfRange(text::StringView reason);

/// Throw an OverflowError with the given reason.
/// This method exists to resolve circular dependency issues and as convenience.
/// @param reason The reason for the overflow error.
[[noreturn]] void throwOverflow(std::string_view reason);
/// @overload
[[noreturn]] void throwOverflow(text::StringView reason);

/// Throw a ParseError with the given reason.
/// This method exists to resolve circular dependency issues and as convenience.
/// @param reason The reason for the parse error.
[[noreturn]] void throwParseError(std::string_view reason);
/// @overload
[[noreturn]] void throwParseError(text::StringView reason);

}
