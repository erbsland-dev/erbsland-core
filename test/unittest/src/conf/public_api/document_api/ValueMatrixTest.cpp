// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ValueTestHelper.hpp"

#include <erbsland/conf/StdFormatForConf.hpp>

using namespace el::text::literals;

namespace {
constexpr auto valueMatrixElementCount(const std::size_t value) noexcept -> el::unit::ElementCount {
    return el::unit::ElementCount::fromSizeT(value);
}

constexpr auto valueMatrixElementIndex(const std::size_t value) noexcept -> el::unit::ElementIndex {
    return el::unit::ElementIndex::fromSizeT(value);
}
}

TESTED_TARGETS(Document Value)
class ValueMatrixTest final : public UNITTEST_SUBCLASS(ValueTestHelper) {
public:
    TESTED_TARGETS(toValueList toValueMatrix)
    void testScalarConversions() {
        WITH_CONTEXT(setupTemplate2("123"));

        auto list = value->toValueList();
        REQUIRE_EQUAL(list.size(), 1U);
        REQUIRE_EQUAL(list[0]->asInteger(), 123);

        auto matrix = value->toValueMatrix();
        REQUIRE_EQUAL(matrix.rowCount(), valueMatrixElementCount(1));
        REQUIRE_EQUAL(matrix.columnCount(), valueMatrixElementCount(1));
        REQUIRE_EQUAL(matrix.actualColumnCount(valueMatrixElementIndex(0)), valueMatrixElementCount(1));
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(0))->asInteger(), 123);

        auto docList = doc->toValueList();
        REQUIRE(docList.empty());
        auto docMatrix = doc->toValueMatrix();
        REQUIRE_EQUAL(docMatrix.rowCount(), valueMatrixElementCount(0));
        REQUIRE_EQUAL(docMatrix.columnCount(), valueMatrixElementCount(0));
    }

    TESTED_TARGETS(toValueMatrix)
    void testListConversions() {
        WITH_CONTEXT(setupTemplate1("1", "2", "3", "\"text\""));

        const auto listValue = doc->valueOrThrow(el::text::String{"main.value_list"_el});
        auto listMatrix = listValue->toValueMatrix();
        REQUIRE_EQUAL(listMatrix.rowCount(), valueMatrixElementCount(3));
        REQUIRE_EQUAL(listMatrix.columnCount(), valueMatrixElementCount(1));
        REQUIRE_EQUAL(listMatrix.actualColumnCount(valueMatrixElementIndex(0)), valueMatrixElementCount(1));
        REQUIRE_EQUAL(listMatrix.actualColumnCount(valueMatrixElementIndex(1)), valueMatrixElementCount(1));
        REQUIRE_EQUAL(listMatrix.actualColumnCount(valueMatrixElementIndex(2)), valueMatrixElementCount(1));
        REQUIRE_EQUAL(listMatrix.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(0))->asInteger(), 1);
        REQUIRE_EQUAL(listMatrix.valueOrThrow(valueMatrixElementIndex(1), valueMatrixElementIndex(0))->asInteger(), 2);
        REQUIRE_EQUAL(listMatrix.valueOrThrow(valueMatrixElementIndex(2), valueMatrixElementIndex(0))->asInteger(), 3);

        const auto matrixValue = doc->valueOrThrow(el::text::String{"main.value_matrix"_el});
        auto matrix = matrixValue->toValueMatrix();
        REQUIRE_EQUAL(matrix.rowCount(), valueMatrixElementCount(3));
        REQUIRE_EQUAL(matrix.columnCount(), valueMatrixElementCount(3));
        REQUIRE_EQUAL(matrix.actualColumnCount(valueMatrixElementIndex(0)), valueMatrixElementCount(3));
        REQUIRE_EQUAL(matrix.actualColumnCount(valueMatrixElementIndex(1)), valueMatrixElementCount(3));
        REQUIRE_EQUAL(matrix.actualColumnCount(valueMatrixElementIndex(2)), valueMatrixElementCount(3));
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(0))->asInteger(), 1);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(1))->asInteger(), 2);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(2))->asInteger(), 3);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(1), valueMatrixElementIndex(0))->asInteger(), 2);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(1), valueMatrixElementIndex(1))->asInteger(), 3);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(1), valueMatrixElementIndex(2))->asInteger(), 1);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(2), valueMatrixElementIndex(0))->asInteger(), 3);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(2), valueMatrixElementIndex(1))->asInteger(), 1);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(2), valueMatrixElementIndex(2))->asInteger(), 2);
    }

    TESTED_TARGETS(asMatrix)
    void testAsMatrixConversions() {
        WITH_CONTEXT(setupTemplate1("1", "2", "3", "\"text\""));

        const auto matrixValue = doc->valueOrThrow(el::text::String{"main.value_matrix"_el});
        auto matrix = matrixValue->asMatrix<int>();
        REQUIRE_EQUAL(matrix.rowCount(), valueMatrixElementCount(3));
        REQUIRE_EQUAL(matrix.columnCount(), valueMatrixElementCount(3));
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(0)), 1);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(1)), 2);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(2)), 3);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(1), valueMatrixElementIndex(0)), 2);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(1), valueMatrixElementIndex(1)), 3);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(1), valueMatrixElementIndex(2)), 1);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(2), valueMatrixElementIndex(0)), 3);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(2), valueMatrixElementIndex(1)), 1);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(2), valueMatrixElementIndex(2)), 2);

        const auto invalidMatrixValue = doc->valueOrThrow(el::text::String{"main.nok_value_matrix"_el});
        auto invalidMatrix = invalidMatrixValue->asMatrix<int>();
        REQUIRE_EQUAL(invalidMatrix.rowCount(), valueMatrixElementCount(0));
        REQUIRE_EQUAL(invalidMatrix.columnCount(), valueMatrixElementCount(0));
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
        REQUIRE_EQUAL(times.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(0)), makeTime(12, 0));
        REQUIRE_EQUAL(times.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(1)), makeTime(13, 0));
        REQUIRE_EQUAL(times.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(2)), makeTime(14, 0));

        const auto zonedTimes = matrixValue->asMatrixOrThrow<el::time::TimeWithZone>();
        REQUIRE(
            zonedTimes.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(0)).timeZone().isLocalTime());
        REQUIRE(zonedTimes.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(1)).timeZone().isUtc());
        REQUIRE_EQUAL(
            zonedTimes.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(2)).timeZone().staticOffset(),
            el::time::Duration{el::time::Hours{2}});
    }

    TESTED_TARGETS(getMatrix)
    void testGetMatrixConversions() {
        WITH_CONTEXT(setupTemplate1("1", "2", "3", "\"text\""));

        auto matrix = doc->getMatrix<int>("main.value_matrix"_el);
        REQUIRE_EQUAL(matrix.rowCount(), valueMatrixElementCount(3));
        REQUIRE_EQUAL(matrix.columnCount(), valueMatrixElementCount(3));
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(0)), 1);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(1)), 2);
        REQUIRE_EQUAL(matrix.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(2)), 3);

        auto scalarMatrix = doc->getMatrix<int>("main.value1"_el);
        REQUIRE_EQUAL(scalarMatrix.rowCount(), valueMatrixElementCount(1));
        REQUIRE_EQUAL(scalarMatrix.columnCount(), valueMatrixElementCount(1));
        REQUIRE_EQUAL(scalarMatrix.valueOrThrow(valueMatrixElementIndex(0), valueMatrixElementIndex(0)), 1);

        auto invalidMatrix = doc->getMatrix<int>("main.nok_value_matrix"_el);
        REQUIRE_EQUAL(invalidMatrix.rowCount(), valueMatrixElementCount(0));
        REQUIRE_EQUAL(invalidMatrix.columnCount(), valueMatrixElementCount(0));
        try {
            (void)doc->getMatrixOrThrow<int>("main.nok_value_matrix"_el);
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::TypeMismatch);
        }
    }
};
