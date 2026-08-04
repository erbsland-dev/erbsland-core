// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormat.hpp>

using namespace el::text::literals;

TESTED_TARGETS(Document Value)
class ValueMatrixTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
private:
    static constexpr auto valueMatrixItemCount(const std::size_t value) noexcept -> el::unit::ItemCount {
        return el::unit::ItemCount::fromSizeT(value);
    }

    static constexpr auto valueMatrixItemIndex(const std::size_t value) noexcept -> el::unit::ItemIndex {
        return el::unit::ItemIndex::fromSizeT(value);
    }

    template <typename tMatrix>
    [[nodiscard]] static auto matrixValueAt(const tMatrix &matrix, const std::size_t row, const std::size_t column) {
        return matrix.valueOrThrow(valueMatrixItemIndex(row), valueMatrixItemIndex(column));
    }

    template <typename tMatrix, typename tExpected>
    void requireMatrixValue(
        const tMatrix &matrix, const std::size_t row, const std::size_t column, const tExpected &expected) {
        const auto actual = matrixValueAt(matrix, row, column);
        REQUIRE_EQUAL(actual, expected);
    }

    template <typename tMatrix>
    void requireIntegerMatrixValue(
        const tMatrix &matrix, const std::size_t row, const std::size_t column, const int expected) {
        const auto actual = matrixValueAt(matrix, row, column)->asInteger();
        REQUIRE_EQUAL(actual, expected);
    }

public:
    TESTED_TARGETS(toValueList toValueMatrix)
    void testScalarConversions() {
        WITH_CONTEXT(setupTemplate2("123"));

        auto list = value->toValueList();
        REQUIRE_EQUAL(list.size(), 1U);
        const auto listValue = list[0]->asInteger();
        REQUIRE_EQUAL(listValue, 123);

        auto matrix = value->toValueMatrix();
        REQUIRE_EQUAL(matrix.rowCount(), valueMatrixItemCount(1));
        REQUIRE_EQUAL(matrix.columnCount(), valueMatrixItemCount(1));
        REQUIRE_EQUAL(matrix.actualColumnCount(valueMatrixItemIndex(0)), valueMatrixItemCount(1));
        requireIntegerMatrixValue(matrix, 0, 0, 123);

        auto docList = doc->toValueList();
        REQUIRE(docList.empty());
        auto docMatrix = doc->toValueMatrix();
        REQUIRE_EQUAL(docMatrix.rowCount(), valueMatrixItemCount(0));
        REQUIRE_EQUAL(docMatrix.columnCount(), valueMatrixItemCount(0));
    }

    TESTED_TARGETS(toValueMatrix)
    void testListConversions() {
        WITH_CONTEXT(setupTemplate1("1", "2", "3", "\"text\""));

        const auto listValue = doc->valueOrThrow(el::text::String{"main.value_list"_el});
        auto listMatrix = listValue->toValueMatrix();
        REQUIRE_EQUAL(listMatrix.rowCount(), valueMatrixItemCount(3));
        REQUIRE_EQUAL(listMatrix.columnCount(), valueMatrixItemCount(1));
        REQUIRE_EQUAL(listMatrix.actualColumnCount(valueMatrixItemIndex(0)), valueMatrixItemCount(1));
        REQUIRE_EQUAL(listMatrix.actualColumnCount(valueMatrixItemIndex(1)), valueMatrixItemCount(1));
        REQUIRE_EQUAL(listMatrix.actualColumnCount(valueMatrixItemIndex(2)), valueMatrixItemCount(1));
        requireIntegerMatrixValue(listMatrix, 0, 0, 1);
        requireIntegerMatrixValue(listMatrix, 1, 0, 2);
        requireIntegerMatrixValue(listMatrix, 2, 0, 3);

        const auto matrixValue = doc->valueOrThrow(el::text::String{"main.value_matrix"_el});
        auto matrix = matrixValue->toValueMatrix();
        REQUIRE_EQUAL(matrix.rowCount(), valueMatrixItemCount(3));
        REQUIRE_EQUAL(matrix.columnCount(), valueMatrixItemCount(3));
        REQUIRE_EQUAL(matrix.actualColumnCount(valueMatrixItemIndex(0)), valueMatrixItemCount(3));
        REQUIRE_EQUAL(matrix.actualColumnCount(valueMatrixItemIndex(1)), valueMatrixItemCount(3));
        REQUIRE_EQUAL(matrix.actualColumnCount(valueMatrixItemIndex(2)), valueMatrixItemCount(3));
        requireIntegerMatrixValue(matrix, 0, 0, 1);
        requireIntegerMatrixValue(matrix, 0, 1, 2);
        requireIntegerMatrixValue(matrix, 0, 2, 3);
        requireIntegerMatrixValue(matrix, 1, 0, 2);
        requireIntegerMatrixValue(matrix, 1, 1, 3);
        requireIntegerMatrixValue(matrix, 1, 2, 1);
        requireIntegerMatrixValue(matrix, 2, 0, 3);
        requireIntegerMatrixValue(matrix, 2, 1, 1);
        requireIntegerMatrixValue(matrix, 2, 2, 2);
    }

    TESTED_TARGETS(asMatrix)
    void testAsMatrixConversions() {
        WITH_CONTEXT(setupTemplate1("1", "2", "3", "\"text\""));

        const auto matrixValue = doc->valueOrThrow(el::text::String{"main.value_matrix"_el});
        auto matrix = matrixValue->asMatrix<int>();
        REQUIRE_EQUAL(matrix.rowCount(), valueMatrixItemCount(3));
        REQUIRE_EQUAL(matrix.columnCount(), valueMatrixItemCount(3));
        requireMatrixValue(matrix, 0, 0, 1);
        requireMatrixValue(matrix, 0, 1, 2);
        requireMatrixValue(matrix, 0, 2, 3);
        requireMatrixValue(matrix, 1, 0, 2);
        requireMatrixValue(matrix, 1, 1, 3);
        requireMatrixValue(matrix, 1, 2, 1);
        requireMatrixValue(matrix, 2, 0, 3);
        requireMatrixValue(matrix, 2, 1, 1);
        requireMatrixValue(matrix, 2, 2, 2);

        const auto invalidMatrixValue = doc->valueOrThrow(el::text::String{"main.nok_value_matrix"_el});
        auto invalidMatrix = invalidMatrixValue->asMatrix<int>();
        REQUIRE_EQUAL(invalidMatrix.rowCount(), valueMatrixItemCount(0));
        REQUIRE_EQUAL(invalidMatrix.columnCount(), valueMatrixItemCount(0));
        try {
            (void)invalidMatrixValue->asMatrixOrThrow<int>();
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::TypeMismatch);
        }
    }

    TESTED_TARGETS(asMatrix asTime asTimeWithZone)
    void testTimeMatrixConversions() {
        WITH_CONTEXT(setupTemplate1("12:00", "13:00z", "14:00+02", "false"));

        const auto matrixValue = doc->valueOrThrow(el::text::String{"main.value_matrix"_el});
        const auto times = matrixValue->asMatrixOrThrow<el::time::Time>();
        requireMatrixValue(times, 0, 0, makeTime(12, 0));
        requireMatrixValue(times, 0, 1, makeTime(13, 0));
        requireMatrixValue(times, 0, 2, makeTime(14, 0));

        const auto zonedTimes = matrixValue->asMatrixOrThrow<el::time::TimeWithZone>();
        const auto localTime = matrixValueAt(zonedTimes, 0, 0);
        REQUIRE(localTime.timeZone().isLocalTime());
        const auto utcTime = matrixValueAt(zonedTimes, 0, 1);
        REQUIRE(utcTime.timeZone().isUtc());
        const auto offsetTime = matrixValueAt(zonedTimes, 0, 2);
        const auto offset = offsetTime.timeZone().staticOffset();
        REQUIRE_EQUAL(offset, el::time::Duration{el::time::Hours{2}});
    }

    TESTED_TARGETS(getMatrix)
    void testGetMatrixConversions() {
        WITH_CONTEXT(setupTemplate1("1", "2", "3", "\"text\""));

        auto matrix = doc->getMatrix<int>("main.value_matrix"_el);
        REQUIRE_EQUAL(matrix.rowCount(), valueMatrixItemCount(3));
        REQUIRE_EQUAL(matrix.columnCount(), valueMatrixItemCount(3));
        requireMatrixValue(matrix, 0, 0, 1);
        requireMatrixValue(matrix, 0, 1, 2);
        requireMatrixValue(matrix, 0, 2, 3);

        auto scalarMatrix = doc->getMatrix<int>("main.value1"_el);
        REQUIRE_EQUAL(scalarMatrix.rowCount(), valueMatrixItemCount(1));
        REQUIRE_EQUAL(scalarMatrix.columnCount(), valueMatrixItemCount(1));
        requireMatrixValue(scalarMatrix, 0, 0, 1);

        auto invalidMatrix = doc->getMatrix<int>("main.nok_value_matrix"_el);
        REQUIRE_EQUAL(invalidMatrix.rowCount(), valueMatrixItemCount(0));
        REQUIRE_EQUAL(invalidMatrix.columnCount(), valueMatrixItemCount(0));
        try {
            (void)doc->getMatrixOrThrow<int>("main.nok_value_matrix"_el);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::TypeMismatch);
        }
    }
};
