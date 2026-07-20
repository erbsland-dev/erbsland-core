// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/Matrix.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <stdexcept>

using namespace el::conf;

namespace {
constexpr auto matrixElementCount(const std::size_t value) noexcept -> el::unit::ElementCount {
    return el::unit::ElementCount::fromSizeT(value);
}

constexpr auto matrixElementIndex(const std::size_t value) noexcept -> el::unit::ElementIndex {
    return el::unit::ElementIndex::fromSizeT(value);
}
}

TESTED_TARGETS(Matrix)
class MatrixTest final : public el::UnitTest {
public:
    void testDefault() {
        const Matrix<int> matrix{};
        REQUIRE(matrix.rowCount() == matrixElementCount(0));
        REQUIRE(matrix.columnCount() == matrixElementCount(0));
    }

    void testConstruction() {
        Matrix<int> matrix{matrixElementCount(3), matrixElementCount(3)};
        matrix.setRow(matrixElementIndex(0), {1, 2, 3});
        matrix.setRow(matrixElementIndex(1), {4});
        REQUIRE(matrix.rowCount() == matrixElementCount(3));
        REQUIRE(matrix.columnCount() == matrixElementCount(3));
        REQUIRE(matrix.actualColumnCount(matrixElementIndex(0)) == matrixElementCount(3));
        REQUIRE(matrix.actualColumnCount(matrixElementIndex(1)) == matrixElementCount(1));
        REQUIRE(matrix.actualColumnCount(matrixElementIndex(2)) == matrixElementCount(0));
    }

    void testAccessDefinedAndDefault() {
        Matrix<int> matrix{matrixElementCount(3), matrixElementCount(3)};
        matrix.setRow(matrixElementIndex(0), {1, 2, 3});
        matrix.setValue(matrixElementIndex(1), matrixElementIndex(0), 4);
        REQUIRE(matrix.valueOrThrow(matrixElementIndex(0), matrixElementIndex(1)) == 2);
        REQUIRE(matrix.valueOrThrow(matrixElementIndex(1), matrixElementIndex(0)) == 4);
        REQUIRE(matrix.value(matrixElementIndex(1), matrixElementIndex(2), 0) == 0);
        REQUIRE(matrix.isDefined(matrixElementIndex(0), matrixElementIndex(2)));
        REQUIRE_FALSE(matrix.isDefined(matrixElementIndex(1), matrixElementIndex(1)));
    }

    void testBoundsChecks() {
        Matrix<int> matrix{matrixElementCount(2), matrixElementCount(2)};
        REQUIRE(matrix.actualColumnCount(matrixElementIndex(5)) == matrixElementCount(0));
        REQUIRE_FALSE(matrix.isDefined(matrixElementIndex(3), matrixElementIndex(1)));
        REQUIRE(matrix.value(matrixElementIndex(0), matrixElementIndex(5), 0) == 0);
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, matrix.valueOrThrow(matrixElementIndex(0), matrixElementIndex(5)));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, matrix.setValue(matrixElementIndex(0), matrixElementIndex(5), 3));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, matrix.setRow(matrixElementIndex(5), {1}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, matrix.setRow(matrixElementIndex(0), {1, 2, 3}));
    }

    void testMaximumSizeIsCheckedBeforeAllocation() {
        const auto excessiveRowCount = matrixElementCount(Matrix<int>::cMaximumValueCount + 1U);
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError, (void)Matrix<int>{excessiveRowCount, el::unit::ElementCount::one()});
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError, (void)Matrix<int>{matrixElementCount(10'001), matrixElementCount(10'001)});
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError,
            (void)Matrix<int>{el::unit::ElementCount::infinite(), el::unit::ElementCount::zero()});
    }
};
