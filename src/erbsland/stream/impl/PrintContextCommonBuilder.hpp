// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../TextOutputStream_fwd.hpp"
#include "../TextPrintContext.hpp"

#include "../../text/AnyStringBuilder.hpp"
#include "../../text/BooleanFormat.hpp"
#include "../../text/ByteFormat.hpp"
#include "../../text/FloatFormat.hpp"
#include "../../text/IntegerFormat.hpp"

namespace erbsland::stream::impl {

/// A print context that uses the `write` method of a text stream to print a collected line.
/// @tested{StandardTextOutputStreamTest AnyStringBuilderStreamTest}
class PrintContextCommonBuilder : public TextPrintContext {
public:
    // defaults/deletions
    PrintContextCommonBuilder() = default;
    ~PrintContextCommonBuilder() override = default;
    PrintContextCommonBuilder(const PrintContextCommonBuilder &) = delete;
    PrintContextCommonBuilder(PrintContextCommonBuilder &&) = delete;
    auto operator=(const PrintContextCommonBuilder &) -> PrintContextCommonBuilder & = delete;
    auto operator=(PrintContextCommonBuilder &&) -> PrintContextCommonBuilder & = delete;

public:
    using TextPrintContext::print;

    void print(text::Char character) override;
    void print(char character) override;
    void print(char8_t character) override;
    void print(char16_t character) override;
    void print(char32_t character) override;
    void print(const char *text) override;
    void print(const char8_t *text) override;
    void print(const char16_t *text) override;
    void print(const char32_t *text) override;
    void print(std::nullptr_t) override;
    void print(std::string_view text) override;
    void print(std::u8string_view text) override;
    void print(std::u16string_view text) override;
    void print(std::u32string_view text) override;
    void print(const text::String &text) override;
    void print(const text::U16String &text) override;
    void print(const text::U32String &text) override;
    void print(bool value) override;
    void print(double value) override;
    void print(float value) override;
    void print(int64_t value) override;
    void print(uint64_t value) override;
    void print(const mem::ByteBlock &bytes) override;
    void print(text::BooleanFormat newFormat) override;
    void print(text::ByteFormat newFormat) override;
    void print(text::IntegerFormat newFormat) override;
    void print(text::FloatFormat newFormat) override;

private:
    /// Convert a value to the selected string kind and append it to the builder.
    template <typename T>
    static void convertAndAppendToBuilder(text::AnyStringBuilder &builder, T text);

protected:
    /// Access the builder for text output.
    [[nodiscard]] virtual auto builder() -> text::AnyStringBuilder & = 0;

protected:
    text::BooleanFormat _booleanFormat = text::BooleanFormat::defaultFormat();
    text::ByteFormat _byteFormat = text::ByteFormat::defaultFormat();
    text::IntegerFormat _integerFormat = text::IntegerFormat::defaultFormat();
    text::FloatFormat _floatFormat = text::FloatFormat::defaultFormat();
};

}
