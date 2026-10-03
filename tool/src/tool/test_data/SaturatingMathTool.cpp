// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SaturatingMathTool.hpp"

#include "impl/SaturatingMathData.hpp"

#include <erbsland/StringJoins.hpp>

#include <array>

namespace tool::test_data {

using namespace el::text::literals;

auto SaturatingMathTool::identifier() const -> el::String {
    return "generate_saturating_math_data"_el;
}

auto SaturatingMathTool::title() const -> el::String {
    return "Generate saturating math fixtures"_el;
}

auto SaturatingMathTool::description() const -> el::String {
    return "Generate exact integer arithmetic expectations for all signed and unsigned widths."_el;
}

auto SaturatingMathTool::defaultDirectory() const -> el::String {
    return "test/unittest/data/saturating_math"_el;
}

void SaturatingMathTool::execute() {
    using impl::SaturatingMathData;
    using Operation = SaturatingMathData::Operation;
    constexpr auto cOperations = std::array{
        Operation::Cast,
        Operation::Add,
        Operation::Subtract,
        Operation::Multiply,
        Operation::Divide,
        Operation::Modulo};
    auto names = el::StringList{};
    for (const auto operation : cOperations) {
        names.append(el::join(SaturatingMathData::name(operation), ".txt"_el));
    }
    validateOutputs(names);
    const auto data = SaturatingMathData{};
    for (const auto operation : cOperations) {
        auto schema = "first_type second_type first second expected overflow"_el;
        if (operation == Operation::Cast) {
            schema = "target_type source_type value expected overflow"_el;
        }
        const auto name = el::join(SaturatingMathData::name(operation), ".txt"_el);
        _outputs.set(name, renderRows(schema, data.rows(operation)));
    }
    publish();
}

}
