// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnyString_fwd.hpp"
#include "AnyStringEditor_fwd.hpp"
#include "Char.hpp"
#include "CharCompareFn.hpp"
#include "CharSet.hpp"
#include "IntegerBase.hpp"
#include "IntegerParseOptions.hpp"
#include "ReadIntegerResult.hpp"
#include "String_fwd.hpp"
#include "StringCharReader_fwd.hpp"
#include "StringCharReaderState.hpp"

#include "impl/StringReaderBase.hpp"
#include "u16/U16String_fwd.hpp"
#include "u16/U16StringEditor_fwd.hpp"
#include "u32/U32String_fwd.hpp"
#include "u32/U32StringEditor_fwd.hpp"
#include "u8/U8String_fwd.hpp"
#include "u8/U8StringEditor_fwd.hpp"

#include "../math/IntegerTraits.hpp"
#include "../mem/SharedDataPointer.hpp"
#include "../unit/CpIndex.hpp"
#include "../unit/CpLength.hpp"
#include "../util/LoopResult.hpp"
#include "../util/LoopStatus.hpp"

#include <optional>
#include <string_view>

namespace erbsland::text {

/// A decoded-character reader for all string encodings.
///
/// The reader accepts UTF-8, UTF-16, and UTF-32 strings and exposes one sequential code-point based read API.
/// Copies share the immutable source data but keep independent reader positions.
/// @seedoc{/reference/text/string_reader}
/// @tested{StringCharReaderTest}
class StringCharReader final {
public:
    /// A function type for reading characters.
    /// Returns `true` to continue reading, `false` to stop.
    using ReadFn = std::function<util::LoopStatus(Char)>;

public:
    /// Create an empty reader.
    StringCharReader();
    /// Create a reader for a UTF-8 string.
    explicit StringCharReader(const U8StringEditor &text);
    /// Create a reader for a UTF-8 read-only string.
    explicit StringCharReader(const U8String &text);
    /// Create a reader for a UTF-16 string.
    explicit StringCharReader(const U16StringEditor &text);
    /// Create a reader for a UTF-16 read-only string.
    explicit StringCharReader(const U16String &text);
    /// Create a reader for a UTF-32 string.
    explicit StringCharReader(const U32StringEditor &text);
    /// Create a reader for a UTF-32 read-only string.
    explicit StringCharReader(const U32String &text);
    /// Create a reader for AnyStringEditor.
    explicit StringCharReader(const AnyStringEditor &text);
    /// Create a reader for AnyString.
    explicit StringCharReader(const AnyString &text);

    // defaults
    ~StringCharReader() = default;
    StringCharReader(const StringCharReader &) = default;
    StringCharReader(StringCharReader &&) = default;
    auto operator=(const StringCharReader &) -> StringCharReader & = default;
    auto operator=(StringCharReader &&) -> StringCharReader & = default;

public:
    /// Get the current decoded code-point position.
    /// The value is suitable for user-facing error locations. Malformed encoded units count as one replacement
    /// character in tolerant operations.
    /// @return The current code-point position.
    [[nodiscard]] auto position() const noexcept -> unit::CpIndex;
    /// Save the current reader state.
    /// The saved state is only restorable for a reader over the same visible storage range and encoding backend.
    /// @return A saved reader state. This state only works for readers with the same backend and visible storage range.
    [[nodiscard]] auto save() const noexcept -> StringCharReaderState;
    /// Restore a saved reader state.
    /// Returns `false` and keeps the current position unchanged if the state belongs to a different backend, a
    /// different visible storage range, or an invalid raw/code-point position pair.
    /// @param state The readers state to restore.
    /// @return `true` if the state was successfully restored, `false` otherwise.
    auto restore(StringCharReaderState state) noexcept -> bool;
    /// Reset the reader position to the start of the string.
    void reset() noexcept;
    /// Read a character, tolerating malformed encoding, and advance on success.
    /// At the end of data, this returns `Char::endOfData()` and keeps the current position unchanged.
    /// @return The read character, or `Char::endOfData()` if at the end of data.
    auto read() noexcept -> Char;
    /// Read a character only if it matches a given character.
    /// If there is no match, the current position is unchanged.
    /// Signal characters never match. Malformed encoding is handled like `read()` and may match a replacement
    /// character.
    /// @param expected The character to match.
    /// @return `true` if the character was read and matched, `false` otherwise.
    auto readIf(Char expected) noexcept -> bool;
    /// Read a character only if it matches a given character set.
    /// If there is no match, the current position is unchanged.
    /// Signal characters never match. Malformed encoding is handled like `read()` and may match a replacement
    /// character from the expected set.
    /// @param expected The character set to match.
    /// @return The character if it matches, `std::nullopt` otherwise.
    auto readIf(const CharSet &expected) noexcept -> std::optional<Char>;
    /// Peek a character, tolerating malformed encoding.
    /// This method never advances the current position.
    [[nodiscard]] auto peek() const noexcept -> Char;
    /// Advance by one decoded character (Skip one character).
    /// @return `true` if the cursor was moved, `false` if at the end of data.
    auto advance() noexcept -> bool;
    /// Advance one character if it matches a given character (Skip if).
    /// Signal characters never match.
    /// @param expected The character to match.
    /// @return `true` if the character matched and the position was advanced, `false` otherwise.
    auto advanceIf(Char expected) noexcept -> bool;
    /// Advance one character if it matches a given character set (Skip if).
    /// Signal characters never match.
    /// @param expected The character set to match.
    /// @return `true` if the character matched and the position was advanced, `false` otherwise.
    auto advanceIf(const CharSet &expected) noexcept -> bool;
    /// Advance if the following decoded characters match an expected UTF-8 string.
    /// The current position remains unchanged if the complete string does not match. An empty expected string
    /// succeeds without changing the position. Malformed source and expected text is compared tolerantly as
    /// replacement characters.
    /// @param expected The decoded character sequence to match.
    /// @param compareFn The optional character comparison function.
    /// @return `true` if the complete string matched and was skipped, `false` otherwise.
    auto advanceIf(const String &expected, CharCompareFn compareFn = {}) noexcept -> bool;
    /// Advance by decoded characters.
    /// Tolerant decoding rules are used for malformed encoded data.
    /// Returns `false` when no movement was possible, returns `true` if moves at least one character.
    auto advance(unit::CpLength count) noexcept -> bool;
    /// Advance by one decoded character or throw if no movement was possible.
    void advanceOrThrow();
    /// Advance by decoded characters or throw if no movement was possible.
    void advanceOrThrow(unit::CpLength count);
    /// Test if the reader is at the end of data.
    [[nodiscard]] auto isAtEnd() const noexcept -> bool;
    /// Test if at least `count` decoded characters are available.
    [[nodiscard]] auto canRead(unit::CpLength count) const noexcept -> bool;

public: // read loops
    /// Read characters while they match an expected set of characters.
    /// Does not consume a character that does not match.
    /// If the read function requests a stop or an error,
    /// the reader position is pointing to the character that caused the stop or error.
    /// @param readFn The read function, or nullptr to continue for all read characters.
    /// @param expected The expected character set.
    /// @param maximum The maximum number of characters to read.
    /// @return `LoopResult::Success` when the first character does not match, `LoopResult::Stopped` when
    ///     the read function requested a stop, `LoopResult::LimitReached` if maximum was reached, but the next
    ///     character would match `expected` too, `LoopResult::EndOfData` if the end of the string was reached,
    ///     `LoopResult::Error` if the function reports an error.
    auto readWhile(
        const ReadFn &readFn, const CharSet &expected, unit::CpLength maximum = unit::CpLength::infinite()) noexcept
        -> util::LoopResult;
    /// Read characters until, but without a character from the given set.
    /// Does not consume the stop character.
    /// @param readFn The read function, or nullptr to continue for all read characters.
    /// @param stopSet The set with stop characters.
    /// @param maximum The maximum number of characters to read.
    /// @return `LoopResult::Success` when a stop character was encountered, `LoopResult::Stopped` when
    ///     the read requested a stop, `LoopResult::LimitReached` if maximum was reached, but the next character
    ///     would not match `stopSet`, `LoopResult::EndOfData` if the end of the string was reached,
    ///     `LoopResult::Error` if the function reports an error.
    auto readUntil(
        const ReadFn &readFn, const CharSet &stopSet, unit::CpLength maximum = unit::CpLength::infinite()) noexcept
        -> util::LoopResult;
    /// Advance while decoded characters match `expected`.
    /// Stops before the first nonmatching character, at the end of data, or after `maximum` characters. Malformed
    /// encoded data is handled tolerantly like `read()`. The returned count may be ignored when only the skip side
    /// effect is needed.
    /// @param expected The expected character set.
    /// @param maximum The maximum number of characters to advance.
    /// @return The number of decoded characters skipped.
    auto advanceWhile(const CharSet &expected, unit::CpLength maximum = unit::CpLength::infinite()) noexcept
        -> unit::CpLength;
    /// Advance until, but without consuming, a decoded character from `stopSet`.
    /// Stops before the first matching character, at the end of data, or after `maximum` characters. Malformed
    /// encoded data is handled tolerantly like `read()`. The returned count may be ignored when only the skip side
    /// effect is needed.
    /// @param stopSet The set with stop characters.
    /// @param maximum The maximum number of characters to advance.
    /// @return The number of decoded characters skipped.
    auto advanceUntil(const CharSet &stopSet, unit::CpLength maximum = unit::CpLength::infinite()) noexcept
        -> unit::CpLength;

public: // read integers
    /// Parse an integer using configurable low-level parse options.
    /// On failure, the reader position is restored to the original state.
    /// @param options The integer parsing options.
    /// @return The parse result.
    [[nodiscard]] auto parseInteger(const IntegerParseOptions &options) noexcept -> ReadIntegerResult;
    /// Read an integer using configurable low-level parse options.
    /// On failure, the reader position is restored to the original state.
    /// @tparam T The native or saturating integer type to read.
    /// @param options The integer parsing options.
    /// @return The parsed integer converted into the requested type.
    /// @throws text::ParseNumberError when the integer cannot be read.
    /// @throws err::OverflowError when the integer cannot be converted into the requested type.
    template <math::AnyIntegerType T>
    [[nodiscard]] auto readIntegerOrThrow(const IntegerParseOptions &options = IntegerParseOptions::parserDefault())
        -> T;

public: // capture strings
    /// Set the capture start position.
    void startCapture() noexcept;
    /// Take the current capture and set the new capture start point to the current reader position.
    /// If the capture point is at or before the start position, an empty string is returned.
    /// The returned string always matches the encoding of the parsed string,
    /// it is returned as an inexpensive slice of that string - no copy is made.
    /// @return The captured string or an empty string if the capture point is invalid.
    [[nodiscard]] auto takeCapture() noexcept -> AnyString;

public: // buffer
    /// Clear all text from the buffer while keeping any retained capacity.
    void clearBuffer() noexcept;
    /// Move out the buffer and reset it.
    /// @return The buffered text as an owning string.
    [[nodiscard]] auto takeBuffer() -> AnyString;
    /// Get the current buffer content without consuming it.
    /// @return The buffer text.
    [[nodiscard]] auto bufferView() const noexcept -> AnyString;
    /// Get the current decoded code-point length of the buffer.
    /// @return The buffer length in decoded code points.
    [[nodiscard]] auto bufferCharacterLength() const noexcept -> unit::CpLength;
    /// Test if the buffer is empty.
    [[nodiscard]] auto isBufferEmpty() const noexcept -> bool;
    /// Replace the buffer with text converted to the reader encoding.
    /// @param text The new buffer content.
    void setBuffer(const AnyString &text);
    /// Append one Unicode code point to the buffer.
    /// Signal characters are ignored.
    /// @param character The character to append.
    void appendToBuffer(Char character);
    /// Append text to the buffer, converting it to the reader encoding if needed.
    /// @param text The text to append.
    void appendToBuffer(const AnyString &text);
    /// Take the current capture and append it to the buffer.
    void appendCaptureToBuffer();
    /// Read a character and append it to the buffer.
    /// At the end of data, this returns `Char::endOfData()` and does not change the buffer.
    /// @return The read character, or `Char::endOfData()` if at the end of data.
    auto readToBuffer() -> Char;
    /// Read a character only if it matches and append it to the buffer.
    /// @param expected The character to match.
    /// @return `true` if the character was read, matched, and appended.
    [[nodiscard]] auto readToBufferIf(Char expected) -> bool;
    /// Read a character only if it matches and append it to the buffer.
    /// @param expected The character set to match.
    /// @return The character if it matched and was appended, `std::nullopt` otherwise.
    [[nodiscard]] auto readToBufferIf(const CharSet &expected) -> std::optional<Char>;
    /// Read matching characters and append them to the buffer.
    /// @param expected The expected character set.
    /// @param maximum The maximum number of characters to read.
    /// @return The loop result, with the same meaning as `readWhile()`.
    [[nodiscard]] auto readToBufferWhile(const CharSet &expected, unit::CpLength maximum = unit::CpLength::infinite())
        -> util::LoopResult;
    /// Read characters until, but without, a stop character and append them to the buffer.
    /// @param stopSet The set with stop characters.
    /// @param maximum The maximum number of characters to read.
    /// @return The loop result, with the same meaning as `readUntil()`.
    [[nodiscard]] auto readToBufferUntil(const CharSet &stopSet, unit::CpLength maximum = unit::CpLength::infinite())
        -> util::LoopResult;

private:
    using ReaderPtr = mem::SharedDataPointer<impl::StringReaderBase>;

private:
    /// Create a reader backend matching an any-string representation.
    [[nodiscard]] static auto createBackendForAnyString(const AnyString &text) -> impl::StringReaderBase *;
    /// Scan an integer without throwing for ordinary parse failures.
    [[nodiscard]] auto scanInteger(const IntegerParseOptions &options) -> ReadIntegerResult;
    /// Scan an integer and throw on failure.
    [[nodiscard]] auto readIntegerResultOrThrow(const IntegerParseOptions &options) -> ReadIntegerResult;
    /// Throw a numeric-reading error at a source position.
    [[noreturn]] static void throwError(ReadNumberStatus status, unit::CpIndex position);

private:
    ReaderPtr _reader; ///< The shared reader backend.
};

}

#include "StringCharReader_integer.tpp"
