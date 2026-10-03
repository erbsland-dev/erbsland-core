// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/NotFoundError.hpp>
#include <erbsland/OutOfRangeError.hpp>
#include <erbsland/OutOfRangeFailure.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/RandomFailure.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <type_traits>

using namespace el::text::literals;

TESTED_TARGETS(Exception RuntimeError LogicFailure ParameterFailure OutOfRangeError OutOfRangeFailure NotFoundError)
class ExceptionHierarchyTest final : public el::UnitTest {
public:
    void testRuntimeBranch() {
        auto applicationScope = ApplicationTestScope<>{};
        verifyRuntime(el::err::OutOfRangeError{"Numeric input exceeds its range"_el});
        verifyRuntime(el::err::NotFoundError{"Key is absent"_el});
    }

    void testFailureBranch() {
        auto applicationScope = ApplicationTestScope<>{};
        verifyFailure(el::err::LogicFailure{"Invalid state"_el});
        verifyFailure(el::err::ParameterFailure{"Invalid argument"_el, "argument"_el});
        verifyFailure(el::err::OutOfRangeFailure{"Unsupported capacity"_el});
        verifyFailure(el::random::RandomFailure{"Entropy unavailable"_el});
    }

    void testDiagnosticContent() {
        auto applicationScope = ApplicationTestScope<>{};
        const auto failure = el::err::ParameterFailure{"Invalid argument"_el, "argument"_el};
        REQUIRE_EQUAL(failure.toString(), "Invalid argument (parameter: argument)"_el);
        const auto cause = std::make_exception_ptr(el::err::RuntimeError{"Source failed"_el});
        const auto random = el::random::RandomFailure{"Entropy unavailable"_el, cause};
        REQUIRE(random.hasCause());
        REQUIRE(random.cause() == cause);
        REQUIRE_EQUAL(random.reason(), "Entropy unavailable"_el);
    }

private:
    template <typename T>
    void verifyRuntime(const T &error) {
        static_assert(std::is_base_of_v<el::err::RuntimeError, T>);
        static_assert(!std::is_base_of_v<el::err::LogicFailure, T>);
        auto caught = false;
        try {
            throw error;
        } catch (const el::err::RuntimeError &caughtError) {
            caught = true;
            REQUIRE_EQUAL(caughtError.reason(), error.reason());
        }
        REQUIRE(caught);
    }

    template <typename T>
    void verifyFailure(const T &failure) {
        static_assert(std::is_base_of_v<el::err::LogicFailure, T>);
        static_assert(!std::is_base_of_v<el::err::RuntimeError, T>);
        auto caught = false;
        try {
            throw failure;
        } catch (const el::err::RuntimeError &) {
            REQUIRE(false);
        } catch (const el::err::LogicFailure &caughtFailure) {
            caught = true;
            REQUIRE_EQUAL(caughtFailure.reason(), failure.reason());
        }
        REQUIRE(caught);
    }
};
