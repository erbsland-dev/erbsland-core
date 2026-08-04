// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/conf/Matrix.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <stdexcept>

using namespace el::conf;

TESTED_TARGETS(Matrix)
class MatrixTest final : public el::UnitTest {
public:
    void testDefault() {
        const Matrix<int> matrix{};
        REQUIRE_EQUAL(matrix.rowCount(), matrixItemCount(0));
        REQUIRE_EQUAL(matrix.columnCount(), matrixItemCount(0));
    }

    void testConstruction() {
        Matrix<int> matrix{matrixItemCount(3), matrixItemCount(3)};
        matrix.setRow(matrixItemIndex(0), {1, 2, 3});
        matrix.setRow(matrixItemIndex(1), {4});
        REQUIRE_EQUAL(matrix.rowCount(), matrixItemCount(3));
        REQUIRE_EQUAL(matrix.columnCount(), matrixItemCount(3));
        REQUIRE_EQUAL(matrix.actualColumnCount(matrixItemIndex(0)), matrixItemCount(3));
        REQUIRE_EQUAL(matrix.actualColumnCount(matrixItemIndex(1)), matrixItemCount(1));
        REQUIRE_EQUAL(matrix.actualColumnCount(matrixItemIndex(2)), matrixItemCount(0));
    }

    void testAccessDefinedAndDefault() {
        Matrix<int> matrix{matrixItemCount(3), matrixItemCount(3)};
        matrix.setRow(matrixItemIndex(0), {1, 2, 3});
        matrix.setValue(matrixItemIndex(1), matrixItemIndex(0), 4);
        const auto firstValue = matrix.valueOrThrow(matrixItemIndex(0), matrixItemIndex(1));
        const auto secondValue = matrix.valueOrThrow(matrixItemIndex(1), matrixItemIndex(0));
        const auto defaultValue = matrix.value(matrixItemIndex(1), matrixItemIndex(2), 0);
        REQUIRE_EQUAL(firstValue, 2);
        REQUIRE_EQUAL(secondValue, 4);
        REQUIRE_EQUAL(defaultValue, 0);
        REQUIRE(matrix.isDefined(matrixItemIndex(0), matrixItemIndex(2)));
        REQUIRE_FALSE(matrix.isDefined(matrixItemIndex(1), matrixItemIndex(1)));
    }

    void testBoundsChecks() {
        Matrix<int> matrix{matrixItemCount(2), matrixItemCount(2)};
        REQUIRE_EQUAL(matrix.actualColumnCount(matrixItemIndex(5)), matrixItemCount(0));
        REQUIRE_FALSE(matrix.isDefined(matrixItemIndex(3), matrixItemIndex(1)));
        const auto defaultValue = matrix.value(matrixItemIndex(0), matrixItemIndex(5), 0);
        REQUIRE_EQUAL(defaultValue, 0);
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, matrix.valueOrThrow(matrixItemIndex(0), matrixItemIndex(5)));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, matrix.setValue(matrixItemIndex(0), matrixItemIndex(5), 3));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, matrix.setRow(matrixItemIndex(5), {1}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, matrix.setRow(matrixItemIndex(0), {1, 2, 3}));
    }

    void testMaximumSizeIsCheckedBeforeAllocation() {
        const auto excessiveRowCount = matrixItemCount(Matrix<int>::cMaximumValueCount + 1U);
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, (void)Matrix<int>{excessiveRowCount, el::unit::ItemCount::one()});
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError, (void)Matrix<int>{matrixItemCount(10'001), matrixItemCount(10'001)});
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError, (void)Matrix<int>{el::unit::ItemCount::infinite(), el::unit::ItemCount::zero()});
    }

private:
    static constexpr auto matrixItemCount(const std::size_t value) noexcept -> el::unit::ItemCount {
        return el::unit::ItemCount::fromSizeT(value);
    }

    static constexpr auto matrixItemIndex(const std::size_t value) noexcept -> el::unit::ItemIndex {
        return el::unit::ItemIndex::fromSizeT(value);
    }
};
