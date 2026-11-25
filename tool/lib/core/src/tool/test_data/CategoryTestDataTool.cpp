// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CategoryTestDataTool.hpp"

#include "impl/CategoryTestData.hpp"

#include "../unicode/UnicodeSource.hpp"

#include <tool/frame/ToolApplication.hpp>

namespace tool::test_data {

using namespace el::text::literals;

auto CategoryTestDataTool::identifier() const -> el::String {
    return "generate_category_test_data"_el;
}

auto CategoryTestDataTool::title() const -> el::String {
    return "Generate regular-expression category fixtures"_el;
}

auto CategoryTestDataTool::description() const -> el::String {
    return "Generate independent category mask expectations from the verified Unicode snapshot."_el;
}

auto CategoryTestDataTool::usedDataSources() const -> el::StringList {
    return el::StringList{unicode::UnicodeSource::sourceIdentifier()};
}

auto CategoryTestDataTool::defaultDirectory() const -> el::String {
    return "test/unittest/data/re"_el;
}

void CategoryTestDataTool::execute() {
    const auto name = "CategoryContainsTestData.txt"_el;
    validateOutputs(el::StringList{name});
    const auto &source = frame::app().source<unicode::UnicodeSource>();
    _outputs.set(name, impl::CategoryTestData::render(source.unicodeData().records(), source.version()));
    publish();
}

}
