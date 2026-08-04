// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StreamWriteStatus.hpp"
#include "TextPrintContext_fwd.hpp"

#include "impl/TextOutputStreamTraits.hpp"

#include "../mem/ByteBlock.hpp"
#include "../text/BooleanFormat_fwd.hpp"
#include "../text/ByteFormat_fwd.hpp"
#include "../text/Char.hpp"
#include "../text/FloatFormat_fwd.hpp"
#include "../text/IntegerFormat_fwd.hpp"
#include "../text/String_fwd.hpp"
#include "../text/u16/U16String_fwd.hpp"
#include "../text/u32/U32String_fwd.hpp"
#include "../text/u8/U8String_fwd.hpp"

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>

namespace erbsland::stream {

/// The print context interface for the print commands in the text streams.
/// You only need this interface if you implement a custom text stream subclass that uses an
/// unusual backend storage, for all other uses, rely the default implementation and implement the `write` methods.
/// @tested{StandardTextOutputStreamTest AnyStringBuilderStreamTest}
class TextPrintContext {
public:
    // defaults
    virtual ~TextPrintContext() = default;

public:
    /// Commit the printed content
    virtual auto commit() -> StreamWriteStatus = 0;

public: // main overloads
    /// Print a character.
    virtual void print(text::Char character) = 0;
    /// Print a native character.
    virtual void print(char character) = 0;
    /// Print a UTF-8 code unit.
    virtual void print(char8_t character) = 0;
    /// Print a UTF-16 code unit.
    virtual void print(char16_t character) = 0;
    /// Print a UTF-32 code point.
    virtual void print(char32_t character) = 0;
    /// Print a null-terminated native string.
    virtual void print(const char *text) = 0;
    /// Print a null-terminated UTF-8 string.
    virtual void print(const char8_t *text) = 0;
    /// Print a null-terminated UTF-16 string.
    virtual void print(const char16_t *text) = 0;
    /// Print a null-terminated UTF-32 string.
    virtual void print(const char32_t *text) = 0;
    /// Print a null pointer as a null value.
    virtual void print(std::nullptr_t) = 0;
    /// Print a native string view.
    virtual void print(std::string_view text) = 0;
    /// Print a UTF-8 string view.
    virtual void print(std::u8string_view text) = 0;
    /// Print a UTF-16 string view.
    virtual void print(std::u16string_view text) = 0;
    /// Print a UTF-32 string view.
    virtual void print(std::u32string_view text) = 0;
    /// Print an string.
    virtual void print(const text::String &text) = 0;
    /// Print an UTF-16 string.
    virtual void print(const text::U16String &text) = 0;
    /// Print an UTF-32 string.
    virtual void print(const text::U32String &text) = 0;
    /// Print a Boolean value.
    virtual void print(bool value) = 0;
    /// Print a double-precision floating-point value.
    virtual void print(double value) = 0;
    /// Print a single-precision floating-point value.
    virtual void print(float value) = 0;
    /// Print a signed 64-bit integer.
    virtual void print(int64_t value) = 0;
    /// Print an unsigned 64-bit integer.
    virtual void print(uint64_t value) = 0;
    /// Print a block of bytes.
    virtual void print(const mem::ByteBlock &bytes) = 0;

public: // format control
    /// Set the Boolean output format.
    virtual void print(text::BooleanFormat newFormat) = 0;
    /// Set the byte output format.
    virtual void print(text::ByteFormat newFormat) = 0;
    /// Set the integer output format.
    virtual void print(text::IntegerFormat newFormat) = 0;
    /// Set the floating-point output format.
    virtual void print(text::FloatFormat newFormat) = 0;

public:
    /// Print a native string literal.
    template <std::size_t N>
    void print(const char (&text)[N]) {
        print(std::string_view{text, N - 1U});
    }
    /// Print a UTF-8 string literal.
    template <std::size_t N>
    void print(const char8_t (&text)[N]) {
        print(std::u8string_view{text, N - 1U});
    }
    /// Print a UTF-16 string literal.
    template <std::size_t N>
    void print(const char16_t (&text)[N]) {
        print(std::u16string_view{text, N - 1U});
    }
    /// Print a UTF-32 string literal.
    template <std::size_t N>
    void print(const char32_t (&text)[N]) {
        print(std::u32string_view{text, N - 1U});
    }
    /// Print a native string without copying it.
    void print(const std::string &text) { print(std::string_view{text.data(), text.size()}); }
    /// Print a UTF-8 string without copying it.
    void print(const std::u8string &text) { print(std::u8string_view{text.data(), text.size()}); }
    /// Print a UTF-16 string without copying it.
    void print(const std::u16string &text) { print(std::u16string_view{text.data(), text.size()}); }
    /// Print a UTF-32 string without copying it.
    void print(const std::u32string &text) { print(std::u32string_view{text.data(), text.size()}); }
    /// Print a native integer.
    template <math::NativeInteger T>
        requires(!impl::PrintCharacterArgument<std::remove_cvref_t<T>>)
    void print(const T value) {
        if constexpr (std::signed_integral<T>) {
            print(static_cast<int64_t>(value));
        } else {
            print(static_cast<uint64_t>(value));
        }
    }
    /// Print an object using its `toString()` result.
    template <impl::PrintObjectWithToString T>
    void print(const T &value) {
        print(value.toString());
    }
    /// Print an object using its `toRawValue()` result.
    template <impl::PrintObjectWithRawInteger T>
    void print(const T &value) {
        print(value.toRawValue());
    }
};

}
