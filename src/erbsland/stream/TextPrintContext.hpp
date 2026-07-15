// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StreamWriteStatus.hpp"

#include "impl/TextOutputStreamTraits.hpp"

#include "../mem/ByteBlockView.hpp"
#include "../text/BooleanFormat_fwd.hpp"
#include "../text/ByteFormat_fwd.hpp"
#include "../text/Char.hpp"
#include "../text/FloatFormat_fwd.hpp"
#include "../text/IntegerFormat_fwd.hpp"
#include "../text/StringView_fwd.hpp"
#include "../text/u16/U16StringView_fwd.hpp"
#include "../text/u32/U32StringView_fwd.hpp"
#include "../text/u8/U8StringView_fwd.hpp"

#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace erbsland::stream {

class TextPrintContext;
using TextPrintContextPtr = std::unique_ptr<TextPrintContext>;

/// The print context interface for the print commands in the text streams.
/// You only need this interface if you implement a custom text stream subclass that uses an
/// unusual backend storage, for all other uses, rely the default implementation and implement the `write` methods.
/// @tested{StandardTextOutputStreamTest StringBuilderStreamTest}
class TextPrintContext {
public:
    virtual ~TextPrintContext() = default;

public:
    /// Commit the printed content
    virtual auto commit() -> StreamWriteStatus = 0;

public:                                                      // main overloads
    virtual void print(text::Char character) = 0;            ///< print a value.
    virtual void print(char character) = 0;                  ///< print a value.
    virtual void print(char8_t character) = 0;               ///< print a value.
    virtual void print(char16_t character) = 0;              ///< print a value.
    virtual void print(char32_t character) = 0;              ///< print a value.
    virtual void print(const char *text) = 0;                ///< print a value.
    virtual void print(const char8_t *text) = 0;             ///< print a value.
    virtual void print(const char16_t *text) = 0;            ///< print a value.
    virtual void print(const char32_t *text) = 0;            ///< print a value.
    virtual void print(std::nullptr_t) = 0;                  ///< print a value.
    virtual void print(std::string_view text) = 0;           ///< print a value.
    virtual void print(std::u8string_view text) = 0;         ///< print a value.
    virtual void print(std::u16string_view text) = 0;        ///< print a value.
    virtual void print(std::u32string_view text) = 0;        ///< print a value.
    virtual void print(const text::StringView &text) = 0;    ///< print a value.
    virtual void print(const text::U16StringView &text) = 0; ///< print a value.
    virtual void print(const text::U32StringView &text) = 0; ///< print a value.
    virtual void print(bool value) = 0;                      ///< print a value.
    virtual void print(double value) = 0;                    ///< print a value.
    virtual void print(float value) = 0;                     ///< print a value.
    virtual void print(int64_t value) = 0;                   ///< print a value.
    virtual void print(uint64_t value) = 0;                  ///< print a value.
    virtual void print(const mem::ByteBlockView &bytes) = 0; ///< print a value.

public:                                                      // format control
    virtual void print(text::BooleanFormat newFormat) = 0;   ///< change the format.
    virtual void print(text::ByteFormat newFormat) = 0;      ///< change the format.
    virtual void print(text::IntegerFormat newFormat) = 0;   ///< change the format.
    virtual void print(text::FloatFormat newFormat) = 0;     ///< change the format.

public:
    template <std::size_t N>
    void print(const char (&text)[N]) { ///< print a literal
        print(std::string_view{text, N - 1U});
    }
    template <std::size_t N>
    void print(const char8_t (&text)[N]) { ///< print a literal
        print(std::u8string_view{text, N - 1U});
    }
    template <std::size_t N>
    void print(const char16_t (&text)[N]) { ///< print a literal
        print(std::u16string_view{text, N - 1U});
    }
    template <std::size_t N>
    void print(const char32_t (&text)[N]) { ///< print a literal
        print(std::u32string_view{text, N - 1U});
    }
    void print(const std::string &text) { ///< print a value.
        print(std::string_view{text.data(), text.size()});
    }
    void print(const std::u8string &text) { ///< print a value.
        print(std::u8string_view{text.data(), text.size()});
    }
    void print(const std::u16string &text) { ///< print a value.
        print(std::u16string_view{text.data(), text.size()});
    }
    void print(const std::u32string &text) { ///< print a value.
        print(std::u32string_view{text.data(), text.size()});
    }
    template <math::NativeInteger T>
        requires(!impl::PrintCharacterArgument<std::remove_cvref_t<T>>)
    void print(const T value) { ///< print a native integer.
        if constexpr (std::signed_integral<T>) {
            print(static_cast<int64_t>(value));
        } else {
            print(static_cast<uint64_t>(value));
        }
    }
    template <impl::PrintObjectWithToString T>
    void print(const T &value) { ///< print obj with `toString()`.
        print(value.toString());
    }
    template <impl::PrintObjectWithRawInteger T>
    void print(const T &value) { ///< print obj with `toRawValue()`.
        print(value.toRawValue());
    }
};

}
