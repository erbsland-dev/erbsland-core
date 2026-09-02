// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../support/BufferTestHelper.hpp"

#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(BufferView)
class BufferViewTest final : public UNITTEST_SUBCLASS(BufferTestHelper) {
public:
    void testCloneCopiesTheVisibleAreaAndTracksTheAssignedContent() {
        auto content = createSharedBuffer({
            "ABC",
            "DEF",
            "GHI",
        });
        auto view = BufferView{content, block::Rectangle{block::Position{1, 1}, block::Size{2, 2}}};
        const auto expectedSize = block::Size{2, 2};
        const auto expectedRect = block::Rectangle{block::Position{0, 0}, expectedSize};
        const auto expectedViewRect = block::Rectangle{block::Position{1, 1}, expectedSize};

        REQUIRE_EQUAL(view.content(), content);
        REQUIRE_EQUAL(view.size(), expectedSize);
        REQUIRE_EQUAL(view.rect(), expectedRect);
        REQUIRE_EQUAL(view.viewRect(), expectedViewRect);
        REQUIRE_EQUAL(view.get(block::Position{0, 0}), U'E');
        REQUIRE_EQUAL(view.get(block::Position{1, 0}), U'F');
        REQUIRE_EQUAL(view.get(block::Position{0, 1}), U'H');
        REQUIRE_EQUAL(view.get(block::Position{1, 1}), U'I');

        const auto clone = view.clone();
        REQUIRE_EQUAL(clone->size(), expectedSize);
        REQUIRE_EQUAL(clone->get(block::Position{0, 0}), U'E');
        REQUIRE_EQUAL(clone->get(block::Position{1, 0}), U'F');
        REQUIRE_EQUAL(clone->get(block::Position{0, 1}), U'H');
        REQUIRE_EQUAL(clone->get(block::Position{1, 1}), U'I');

        auto replacement = createSharedBuffer({"Z"});
        view.setContent(replacement);
        view.setViewRect(block::Rectangle{block::Position{0, 0}, block::Size{1, 1}});
        const auto expectedReplacementRect = block::Rectangle{block::Position{0, 0}, block::Size{1, 1}};

        REQUIRE_EQUAL(view.content(), replacement);
        REQUIRE_EQUAL(view.viewRect(), expectedReplacementRect);
        REQUIRE_EQUAL(view.get(block::Position{0, 0}), U'Z');
    }

    void testCropCharactersCanBeConfiguredAndAreIgnoredForInvalidDirections() {
        auto content = createSharedBuffer({"A"});
        auto view = BufferView{content, block::Size{0, 0}};

        REQUIRE_FALSE(view.showCropCharacters());
        REQUIRE_EQUAL(view.get(block::Position{0, 0}), U' ');

        view.setShowCropCharacters(true);
        view.setCropCharacter(block::Direction::None, Block{U'.'});
        view.setCropCharacter(block::Direction::East, Block{U'>'});

        REQUIRE(view.showCropCharacters());
        REQUIRE_EQUAL(view.cropCharacter(block::Direction::None), U'.');
        REQUIRE_EQUAL(view.cropCharacter(block::Direction::East), U'>');

        const auto invalidDirection = block::Direction{static_cast<block::Direction::Enum>(99)};
        REQUIRE_EQUAL(view.cropCharacter(invalidDirection), Block{});

        view.setCropCharacter(invalidDirection, Block{U'!'});
        REQUIRE_EQUAL(view.cropCharacter(block::Direction::East), U'>');
    }

    void testViewsRenderConfiguredCropMarksForSharedAndReferencedContent() {
        auto content = createSharedBuffer({
            "ABC",
            "DEF",
            "GHI",
        });
        auto view = BufferView{content, block::Rectangle{block::Position{0, 0}, block::Size{2, 2}}};
        view.setShowCropCharacters(true);
        view.setCropCharacter(block::Direction::East, Block{U'>'});
        view.setCropCharacter(block::Direction::South, Block{U'v'});
        view.setCropCharacter(block::Direction::SouthEast, Block{U'x'});

        REQUIRE_EQUAL(view.get(block::Position{0, 0}), U'A');
        REQUIRE_EQUAL(view.get(block::Position{1, 0}), U'>');
        REQUIRE_EQUAL(view.get(block::Position{0, 1}), U'v');
        REQUIRE_EQUAL(view.get(block::Position{1, 1}), U'x');

        auto refView = BufferConstRefView{*content, block::Rectangle{block::Position{0, 0}, block::Size{2, 2}}};
        refView.setShowCropCharacters(true);
        refView.setCropCharacter(block::Direction::East, Block{U']'});
        refView.setCropCharacter(block::Direction::South, Block{U'_'});
        refView.setCropCharacter(block::Direction::SouthEast, Block{U'+'});

        REQUIRE_EQUAL(refView.get(block::Position{0, 0}), U'A');
        REQUIRE_EQUAL(refView.get(block::Position{1, 0}), U']');
        REQUIRE_EQUAL(refView.get(block::Position{0, 1}), U'_');
        REQUIRE_EQUAL(refView.get(block::Position{1, 1}), U'+');
    }

    void testCursorBufferViewsRenderTopCropMarksForScrolledViews() {
        auto content = std::make_shared<CursorBuffer>(block::Size{3, 3});
        fillBufferFromRows(
            *content,
            {
                "ABC",
                "DEF",
                "GHI",
            });
        auto view = BufferView{content, block::Rectangle{block::Position{0, 1}, block::Size{3, 2}}};
        view.setShowCropCharacters(true);
        view.setCropCharacter(block::Direction::North, Block{U'^'});

        REQUIRE_EQUAL(view.get(block::Position{0, 0}), U'^');
        REQUIRE_EQUAL(view.get(block::Position{1, 0}), U'^');
        REQUIRE_EQUAL(view.get(block::Position{2, 0}), U'^');
        REQUIRE_EQUAL(view.get(block::Position{0, 1}), U'G');
        REQUIRE_EQUAL(view.get(block::Position{1, 1}), U'H');
        REQUIRE_EQUAL(view.get(block::Position{2, 1}), U'I');
    }
};
