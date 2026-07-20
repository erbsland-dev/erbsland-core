// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "VrBase.hpp"

using namespace el::conf;

// Test special cases for matrix values and validation rules.
TESTED_TARGETS(Rules)
TAGS(ValidationRules)
class VrMatrixTest final : public UNITTEST_SUBCLASS(VrBase) {
public:
    void setupIntegerMatrix() {
        WITH_CONTEXT(requireRulesPassLines({
            "[app.matrix]",
            "type: \"value_matrix\"",
            "minimum: 1, 1",
            "maximum: 5, 5",
            "[app.matrix.vr_entry]",
            "type: \"integer\"",
            "minimum: 0",
            "maximum: 100",
        }));
    }

    void test1x1Matrix() {
        setupIntegerMatrix();
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "matrix: 1", // valid, 1x1 matrix
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix: -1",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix: \"text\"",
        }));
    }

    void test1x5Matrix() {
        setupIntegerMatrix();
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "matrix: 10, 20, 30, 40, 50", // valid, 1x5 matrix
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix: 10, 20, 30, 40, 50, 60", // invalid, 1x6 matrix
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix: 10, 20, 30, 40, -1",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix: 10, 20, 30, 40, \"text\"",
        }));
    }

    void test5x5Matrix() {
        setupIntegerMatrix();
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "matrix:", // valid, mixed 5x5 matrix
            "    * 10",
            "    * 20, 21",
            "    * 30, 31, 32",
            "    * 40, 41, 42, 43",
            "    * 50, 51, 52, 53, 54",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix:", // invalid, mixed 5x6 matrix
            "    * 10",
            "    * 20, 21",
            "    * 30, 31, 32",
            "    * 40, 41, 42, 43",
            "    * 50, 51, 52, 53, 54",
            "    * 60, 61, 62",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix:", // invalid, mixed 6x5 matrix
            "    * 10, 11",
            "    * 20, 21, 22",
            "    * 30, 31, 32, 33",
            "    * 40, 41, 42, 43, 44",
            "    * 50, 51, 52, 53, 54, 55",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix:", // invalid, mixed 6x5 matrix
            "    * 10, 11",
            "    * 20, 21, 22",
            "    * 30, 31, 32, 33",
            "    * 40, 41, 42, 43, 44",
            "    * 50, 51, 52, 53, 54, \"text\"",
        }));
    }

    void test3x3Matrix() {
        setupIntegerMatrix();
        WITH_CONTEXT(requirePassLines({
            "[app]",
            "matrix:", // valid, full 3x3 matrix
            "    * 10, 11, 12",
            "    * 20, 21, 22",
            "    * 30, 31, 32",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix:",
            "    * -1, 11, 12",
            "    * 20, 21, 22",
            "    * 30, 31, 32",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix:",
            "    * \"text\", 11, 12",
            "    * 20, 21, 22",
            "    * 30, 31, 32",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix:",
            "    * 10, 11, 12",
            "    * 20, -1, 22",
            "    * 30, 31, 32",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix:",
            "    * 10, 11, 12",
            "    * 20, \"text\", 22",
            "    * 30, 31, 32",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix:",
            "    * 10, 11, 12",
            "    * 20, 21, 22",
            "    * 30, 31, -1",
        }));
        WITH_CONTEXT(requireFailLines({
            "[app]",
            "matrix:",
            "    * 10, 11, 12",
            "    * 20, 21, 22",
            "    * 30, 31, \"text\"",
        }));
    }
};
