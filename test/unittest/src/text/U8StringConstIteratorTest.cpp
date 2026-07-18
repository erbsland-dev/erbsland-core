// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/u8/U8String.hpp>
#include <erbsland/text/u8/U8StringConstIterator.hpp>
#include <erbsland/text/u8/U8StringEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

using el::text::U8String;
using el::text::U8StringConstIterator;
using el::text::U8StringEditor;

TESTED_TARGETS(U8StringConstIterator)
class U8StringConstIteratorTest final : public el::UnitTest {
public:
    void testDefaultIterator() {
        const auto first = U8StringConstIterator{};
        const auto second = U8StringConstIterator{};

        REQUIRE(first == second);
        REQUIRE_FALSE(first.isValid());
        REQUIRE((*first).isNull());
        REQUIRE(first.operator->() == nullptr);
    }

    void testEmptyStringIteration() {
        const auto text = U8StringEditor{};

        REQUIRE(text.begin() == text.end());
        REQUIRE_FALSE(text.begin().isValid());
    }

    void testStringIteration() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢€😀"}};

        const auto values = collectRawValues(text);

        REQUIRE_EQUAL(values.size(), std::size_t{4});
        REQUIRE_EQUAL(values[0], U'A');
        REQUIRE_EQUAL(values[1], U'\u00A2');
        REQUIRE_EQUAL(values[2], U'\u20AC');
        REQUIRE_EQUAL(values[3], U'\U0001F600');
    }

    void testLiteralViewIteratorComparison() {
        using namespace el::text::literals;

        const auto view = U8String{"Hello"_el};

        REQUIRE(view.begin() == view.begin());
        REQUIRE(view.begin() != view.end());
    }

    void testViewIteration() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢"}};
        const auto view = U8String{text};

        const auto values = collectRawValues(view);

        REQUIRE_EQUAL(values.size(), std::size_t{2});
        REQUIRE_EQUAL(values[0], U'A');
        REQUIRE_EQUAL(values[1], U'\u00A2');
    }

    void testInvalidUtf8Iteration() {
        const auto data = invalidUtf8Data();
        const auto text = U8StringEditor{std::string_view{data}};

        const auto values = collectRawValues(text);

        REQUIRE_EQUAL(values.size(), std::size_t{3});
        REQUIRE_EQUAL(values[0], U'A');
        REQUIRE_EQUAL(values[1], U'\uFFFD');
        REQUIRE_EQUAL(values[2], U'B');
    }

    void testPreAndPostIncrement() {
        const auto text = U8StringEditor{std::u8string_view{u8"A¢"}};
        auto iterator = text.begin();
        const auto samePosition = iterator;

        REQUIRE(iterator == samePosition);
        REQUIRE(iterator != text.end());
        REQUIRE_EQUAL((*iterator).toRawValue(), U'A');
        REQUIRE_EQUAL(iterator->toRawValue(), U'A');

        ++iterator;
        REQUIRE_EQUAL((*iterator).toRawValue(), U'\u00A2');

        const auto previous = iterator++;
        REQUIRE_EQUAL((*previous).toRawValue(), U'\u00A2');
        REQUIRE(iterator == text.end());
        REQUIRE_FALSE(previous == iterator);
        REQUIRE((*iterator).isNull());
    }

    void testCopyAndMoveAssignment() {
        const auto firstText = U8StringEditor{std::string_view{"First"}};
        const auto secondText = U8StringEditor{std::string_view{"Second"}};
        auto first = firstText.begin();
        auto second = secondText.begin();

        REQUIRE(first != second);

        second = first;
        REQUIRE(second == first);
        REQUIRE_EQUAL((*second).toRawValue(), U'F');

        auto moved = U8StringConstIterator{};
        moved = std::move(second);
        REQUIRE(moved == first);
        REQUIRE_EQUAL((*moved).toRawValue(), U'F');

        auto moveConstructed = U8StringConstIterator{std::move(moved)};
        REQUIRE(moveConstructed == first);
        REQUIRE_EQUAL((*moveConstructed).toRawValue(), U'F');
    }

    void testInvalidIteratorIncrement() {
        auto iterator = U8StringConstIterator{};

        REQUIRE_FALSE(iterator.isValid());
        REQUIRE(&++iterator == &iterator);

        const auto previous = iterator++;

        REQUIRE_FALSE(iterator.isValid());
        REQUIRE_FALSE(previous.isValid());
        REQUIRE(previous == iterator);
    }

private:
    template <typename T>
    [[nodiscard]] static auto collectRawValues(const T &text) -> std::vector<char32_t> {
        auto result = std::vector<char32_t>{};
        for (const auto character : text) {
            result.push_back(character.toRawValue());
        }
        return result;
    }

    [[nodiscard]] static auto invalidUtf8Data() -> std::string {
        auto result = std::string{"A"};
        result.push_back(static_cast<char>(0xC0U));
        result.push_back('B');
        return result;
    }
};
