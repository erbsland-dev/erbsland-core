// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/re/CharAndPosition.hpp>
#include <erbsland/re/Input.hpp>
#include <erbsland/re/InputBase.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/text/U8EncodingError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <vector>

using el::re::CharAndPosition;
using el::re::InputBase;
using el::text::Char;
using namespace el::text::literals;

TESTED_TARGETS(InputBase CharAndPosition)
TAGS(Api Unicode)
class InputBaseTest final : public el::UnitTest {
private:
    class TestInput final : public InputBase {
    public:
        explicit TestInput(std::vector<Char> characters) : _characters{std::move(characters)} {}

        [[nodiscard]] auto read() -> CharAndPosition override {
            const auto result = peek();
            if (!result.character.isEndOfData()) {
                _position += 1U;
            }
            return result;
        }

        [[nodiscard]] auto peek() -> CharAndPosition override {
            if (_position >= _characters.size()) {
                return {Char::endOfData(), _position};
            }
            return {_characters[_position], _position};
        }

        void skip(const el::unit::CpLength characterCount) override {
            _position = std::min(_characters.size(), _position + characterCount.toSizeT());
        }

    private:
        std::vector<Char> _characters;
        std::size_t _position = 0;
    };

    class ThrowingInput final : public el::re::Input {
    public:
        enum class Operation : std::uint8_t {
            Read,
            Peek,
            Skip,
            CreateMatch,
        };

        explicit ThrowingInput(const Operation operation) : _operation{operation} {}

        [[nodiscard]] auto read() -> CharAndPosition override {
            if (_operation == Operation::Read) {
                throwEncodingError();
            }
            if (_position == 0U) {
                ++_position;
                return {_operation == Operation::CreateMatch ? Char{U'a'} : Char{U'\r'}, 0U};
            }
            return {Char::endOfData(), _position};
        }
        [[nodiscard]] auto peek() -> CharAndPosition override {
            if (_operation == Operation::Peek) {
                throwEncodingError();
            }
            return {_operation == Operation::Skip ? Char{U'\n'} : Char::endOfData(), _position};
        }
        void skip(el::unit::CpLength) override {
            if (_operation == Operation::Skip) {
                throwEncodingError();
            }
        }
        [[nodiscard]] auto createMatch(el::re::CaptureGroupList) -> el::re::MatchPtr override {
            if (_operation == Operation::CreateMatch) {
                throwEncodingError();
            }
            return {};
        }

    private:
        [[noreturn]] static void throwEncodingError() {
            throw el::text::U8EncodingError{"Malformed custom input", el::unit::ByteIndex{7U}};
        }

    private:
        Operation _operation;
        std::size_t _position{0U};
    };

    void requireCustomInputErrorPropagation(const ThrowingInput::Operation operation, const el::re::Flags flags = {}) {
        const auto pattern = operation == ThrowingInput::Operation::CreateMatch ? "a"_el : "."_el;
        const auto regex = el::re::RegEx::compile(pattern, flags);
        const auto input = std::make_shared<ThrowingInput>(operation);
        try {
            static_cast<void>(regex->match(input));
            REQUIRE(false);
        } catch (const el::text::U8EncodingError &error) {
            REQUIRE_EQUAL(error.index(), el::unit::ByteIndex{7U});
        }
    }

public:
    void testNullCharacterAndRepeatedEndOfData() {
        TestInput input{{Char{U'A'}, Char::null()}};

        const auto first = input.read();
        REQUIRE_EQUAL(first.character, U'A');
        REQUIRE_EQUAL(first.position, 0U);

        const auto nullCharacter = input.peek();
        REQUIRE(nullCharacter.character.isNull());
        REQUIRE_FALSE(nullCharacter.character.isEndOfData());
        REQUIRE_EQUAL(nullCharacter.position, 1U);

        REQUIRE(input.read().character.isNull());
        for (int i = 0; i < 3; ++i) {
            const auto endOfData = input.read();
            REQUIRE(endOfData.character.isEndOfData());
            REQUIRE_FALSE(endOfData.character.isNull());
            REQUIRE_EQUAL(endOfData.position, 2U);
        }
    }

    void testSkipBeyondEnd() {
        TestInput input{{Char{U'A'}}};
        input.skip(el::unit::CpLength{10U});
        REQUIRE(input.peek().character.isEndOfData());
        REQUIRE(input.read().character.isEndOfData());
    }

    void testCustomInputEncodingErrorsPropagateFromAllOperations() {
        WITH_CONTEXT(requireCustomInputErrorPropagation(ThrowingInput::Operation::Read));
        WITH_CONTEXT(
            requireCustomInputErrorPropagation(ThrowingInput::Operation::Peek, el::re::Flags{el::re::Flag::CRLF}));
        WITH_CONTEXT(
            requireCustomInputErrorPropagation(ThrowingInput::Operation::Skip, el::re::Flags{el::re::Flag::CRLF}));
        WITH_CONTEXT(requireCustomInputErrorPropagation(ThrowingInput::Operation::CreateMatch));
    }
};
