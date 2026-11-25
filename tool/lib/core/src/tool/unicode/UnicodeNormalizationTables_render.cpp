// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnicodeNormalizationTables.hpp"

#include <erbsland/render/Context.hpp>
#include <erbsland/StringFormat.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::unicode {

using namespace el::text;
using namespace el::text::literals;

auto UnicodeNormalizationTables::arrayContext(
    const el::String &name, const el::String &type, const std::vector<uint32_t> &values, const uint32_t digits)
    -> render::ValueMap {
    static const auto cHexByte = el::StringFormat{"0x{:number:hex,precision=2,lc=uppercase}U,"_el};
    static const auto cHexPage = el::StringFormat{"0x{:number:hex,precision=8,lc=uppercase}U,"_el};
    auto result = render::ValueMap{};
    result.set("name"_el, name);
    result.set("type"_el, type);
    result.set("count"_el, values.size());
    result.set("row_size"_el, digits == 2U ? 16U : 8U);
    auto entries = render::ValueList{};
    for (const auto value : values) {
        if (digits == 2U) {
            entries.append(cHexByte.build(value));
        } else {
            entries.append(cHexPage.build(value));
        }
    }
    result.set("values"_el, entries);
    return result;
}

auto UnicodeNormalizationTables::render() const -> el::String {
    auto arrays = render::ValueList{};
    arrays.append(arrayContext("unicodeNormalizationDecompositionPages"_el, "uint32_t"_el, _decompositionPages, 8U));
    arrays.append(arrayContext(
        "unicodeNormalizationDecompositionData"_el,
        "uint8_t"_el,
        {_decompositionData.begin(), _decompositionData.end()},
        2U));
    arrays.append(arrayContext("unicodeNormalizationCombiningClassPages"_el, "uint32_t"_el, _combiningPages, 8U));
    arrays.append(arrayContext(
        "unicodeNormalizationCombiningClassData"_el, "uint8_t"_el, {_combiningData.begin(), _combiningData.end()}, 2U));
    arrays.append(arrayContext("unicodeNormalizationCompositionPages"_el, "uint32_t"_el, _compositionPages, 8U));
    arrays.append(arrayContext(
        "unicodeNormalizationCompositionData"_el,
        "uint8_t"_el,
        {_compositionData.begin(), _compositionData.end()},
        2U));
    auto context = render::Context{};
    context.set("arrays"_el, arrays);
    return frame::app()
        .createRenderEnvironment(frame::TemplateLanguage::Cpp)
        ->render("unicode/normalization_data.cpp"_el, context);
}

}
