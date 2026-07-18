// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PrintContextCommonBuilder.hpp"

#include "../../text/StringConverter.hpp"

namespace erbsland::stream::impl {

using namespace text;

/// Prevent an extra copy when converting a string to be appended to a builder by choosing the correct string kind.
template <typename T>
void PrintContextCommonBuilder::convertAndAppendToBuilder(AnyStringBuilder &builder, T text) {
    if (builder.kind() == StringKind::U32) {
        builder.append(StringConverter{text}.toU32String());
    } else if (builder.kind() == StringKind::U16) {
        builder.append(StringConverter{text}.toU16String());
    } else {
        builder.append(StringConverter{text}.toU8String());
    }
}

void PrintContextCommonBuilder::print(const Char character) {
    builder().append(character);
}

void PrintContextCommonBuilder::print(const char character) {
    builder().append(Char{static_cast<char32_t>(static_cast<unsigned char>(character))});
}

void PrintContextCommonBuilder::print(const char8_t character) {
    builder().append(Char{static_cast<char32_t>(character)});
}

void PrintContextCommonBuilder::print(const char16_t character) {
    builder().append(Char{static_cast<char32_t>(character)});
}

void PrintContextCommonBuilder::print(const char32_t character) {
    builder().append(Char{character});
}

void PrintContextCommonBuilder::print(const char *text) {
    if (text != nullptr) {
        print(std::string_view{text});
    }
}

void PrintContextCommonBuilder::print(const char8_t *text) {
    if (text != nullptr) {
        print(std::u8string_view{text});
    }
}

void PrintContextCommonBuilder::print(const char16_t *text) {
    if (text != nullptr) {
        print(std::u16string_view{text});
    }
}

void PrintContextCommonBuilder::print(const char32_t *text) {
    if (text != nullptr) {
        print(std::u32string_view{text});
    }
}

void PrintContextCommonBuilder::print(std::nullptr_t) {
    // do nothing
}

void PrintContextCommonBuilder::print(const std::string_view text) {
    convertAndAppendToBuilder(builder(), text);
}

void PrintContextCommonBuilder::print(const std::u8string_view text) {
    convertAndAppendToBuilder(builder(), text);
}

void PrintContextCommonBuilder::print(const std::u16string_view text) {
    convertAndAppendToBuilder(builder(), text);
}

void PrintContextCommonBuilder::print(const std::u32string_view text) {
    convertAndAppendToBuilder(builder(), text);
}

void PrintContextCommonBuilder::print(const String &text) {
    builder().append(text);
}

void PrintContextCommonBuilder::print(const U16String &text) {
    builder().append(text);
}

void PrintContextCommonBuilder::print(const U32String &text) {
    builder().append(text);
}

void PrintContextCommonBuilder::print(bool value) {
    builder().append(_booleanFormat.text(value));
}

void PrintContextCommonBuilder::print(double value) {
    builder().appendFloat(value, _floatFormat);
}

void PrintContextCommonBuilder::print(float value) {
    builder().appendFloat(value, _floatFormat);
}

void PrintContextCommonBuilder::print(int64_t value) {
    builder().appendInteger(value, _integerFormat);
}

void PrintContextCommonBuilder::print(uint64_t value) {
    builder().appendInteger(value, _integerFormat);
}

void PrintContextCommonBuilder::print(const mem::ByteBlock &bytes) {
    builder().appendByteBlock(bytes, _byteFormat);
}

void PrintContextCommonBuilder::print(const BooleanFormat newFormat) {
    _booleanFormat = newFormat;
}

void PrintContextCommonBuilder::print(const ByteFormat newFormat) {
    _byteFormat = newFormat;
}

void PrintContextCommonBuilder::print(const IntegerFormat newFormat) {
    _integerFormat = newFormat;
}

void PrintContextCommonBuilder::print(const FloatFormat newFormat) {
    _floatFormat = newFormat;
}

}
