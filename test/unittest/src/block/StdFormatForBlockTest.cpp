// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/block/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <string>

TESTED_TARGETS(StdFormatForBlock)
class StdFormatForBlockTest final : public el::UnitTest {
public:
    void testBlockValues() {
        const auto positionText = std::format("{}", el::block::Position{2, -3});
        const auto sizeText = std::format("{}", el::block::Size{8, 5});
        const auto rectangle = el::block::Rectangle{el::block::Position{2, 3}, el::block::Size{8, 5}};
        const auto rectangleText = std::format("{}", rectangle);
        REQUIRE_EQUAL(positionText, std::string{"2,-3"});
        REQUIRE_EQUAL(sizeText, std::string{"8x5"});
        REQUIRE_EQUAL(rectangleText, std::string{"2,3:8x5"});
    }
};
