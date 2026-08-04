// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/math/SaturatingInteger.hpp>
#include <erbsland/stream/impl/NativeOutputStream.hpp>
#include <erbsland/stream/impl/StandardTextOutputStream.hpp>
#include <erbsland/stream/StreamError.hpp>
#include <erbsland/text/BooleanFormat.hpp>
#include <erbsland/text/IntegerFormatFlag.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <string>

using namespace el::text::literals;

using el::stream::StreamError;
using el::stream::TextOutputStream;
using el::unit::ItemCount;
using namespace el::text;
using namespace el::text::literals;

TESTED_TARGETS(StandardTextOutputStream NativeOutputStream)
class StandardTextOutputStreamTest final : public el::UnitTest {
private:
    class PrintStringType final {
    public:
        [[nodiscard]] auto toString() const -> String { return String{"string"_el}; }
    };

    class PrintReadOnlyStringType final {
    public:
        [[nodiscard]] auto toString() const -> String { return "view"_el; }
    };

    class PrintRawValueType final {
    public:
        [[nodiscard]] constexpr auto toRawValue() const noexcept -> std::uint16_t { return 255U; }
    };

    class PrintBothType final {
    public:
        [[nodiscard]] auto toString() const -> String { return String{"string-wins"_el}; }
        [[nodiscard]] constexpr auto toRawValue() const noexcept -> std::uint16_t { return 17U; }
    };

    class MutableRawValueType final {
    public:
        [[nodiscard]] constexpr auto toRawValue() noexcept -> std::uint16_t { return 1U; }
    };

    class NonIntegerRawValueType final {
    public:
        [[nodiscard]] constexpr auto toRawValue() const noexcept -> float { return 1.0F; }
    };

    class CharacterRawValueType final {
    public:
        [[nodiscard]] constexpr auto toRawValue() const noexcept -> char { return 'x'; }
    };

    static_assert(el::stream::impl::PrintObjectWithToString<PrintStringType>);
    static_assert(el::stream::impl::PrintObjectWithToString<PrintReadOnlyStringType>);
    static_assert(el::stream::impl::PrintObjectWithToString<PrintBothType>);
    static_assert(el::stream::impl::PrintObjectWithRawInteger<PrintRawValueType>);
    static_assert(!el::stream::impl::PrintObjectWithRawInteger<PrintBothType>);
    static_assert(!el::stream::impl::PrintObjectWithRawInteger<MutableRawValueType>);
    static_assert(!el::stream::impl::PrintObjectWithRawInteger<NonIntegerRawValueType>);
    static_assert(!el::stream::impl::PrintObjectWithRawInteger<CharacterRawValueType>);

    class FakeNativeOutputStream final : public el::stream::impl::NativeOutputStream {
    public:
        void writeBytes(const std::span<const char> bytes) override {
            if (failOnWrite) {
                throw StreamError{el::stream::StreamErrorContext{
                    "Failed to write to the test output stream."_el,
                    "The configured test write failure was triggered."_el}};
            }
            text.append(bytes.data(), bytes.size());
        }

        void flush() override {
            if (failOnFlush) {
                throw StreamError{el::stream::StreamErrorContext{
                    "Failed to flush the test output stream."_el,
                    "The configured test flush failure was triggered."_el}};
            }
            flushCount += 1U;
        }

        void abort() noexcept override { aborted = true; }

    public:
        std::string text;
        std::size_t flushCount{0};
        bool failOnWrite{false};
        bool failOnFlush{false};
        bool aborted{false};
    };

public:
    void testWriteMethods() {

        const auto fake = std::make_shared<FakeNativeOutputStream>();
        auto stream = el::stream::impl::StandardTextOutputStream{fake};
        auto &textStream = static_cast<TextOutputStream &>(stream);

        REQUIRE_EQUAL(textStream.encoding(), StringEncoding::Utf8);
        REQUIRE_EQUAL(textStream.effectiveEncoding(), StringEncoding::Utf8);
        textStream.write("Hello"_el);
        textStream.write(Char{U' '});
        textStream.writeLine("World"_el);
        textStream.writeLine();
        textStream.flush();

        REQUIRE_EQUAL(fake->text, std::string{"Hello World\n\n"});
    }

    void testPrintConvenienceBuildsOneLine() {
        const auto fake = std::make_shared<FakeNativeOutputStream>();
        auto stream = el::stream::impl::StandardTextOutputStream{fake};

        auto integerFormat = IntegerFormat::hexadecimal();
        integerFormat.setFlags(IntegerFormatFlag::BasePrefix);
        auto floatFormat = FloatFormat::fixed();
        floatFormat.setPrecision(ItemCount{2U});
        auto booleanFormat = BooleanFormat::yesNo().setCapitalization(Capitalization::Titlecase);

        stream.printLine("value=", integerFormat, 255U, ", ok=", true, ", ratio=", floatFormat, 1.25);
        stream.printLine("styled=", booleanFormat, true, "|", false);
        stream.print("next");
        stream.printLine();
        stream.flush();

        REQUIRE_EQUAL(fake->text, std::string{"value=0xff, ok=true, ratio=1.25\nstyled=Yes|No\nnext\n"});
    }

    void testPrintConvenienceSupportsCustomObjects() {
        const auto fake = std::make_shared<FakeNativeOutputStream>();
        auto stream = el::stream::impl::StandardTextOutputStream{fake};

        auto integerFormat = IntegerFormat::hexadecimal();
        integerFormat.setFlags(IntegerFormatFlag::BasePrefix);

        stream.printLine(
            PrintStringType{},
            "|",
            PrintReadOnlyStringType{},
            "|",
            integerFormat,
            PrintRawValueType{},
            "|",
            PrintBothType{});
        stream.flush();

        REQUIRE_EQUAL(fake->text, std::string{"string|view|0xff|string-wins\n"});
    }

    void testPrintConvenienceSupportsSaturatingIntegers() {
        const auto fake = std::make_shared<FakeNativeOutputStream>();
        auto stream = el::stream::impl::StandardTextOutputStream{fake};

        stream.printLine("value=", el::math::SatInt32{42}, ", size=", std::size_t{7U});
        stream.flush();

        REQUIRE_EQUAL(fake->text, std::string{"value=42, size=7\n"});
    }

    void testPrintConvenienceNormalizesNativeIntegerWidths() {
        const auto fake = std::make_shared<FakeNativeOutputStream>();
        auto stream = el::stream::impl::StandardTextOutputStream{fake};

        stream.printLine(
            std::int8_t{-8},
            "|",
            std::uint8_t{8U},
            "|",
            std::int16_t{-16},
            "|",
            std::uint16_t{16U},
            "|",
            std::int32_t{-32},
            "|",
            std::uint32_t{32U},
            "|",
            std::int64_t{-64},
            "|",
            std::uint64_t{64U});
        stream.flush();

        REQUIRE_EQUAL(fake->text, std::string{"-8|8|-16|16|-32|32|-64|64\n"});
    }

    void testPrintConvenienceSupportsCharacterPointers() {
        const auto fake = std::make_shared<FakeNativeOutputStream>();
        auto stream = el::stream::impl::StandardTextOutputStream{fake};

        const auto *plainText = "plain";
        const auto *utf8Text = u8"utf8";
        const auto *utf16Text = u"utf16";
        const auto *utf32Text = U"utf32";

        stream.printLine('A', u8'B', u'C', U'D');
        stream.printLine(plainText, "|", utf8Text, "|", utf16Text, "|", utf32Text, "|", false);
        stream.printLine("null:", nullptr);
        stream.flush();

        REQUIRE_EQUAL(fake->text, std::string{"ABCD\nplain|utf8|utf16|utf32|false\nnull:\n"});
    }

    void testPrintConvenienceSupportsStandardStrings() {
        const auto fake = std::make_shared<FakeNativeOutputStream>();
        auto stream = el::stream::impl::StandardTextOutputStream{fake};

        const auto plainText = std::string{"plain"};
        const auto utf8Text = std::u8string{u8"utf8"};
        const auto utf16Text = std::u16string{u"utf16"};
        const auto utf32Text = std::u32string{U"utf32"};

        stream.printLine(plainText, "|", utf8Text, "|", utf16Text, "|", utf32Text);
        stream.flush();

        REQUIRE_EQUAL(fake->text, std::string{"plain|utf8|utf16|utf32\n"});
    }

    void testFlushDelegates() {
        const auto fake = std::make_shared<FakeNativeOutputStream>();
        auto stream = el::stream::impl::StandardTextOutputStream{fake};

        stream.flush();
        stream.flush();

        REQUIRE_EQUAL(fake->flushCount, std::size_t{2U});
    }

    void testCloseDrainsAndCloses() {
        const auto fake = std::make_shared<FakeNativeOutputStream>();
        auto stream = el::stream::impl::StandardTextOutputStream{fake};

        stream.printLine("Before close");
        const auto closeStatus = stream.close();
        REQUIRE_EQUAL(closeStatus, el::stream::StreamCloseStatus::Closed);
        REQUIRE_FALSE(stream.isOpen());
        REQUIRE_EQUAL(fake->text, std::string{"Before close\n"});
    }

    void testNativeErrorsPropagate() {
        const auto fake = std::make_shared<FakeNativeOutputStream>();
        auto stream = el::stream::impl::StandardTextOutputStream{fake};
        fake->failOnWrite = true;

        stream.write(StringEditor{std::string_view{"test"}});
        REQUIRE_THROWS_AS(StreamError, stream.flush());

        fake->failOnWrite = false;
        fake->failOnFlush = true;
        REQUIRE_THROWS_AS(StreamError, stream.flush());
    }

    void testMissingNativeStreamThrows() {
        REQUIRE_THROWS_AS(
            StreamError, el::stream::impl::StandardTextOutputStream{el::stream::impl::NativeOutputStreamPtr{}});
    }
};
