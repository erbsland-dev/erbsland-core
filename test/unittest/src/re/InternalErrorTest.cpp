// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TestHelper.hpp"

#include <erbsland/re/impl/error/InternalError.hpp>

using namespace el::text::literals;

TESTED_TARGETS(InternalError)
class InternalErrorTest final : public UNITTEST_SUBCLASS(re_test::TestHelper) {
public:
    void testRequireMessagesAreEvaluatedOnlyOnFailure() {
        auto evaluations = std::size_t{};
        const auto message = [&]() -> el::text::String {
            ++evaluations;
            return "Test failure"_el;
        };
        const auto failSafety = [&]() -> void { ERBSLAND_CORE_RE_REQUIRE_SAFETY(false, message()); };
        const auto failDebug = [&]() -> void { ERBSLAND_CORE_RE_REQUIRE_DEBUG(false, message()); };

        ERBSLAND_CORE_RE_REQUIRE_SAFETY(true, message());
        ERBSLAND_CORE_RE_REQUIRE_DEBUG(true, message());
        REQUIRE_EQUAL(evaluations, std::size_t{});

        REQUIRE_THROWS_AS(el::re::RegExError, failSafety());
        REQUIRE_EQUAL(evaluations, std::size_t{1U});
        REQUIRE_THROWS_AS(el::re::RegExError, failDebug());
        REQUIRE_EQUAL(evaluations, std::size_t{2U});
    }
};
