// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/bgeo/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <string>

TESTED_TARGETS(StdFormatForBlock)
class StdFormatForBlockTest final : public el::UnitTest {
public:
    void testBlockValues() {
        REQUIRE_EQUAL(std::format("{}", el::bgeo::BlockPosition{2, -3}), std::string{"2,-3"});
        REQUIRE_EQUAL(std::format("{}", el::bgeo::BlockSize{8, 5}), std::string{"8x5"});
        REQUIRE_EQUAL(
            std::format("{}", el::bgeo::BlockRectangle{el::bgeo::BlockPosition{2, 3}, el::bgeo::BlockSize{8, 5}}),
            std::string{"2,3:8x5"});
    }
};
