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
        auto view = BufferView{content, bgeo::BlockRectangle{bgeo::BlockPosition{1, 1}, bgeo::BlockSize{2, 2}}};
        const auto expectedSize = bgeo::BlockSize{2, 2};
        const auto expectedRect = bgeo::BlockRectangle{bgeo::BlockPosition{0, 0}, expectedSize};
        const auto expectedViewRect = bgeo::BlockRectangle{bgeo::BlockPosition{1, 1}, expectedSize};

        REQUIRE(view.content() == content);
        REQUIRE_EQUAL(view.size(), expectedSize);
        REQUIRE_EQUAL(view.rect(), expectedRect);
        REQUIRE_EQUAL(view.viewRect(), expectedViewRect);
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{0, 0}), U'E');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{1, 0}), U'F');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{0, 1}), U'H');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{1, 1}), U'I');

        const auto clone = view.clone();
        REQUIRE_EQUAL(clone->size(), expectedSize);
        REQUIRE_EQUAL(clone->get(bgeo::BlockPosition{0, 0}), U'E');
        REQUIRE_EQUAL(clone->get(bgeo::BlockPosition{1, 0}), U'F');
        REQUIRE_EQUAL(clone->get(bgeo::BlockPosition{0, 1}), U'H');
        REQUIRE_EQUAL(clone->get(bgeo::BlockPosition{1, 1}), U'I');

        auto replacement = createSharedBuffer({"Z"});
        view.setContent(replacement);
        view.setViewRect(bgeo::BlockRectangle{bgeo::BlockPosition{0, 0}, bgeo::BlockSize{1, 1}});
        const auto expectedReplacementRect = bgeo::BlockRectangle{bgeo::BlockPosition{0, 0}, bgeo::BlockSize{1, 1}};

        REQUIRE(view.content() == replacement);
        REQUIRE_EQUAL(view.viewRect(), expectedReplacementRect);
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{0, 0}), U'Z');
    }

    void testCropCharactersCanBeConfiguredAndAreIgnoredForInvalidDirections() {
        auto content = createSharedBuffer({"A"});
        auto view = BufferView{content, bgeo::BlockSize{0, 0}};

        REQUIRE_FALSE(view.showCropCharacters());
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{0, 0}), U' ');

        view.setShowCropCharacters(true);
        view.setCropCharacter(bgeo::BlockDirection::None, Block{U'.'});
        view.setCropCharacter(bgeo::BlockDirection::East, Block{U'>'});

        REQUIRE(view.showCropCharacters());
        REQUIRE_EQUAL(view.cropCharacter(bgeo::BlockDirection::None), U'.');
        REQUIRE_EQUAL(view.cropCharacter(bgeo::BlockDirection::East), U'>');

        const auto invalidDirection = bgeo::BlockDirection{static_cast<bgeo::BlockDirection::Enum>(99)};
        REQUIRE_EQUAL(view.cropCharacter(invalidDirection), Block{});

        view.setCropCharacter(invalidDirection, Block{U'!'});
        REQUIRE_EQUAL(view.cropCharacter(bgeo::BlockDirection::East), U'>');
    }

    void testViewsRenderConfiguredCropMarksForSharedAndReferencedContent() {
        auto content = createSharedBuffer({
            "ABC",
            "DEF",
            "GHI",
        });
        auto view = BufferView{content, bgeo::BlockRectangle{bgeo::BlockPosition{0, 0}, bgeo::BlockSize{2, 2}}};
        view.setShowCropCharacters(true);
        view.setCropCharacter(bgeo::BlockDirection::East, Block{U'>'});
        view.setCropCharacter(bgeo::BlockDirection::South, Block{U'v'});
        view.setCropCharacter(bgeo::BlockDirection::SouthEast, Block{U'x'});

        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{0, 0}), U'A');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{1, 0}), U'>');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{0, 1}), U'v');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{1, 1}), U'x');

        auto refView =
            BufferConstRefView{*content, bgeo::BlockRectangle{bgeo::BlockPosition{0, 0}, bgeo::BlockSize{2, 2}}};
        refView.setShowCropCharacters(true);
        refView.setCropCharacter(bgeo::BlockDirection::East, Block{U']'});
        refView.setCropCharacter(bgeo::BlockDirection::South, Block{U'_'});
        refView.setCropCharacter(bgeo::BlockDirection::SouthEast, Block{U'+'});

        REQUIRE_EQUAL(refView.get(bgeo::BlockPosition{0, 0}), U'A');
        REQUIRE_EQUAL(refView.get(bgeo::BlockPosition{1, 0}), U']');
        REQUIRE_EQUAL(refView.get(bgeo::BlockPosition{0, 1}), U'_');
        REQUIRE_EQUAL(refView.get(bgeo::BlockPosition{1, 1}), U'+');
    }

    void testCursorBufferViewsRenderTopCropMarksForScrolledViews() {
        auto content = std::make_shared<CursorBuffer>(bgeo::BlockSize{3, 3});
        fillBufferFromRows(
            *content,
            {
                "ABC",
                "DEF",
                "GHI",
            });
        auto view = BufferView{content, bgeo::BlockRectangle{bgeo::BlockPosition{0, 1}, bgeo::BlockSize{3, 2}}};
        view.setShowCropCharacters(true);
        view.setCropCharacter(bgeo::BlockDirection::North, Block{U'^'});

        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{0, 0}), U'^');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{1, 0}), U'^');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{2, 0}), U'^');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{0, 1}), U'G');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{1, 1}), U'H');
        REQUIRE_EQUAL(view.get(bgeo::BlockPosition{2, 1}), U'I');
    }
};
