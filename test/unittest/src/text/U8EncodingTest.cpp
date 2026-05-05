// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteReader.hpp>
#include <erbsland/text/u8/impl/U8Encoding.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unittest/TextHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <vector>

using el::mem::ByteBlock;
using el::mem::ByteReader;
using el::text::Char;
using el::text::EncodingErrorMode;
using el::unit::ByteIndex;

namespace th = erbsland::unittest::th;

TESTED_TARGETS(U8Encoding)
class U8EncodingTest final : public el::UnitTest {
public:
    void testDecodeContinuationByte() {
        auto unicodeValue = char32_t{0b10};
        auto bytes = byteArray(0x80U);

        el::text::impl::utf8::decodeCharOrThrow_decodeCont(std::span<char>{bytes}, 0U, unicodeValue);

        REQUIRE_EQUAL(unicodeValue, char32_t{0b10000000});
    }

    void testDecodeContinuationByteErrors() {
        auto unicodeValue = char32_t{};
        auto bytes = byteArray(0x20U);
        auto empty = std::array<char, 0>{};

        REQUIRE_THROWS(el::text::impl::utf8::decodeCharOrThrow_decodeCont(std::span<char>{empty}, 0U, unicodeValue));
        REQUIRE_THROWS(el::text::impl::utf8::decodeCharOrThrow_decodeCont(std::span<char>{bytes}, 0U, unicodeValue));
    }

    void testDecodeCharOrThrowSuccessCases() {
        requireDecodeOrThrow({0x41U}, U'A', 1U);
        requireDecodeOrThrow({0xC2U, 0xA2U}, U'\u00A2', 2U);
        requireDecodeOrThrow({0xE2U, 0x82U, 0xACU}, U'\u20AC', 3U);
        requireDecodeOrThrow({0xF0U, 0x9FU, 0x98U, 0x80U}, U'\U0001F600', 4U);
    }

    void testDecodeCharOrThrowErrors() {
        auto empty = std::array<char, 0>{};
        auto position = ByteIndex::zero();

        REQUIRE_THROWS(el::text::impl::utf8::decodeCharOrThrow(std::span<char>{empty}, position));

        requireDecodeOrThrowFails({0xC0U, 0x80U}, 0U);
        requireDecodeOrThrowFails({0xE2U}, 0U);
        requireDecodeOrThrowFails({0xE2U, 0x28U, 0xA1U}, 0U);
        requireDecodeOrThrowFails({0xE0U, 0x80U, 0x80U}, 0U);
        requireDecodeOrThrowFails({0xF0U, 0x80U, 0x80U, 0x80U}, 0U);
        requireDecodeOrThrowFails({0xF5U, 0x80U, 0x80U, 0x80U}, 0U);
        requireDecodeOrThrowFails({0xEDU, 0xA0U, 0x80U}, 0U);
        requireDecodeOrThrowFails({0xF4U, 0x90U, 0x80U, 0x80U}, 0U);
    }

    void testDecodeCharOrThrowAllMalformedUtf8Categories() {
        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireDecodeOrThrowFailsForMalformedUtf8(error));
        }
    }

    void testDecodeCharOrReplaceSuccessCases() {
        requireDecodeOrReplace({0x41U}, U'A', 1U);
        requireDecodeOrReplace({0xC2U, 0xA2U}, U'\u00A2', 2U);
        requireDecodeOrReplace({0xE2U, 0x82U, 0xACU}, U'\u20AC', 3U);
        requireDecodeOrReplace({0xF0U, 0x9FU, 0x98U, 0x80U}, U'\U0001F600', 4U);
    }

    void testDecodeCharOrReplaceErrors() {
        auto empty = std::array<char, 0>{};
        auto position = ByteIndex::zero();

        const auto result = el::text::impl::utf8::decodeCharOrReplace(std::span<char>{empty}, position);

        REQUIRE_EQUAL(result.toRawValue(), U'\0');
        REQUIRE(position.isZero());

        requireDecodeOrReplace({0xC0U, 0x80U}, 0xFFFDU, 1U);
        requireDecodeOrReplace({0xE2U}, 0xFFFDU, 1U);
        requireDecodeOrReplace({0xE2U, 0x28U, 0xA1U}, 0xFFFDU, 1U);
        requireDecodeOrReplace({0xE0U, 0x80U, 0x80U}, 0xFFFDU, 1U);
        requireDecodeOrReplace({0xF0U, 0x80U, 0x80U, 0x80U}, 0xFFFDU, 1U);
        requireDecodeOrReplace({0xEDU, 0xA0U, 0x80U}, 0xFFFDU, 3U);
        requireDecodeOrReplace({0xF4U, 0x90U, 0x80U, 0x80U}, 0xFFFDU, 4U);
    }

    void testDecodeCharOrReplaceAllMalformedUtf8Categories() {
        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireDecodeOrReplaceMalformedUtf8(error));
        }
    }

    void testDecodeCharOrIgnoreSuccessCases() {
        requireDecodeOrIgnore({0x41U}, U'A', 1U);
        requireDecodeOrIgnore({0xC2U, 0xA2U}, U'\u00A2', 2U);
        requireDecodeOrIgnore({0xE2U, 0x82U, 0xACU}, U'\u20AC', 3U);
        requireDecodeOrIgnore({0xF0U, 0x9FU, 0x98U, 0x80U}, U'\U0001F600', 4U);
    }

    void testDecodeCharOrIgnoreErrors() {
        auto empty = std::array<char, 0>{};
        auto position = ByteIndex::zero();

        REQUIRE_FALSE(el::text::impl::utf8::decodeCharOrIgnore(std::span<char>{empty}, position).has_value());
        REQUIRE(position.isZero());

        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireDecodeOrIgnoreMalformedUtf8(error));
        }
    }

    void testReaderDecodeCharSuccessCases() {
        requireReaderDecodeOrThrow({0x41U}, U'A', 1U);
        requireReaderDecodeOrThrow({0xC2U, 0xA2U}, U'\u00A2', 2U);
        requireReaderDecodeOrThrow({0xE2U, 0x82U, 0xACU}, U'\u20AC', 3U);
        requireReaderDecodeOrThrow({0xF0U, 0x9FU, 0x98U, 0x80U}, U'\U0001F600', 4U);

        requireReaderDecodeOrReplace({0xE2U, 0x82U, 0xACU}, U'\u20AC', 3U);
        requireReaderDecodeOrIgnore({0xF0U, 0x9FU, 0x98U, 0x80U}, U'\U0001F600', 4U);
    }

    void testReaderDecodeCharErrorMovement() {
        auto emptyReader = ByteReader{ByteBlock{}};
        REQUIRE_THROWS(el::text::impl::utf8::decodeCharOrThrow(emptyReader));
        REQUIRE(emptyReader.position().isZero());

        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireReaderDecodeOrThrowFailsForMalformedUtf8(error));
            WITH_CONTEXT(requireReaderDecodeOrReplaceMalformedUtf8(error));
            WITH_CONTEXT(requireReaderDecodeOrIgnoreMalformedUtf8(error));
        }
    }

    void testReaderForEachDecodedCharacterUsesCurrentPosition() {
        auto reader = ByteReader{makeBlock({0x78U, 0x41U, 0xC2U, 0xA2U})};
        auto collected = std::u32string{};
        reader.setPosition(ByteIndex{1U});

        const auto completed = el::text::impl::utf8::forEachDecodedCharacter(
            reader, EncodingErrorMode::Throw, [&](const Char character) -> void {
                collected.push_back(character.toRawValue());
            });

        REQUIRE(completed);
        REQUIRE_EQUAL(collected, std::u32string{U"A¢"});
        REQUIRE(reader.isAtEnd());
    }

    void testCompileTimeForEachDecodedCharacter() {
        const auto text = th::stdStringFromHex("41 C2 A2");
        auto spanCollected = std::u32string{};
        auto readerCollected = std::u32string{};
        auto reader = ByteReader{makeBlock(text)};

        REQUIRE(
            el::text::impl::utf8::forEachDecodedCharacter<EncodingErrorMode::Throw>(
                std::span<const char>{text.data(), text.size()},
                [&](const Char character) -> void { spanCollected.push_back(character.toRawValue()); }));
        REQUIRE(
            el::text::impl::utf8::forEachDecodedCharacter<EncodingErrorMode::Throw>(
                reader, [&](const Char character) -> void { readerCollected.push_back(character.toRawValue()); }));

        REQUIRE_EQUAL(spanCollected, std::u32string{U"A¢"});
        REQUIRE_EQUAL(readerCollected, std::u32string{U"A¢"});
        REQUIRE(reader.isAtEnd());
    }

    void testFastAdvanceCharSuccessCases() {
        requireFastAdvance({0x41U}, 1U);
        requireFastAdvance({0xC2U, 0xA2U}, 2U);
        requireFastAdvance({0xE2U, 0x82U, 0xACU}, 3U);
        requireFastAdvance({0xF0U, 0x9FU, 0x98U, 0x80U}, 4U);
    }

    void testFastAdvanceCharErrorsAndKnownExceptions() {
        auto empty = std::array<char, 0>{};
        auto position = ByteIndex::zero();

        el::text::impl::utf8::fastAdvanceChar(std::span<char>{empty}, position);
        REQUIRE(position.isZero());

        requireFastAdvance({0x80U}, 1U);
        requireFastAdvance({0xC0U, 0x80U}, 1U);
        requireFastAdvance({0xE2U}, 1U);
        requireFastAdvance({0xE2U, 0x28U, 0xA1U}, 1U);
        requireFastAdvance({0xE0U, 0x80U, 0x80U}, 1U);
        requireFastAdvance({0xF0U, 0x80U, 0x80U, 0x80U}, 1U);
        requireFastAdvance({0xF5U, 0x80U, 0x80U, 0x80U}, 1U);
        requireFastAdvance({0xEDU, 0xA0U, 0x80U}, 3U);
        requireFastAdvance({0xF4U, 0x90U, 0x80U, 0x80U}, 4U);
    }

    void testFastRetreatCharSuccessCases() {
        requireFastRetreat({0x41U}, 0U);
        requireFastRetreat({0xC2U, 0xA2U}, 0U);
        requireFastRetreat({0xE2U, 0x82U, 0xACU}, 0U);
        requireFastRetreat({0xF0U, 0x9FU, 0x98U, 0x80U}, 0U);
    }

    void testFastRetreatCharErrorsAndKnownExceptions() {
        auto empty = std::array<char, 0>{};
        auto position = ByteIndex::zero();

        el::text::impl::utf8::fastRetreatChar(std::span<char>{empty}, position);
        REQUIRE(position.isZero());

        requireFastRetreat({0x80U}, 0U);
        requireFastRetreat({0xC0U, 0x80U}, 1U);
        requireFastRetreat({0xE2U}, 0U);
        requireFastRetreat({0xE2U, 0x28U, 0xA1U}, 2U);
        requireFastRetreat({0xE0U, 0x80U, 0x80U}, 2U);
        requireFastRetreat({0xF0U, 0x80U, 0x80U, 0x80U}, 3U);
        requireFastRetreat({0xF5U, 0x80U, 0x80U, 0x80U}, 3U);
        requireFastRetreat({0xEDU, 0xA0U, 0x80U}, 0U);
        requireFastRetreat({0xF4U, 0x90U, 0x80U, 0x80U}, 0U);
    }

    void testValidUtf8CutOffAtDataBoundaries() {
        WITH_CONTEXT(requireCutOffValidUtf8Sequence({0xC2U, 0xA2U}));
        WITH_CONTEXT(requireCutOffValidUtf8Sequence({0xE2U, 0x82U, 0xACU}));
        WITH_CONTEXT(requireCutOffValidUtf8Sequence({0xF0U, 0x9FU, 0x98U, 0x80U}));
    }

    void testForEachDecodedCharacterReplaceModeOverAllMalformedUtf8Categories() {
        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireForEachDecodedCharacterReplaceMode(error));
        }
    }

    void testForEachDecodedCharacterIgnoreModeOverAllMalformedUtf8Categories() {
        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireForEachDecodedCharacterIgnoreMode(error));
        }
    }

    void testForEachDecodedCharacterThrowModeOverAllMalformedUtf8Categories() {
        for (const auto error : th::allUtf8Errors) {
            WITH_CONTEXT(requireForEachDecodedCharacterThrowMode(error));
        }
    }

    void testForEachDecodedCharacterStopsWhenCallbackReturnsFalse() {
        const auto text = th::stdStringFromHex("41 C2 A2 E2 82 AC F0 9F 98 80");
        auto collected = std::u32string{};
        auto callbackCount = std::size_t{0};

        const auto finished = el::text::impl::utf8::forEachDecodedCharacter(
            std::span<const char>{text.data(), text.size()},
            EncodingErrorMode::Throw,
            [&](const Char character) -> bool {
                collected.push_back(character.toRawValue());
                callbackCount += 1U;
                return callbackCount < 2U;
            });

        REQUIRE_FALSE(finished);
        REQUIRE_EQUAL(collected, std::u32string{U"A¢"});
    }

    void testIsValid() {
        REQUIRE(el::text::impl::utf8::isValid(std::span<const char>{"", 0}));
        REQUIRE(el::text::impl::utf8::isValid(std::span<const char>{"abc", 3}));

        for (const auto error : th::allUtf8Errors) {
            const auto malformed = th::invalidUtf8(error);
            REQUIRE_FALSE(el::text::impl::utf8::isValid(std::span<const char>{malformed.data(), malformed.size()}));
        }
    }

private:
    template <std::size_t N>
    [[nodiscard]] static auto byteArray(const std::uint8_t (&values)[N]) noexcept -> std::array<char, N> {
        auto result = std::array<char, N>{};
        for (auto i = std::size_t{0}; i < N; ++i) {
            result[i] = static_cast<char>(values[i]);
        }
        return result;
    }

    [[nodiscard]] static auto byteArray(std::uint8_t value) noexcept -> std::array<char, 1> {
        return {static_cast<char>(value)};
    }

    template <std::size_t N>
    [[nodiscard]] static auto makeBlock(const std::uint8_t (&values)[N]) -> ByteBlock {
        auto result = std::vector<uint8_t>{};
        result.reserve(N);
        for (const auto value : values) {
            result.push_back(value);
        }
        return ByteBlock{result};
    }

    [[nodiscard]] static auto makeBlock(std::initializer_list<uint8_t> values) -> ByteBlock {
        return ByteBlock{std::vector<uint8_t>{values}};
    }

    [[nodiscard]] static auto makeBlock(const std::string &values) -> ByteBlock {
        return ByteBlock{std::span<const char>{values.data(), values.size()}};
    }

    template <std::size_t N>
    void requireDecodeOrThrow(
        const std::uint8_t (&values)[N], const char32_t expected, const std::size_t expectedPosition) {
        auto bytes = byteArray(values);
        auto position = ByteIndex::zero();

        const auto result = el::text::impl::utf8::decodeCharOrThrow(std::span<char>{bytes}, position);

        REQUIRE_EQUAL(result.toRawValue(), expected);
        REQUIRE_EQUAL(position.toSizeT(), expectedPosition);
    }

    template <std::size_t N>
    void requireDecodeOrThrowFails(const std::uint8_t (&values)[N], const std::size_t initialPosition) {
        auto bytes = byteArray(values);
        auto position = ByteIndex::fromSizeT(initialPosition);

        REQUIRE_THROWS(el::text::impl::utf8::decodeCharOrThrow(std::span<char>{bytes}, position));
    }

    template <std::size_t N>
    void requireDecodeOrReplace(
        const std::uint8_t (&values)[N], const char32_t expected, const std::size_t expectedPosition) {
        auto bytes = byteArray(values);
        auto position = ByteIndex::zero();

        const auto result = el::text::impl::utf8::decodeCharOrReplace(std::span<char>{bytes}, position);

        REQUIRE_EQUAL(result.toRawValue(), expected);
        REQUIRE_EQUAL(position.toSizeT(), expectedPosition);
    }

    template <std::size_t N>
    void requireDecodeOrIgnore(
        const std::uint8_t (&values)[N], const char32_t expected, const std::size_t expectedPosition) {
        auto bytes = byteArray(values);
        auto position = ByteIndex::zero();

        const auto result = el::text::impl::utf8::decodeCharOrIgnore(std::span<char>{bytes}, position);

        REQUIRE(result.has_value());
        REQUIRE_EQUAL(result->toRawValue(), expected);
        REQUIRE_EQUAL(position.toSizeT(), expectedPosition);
    }

    template <std::size_t N>
    void requireReaderDecodeOrThrow(
        const std::uint8_t (&values)[N], const char32_t expected, const std::size_t expectedPosition) {
        auto reader = ByteReader{makeBlock(values)};

        const auto result = el::text::impl::utf8::decodeCharOrThrow(reader);

        REQUIRE_EQUAL(result.toRawValue(), expected);
        REQUIRE_EQUAL(reader.position().toSizeT(), expectedPosition);
    }

    template <std::size_t N>
    void requireReaderDecodeOrReplace(
        const std::uint8_t (&values)[N], const char32_t expected, const std::size_t expectedPosition) {
        auto reader = ByteReader{makeBlock(values)};

        const auto result = el::text::impl::utf8::decodeCharOrReplace(reader);

        REQUIRE_EQUAL(result.toRawValue(), expected);
        REQUIRE_EQUAL(reader.position().toSizeT(), expectedPosition);
    }

    template <std::size_t N>
    void requireReaderDecodeOrIgnore(
        const std::uint8_t (&values)[N], const char32_t expected, const std::size_t expectedPosition) {
        auto reader = ByteReader{makeBlock(values)};

        const auto result = el::text::impl::utf8::decodeCharOrIgnore(reader);

        REQUIRE(result.has_value());
        REQUIRE_EQUAL(result->toRawValue(), expected);
        REQUIRE_EQUAL(reader.position().toSizeT(), expectedPosition);
    }

    template <std::size_t N>
    void requireFastAdvance(const std::uint8_t (&values)[N], const std::size_t expectedPosition) {
        auto bytes = byteArray(values);
        auto position = ByteIndex::zero();

        el::text::impl::utf8::fastAdvanceChar(std::span<char>{bytes}, position);

        REQUIRE_EQUAL(position.toSizeT(), expectedPosition);
    }

    template <std::size_t N>
    void requireFastRetreat(const std::uint8_t (&values)[N], const std::size_t expectedPosition) {
        auto bytes = byteArray(values);
        auto position = ByteIndex::fromSizeT(bytes.size());

        el::text::impl::utf8::fastRetreatChar(std::span<char>{bytes}, position);

        REQUIRE_EQUAL(position.toSizeT(), expectedPosition);
    }

    void requireDecodeOrThrowFailsForMalformedUtf8(const th::Utf8Error error) {
        const auto malformed = th::invalidUtf8(error);
        auto position = ByteIndex::zero();

        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void {
                REQUIRE_THROWS(el::text::impl::utf8::decodeCharOrThrow(std::span<const char>{malformed}, position));
                REQUIRE(position.isZero());
            },
            [&]() -> std::string {
                return std::format(
                    "Moved position on malformed UTF-8 input: {}", th::toConsoleSafeString(malformed, 100U));
            });
    }

    void requireDecodeOrReplaceMalformedUtf8(const th::Utf8Error error) {
        const auto malformed = th::invalidUtf8(error);
        auto position = ByteIndex::zero();

        const auto result = el::text::impl::utf8::decodeCharOrReplace(std::span<const char>{malformed}, position);
        auto fastAdvancePosition = ByteIndex::zero();
        el::text::impl::utf8::fastAdvanceChar(std::span<const char>{malformed}, fastAdvancePosition);

        REQUIRE(result.isReplacement());
        REQUIRE_EQUAL(position.toSizeT(), expectedMalformedMovement(error));
        REQUIRE_EQUAL(position, fastAdvancePosition);
    }

    void requireDecodeOrIgnoreMalformedUtf8(const th::Utf8Error error) {
        const auto malformed = th::invalidUtf8(error);
        auto position = ByteIndex::zero();

        const auto result = el::text::impl::utf8::decodeCharOrIgnore(std::span<const char>{malformed}, position);
        auto fastAdvancePosition = ByteIndex::zero();
        el::text::impl::utf8::fastAdvanceChar(std::span<const char>{malformed}, fastAdvancePosition);

        REQUIRE_FALSE(result.has_value());
        REQUIRE_EQUAL(position.toSizeT(), expectedMalformedMovement(error));
        REQUIRE_EQUAL(position, fastAdvancePosition);
    }

    void requireReaderDecodeOrThrowFailsForMalformedUtf8(const th::Utf8Error error) {
        const auto malformed = th::invalidUtf8(error);
        auto reader = ByteReader{makeBlock(malformed)};

        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void {
                REQUIRE_THROWS(el::text::impl::utf8::decodeCharOrThrow(reader));
                REQUIRE(reader.position().isZero());
            },
            [&]() -> std::string {
                return std::format(
                    "Moved reader on malformed UTF-8 input: {}", th::toConsoleSafeString(malformed, 100U));
            });
    }

    void requireReaderDecodeOrReplaceMalformedUtf8(const th::Utf8Error error) {
        const auto malformed = th::invalidUtf8(error);
        auto reader = ByteReader{makeBlock(malformed)};

        const auto result = el::text::impl::utf8::decodeCharOrReplace(reader);

        REQUIRE(result.isReplacement());
        REQUIRE_EQUAL(reader.position().toSizeT(), expectedMalformedMovement(error));
    }

    void requireReaderDecodeOrIgnoreMalformedUtf8(const th::Utf8Error error) {
        const auto malformed = th::invalidUtf8(error);
        auto reader = ByteReader{makeBlock(malformed)};

        const auto result = el::text::impl::utf8::decodeCharOrIgnore(reader);

        REQUIRE_FALSE(result.has_value());
        REQUIRE_EQUAL(reader.position().toSizeT(), expectedMalformedMovement(error));
    }

    template <std::size_t N>
    void requireCutOffValidUtf8Sequence(const std::uint8_t (&values)[N]) {
        auto bytes = byteArray(values);
        for (auto cutLength = std::size_t{1}; cutLength < N; ++cutLength) {
            auto truncated = std::string_view{bytes.data(), cutLength};

            auto decodeThrowPosition = ByteIndex::zero();
            REQUIRE_THROWS(
                el::text::impl::utf8::decodeCharOrThrow(
                    std::span<const char>{truncated.data(), truncated.size()}, decodeThrowPosition));
            REQUIRE(decodeThrowPosition.isZero());

            auto decodeReplacePosition = ByteIndex::zero();
            const auto decodeReplaceResult = el::text::impl::utf8::decodeCharOrReplace(
                std::span<const char>{truncated.data(), truncated.size()}, decodeReplacePosition);
            REQUIRE(decodeReplaceResult.isReplacement());
            REQUIRE_EQUAL(decodeReplacePosition.toSizeT(), std::size_t{1});

            auto decodeIgnorePosition = ByteIndex::zero();
            REQUIRE_FALSE(
                el::text::impl::utf8::decodeCharOrIgnore(
                    std::span<const char>{truncated.data(), truncated.size()}, decodeIgnorePosition)
                    .has_value());
            REQUIRE_EQUAL(decodeIgnorePosition.toSizeT(), std::size_t{1});

            auto fastAdvancePosition = ByteIndex::zero();
            el::text::impl::utf8::fastAdvanceChar(
                std::span<const char>{truncated.data(), truncated.size()}, fastAdvancePosition);
            REQUIRE_EQUAL(fastAdvancePosition.toSizeT(), std::size_t{1});
        }
    }

    [[nodiscard]] static auto expectedMalformedMovement(const th::Utf8Error error) noexcept -> std::size_t {
        switch (error) {
        case th::Utf8Error::SurrogateCodePoint:
            return 3U;
        case th::Utf8Error::CodePointBeyondUnicodeRange:
            return 4U;
        default:
            return 1U;
        }
    }

    void requireForEachDecodedCharacterReplaceMode(const th::Utf8Error error) {
        const auto malformed = th::invalidUtf8(error, "A", "B");
        auto collected = std::u32string{};

        const auto completed = el::text::impl::utf8::forEachDecodedCharacter(
            std::span<const char>{malformed.data(), malformed.size()},
            EncodingErrorMode::Replace,
            [&](const Char character) -> void { collected.push_back(character.toRawValue()); });

        REQUIRE(completed);
        REQUIRE_EQUAL(collected, expectedReplaceDecodedText(error));
    }

    void requireForEachDecodedCharacterIgnoreMode(const th::Utf8Error error) {
        const auto malformed = th::invalidUtf8(error, "A", "B");
        auto collected = std::u32string{};

        const auto completed = el::text::impl::utf8::forEachDecodedCharacter(
            std::span<const char>{malformed.data(), malformed.size()},
            EncodingErrorMode::Ignore,
            [&](const Char character) -> void { collected.push_back(character.toRawValue()); });

        REQUIRE(completed);
        REQUIRE_EQUAL(collected, expectedIgnoreDecodedText(error));
    }

    void requireForEachDecodedCharacterThrowMode(const th::Utf8Error error) {
        const auto malformed = th::invalidUtf8(error, "A", "B");

        REQUIRE_THROWS(
            el::text::impl::utf8::forEachDecodedCharacter(
                std::span<const char>{malformed.data(), malformed.size()},
                EncodingErrorMode::Throw,
                [&](const Char) -> void {}));
    }

    [[nodiscard]] static auto expectedReplaceDecodedText(const th::Utf8Error error) -> std::u32string {
        switch (error) {
        case th::Utf8Error::UnexpectedContinuationByte:
        case th::Utf8Error::Truncated2ByteSequence:
        case th::Utf8Error::SurrogateCodePoint:
        case th::Utf8Error::CodePointBeyondUnicodeRange:
        case th::Utf8Error::InvalidStartByte:
            return std::u32string{U"A\uFFFDB"};
        case th::Utf8Error::Overlong2ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFDB"};
        case th::Utf8Error::InvalidContinuationByteIn2ByteSequence:
            return std::u32string{U"A\uFFFD B"};
        case th::Utf8Error::Overlong3ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD\uFFFDB"};
        case th::Utf8Error::Truncated3ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFDB"};
        case th::Utf8Error::InvalidContinuationByteIn3ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD B"};
        case th::Utf8Error::Overlong4ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD\uFFFD\uFFFDB"};
        case th::Utf8Error::Truncated4ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD\uFFFDB"};
        case th::Utf8Error::InvalidContinuationByteIn4ByteSequence:
            return std::u32string{U"A\uFFFD\uFFFD\uFFFD B"};
        default:
            return {};
        }
    }

    [[nodiscard]] static auto expectedIgnoreDecodedText(const th::Utf8Error error) -> std::u32string {
        switch (error) {
        case th::Utf8Error::InvalidContinuationByteIn2ByteSequence:
        case th::Utf8Error::InvalidContinuationByteIn3ByteSequence:
        case th::Utf8Error::InvalidContinuationByteIn4ByteSequence:
            return std::u32string{U"A B"};
        default:
            return std::u32string{U"AB"};
        }
    }
};
