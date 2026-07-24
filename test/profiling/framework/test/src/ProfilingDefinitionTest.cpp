// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/profiling/AxisDefinition.hpp>
#include <erbsland/profiling/ProfilingDefinition.hpp>
#include <erbsland/unittest/UnitTest.hpp>

namespace pf = erbsland::profiling;

using namespace el::text::literals;

TESTED_TARGETS(ProfilingDefinition AxisDefinition)
class ProfilingDefinitionTest final : public el::UnitTest {
public:
    void testRegistration() {
        auto definition = pf::ProfilingDefinition{};
        auto axis = pf::AxisDefinition{"width"_el, "widths"_el, "width"_el};
        axis.addValue({.id = "u8"_el, .description = "UTF-8"_el});
        definition.addAxis(std::move(axis));
        REQUIRE(definition.findAxis("width"_el) != nullptr);
        REQUIRE(definition.findAxis("width"_el)->hasValue("u8"_el));
    }

    void testDuplicateRegistration() {
        auto definition = pf::ProfilingDefinition{};
        definition.addMetric({.id = "bytes"_el});
        REQUIRE_THROWS_AS(el::ApplicationError, definition.addMetric({.id = "bytes"_el}));
    }
};
