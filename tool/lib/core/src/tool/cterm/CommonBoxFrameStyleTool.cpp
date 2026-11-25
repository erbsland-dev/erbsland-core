// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CommonBoxFrameStyleTool.hpp"

#include <erbsland/render/Context.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <tool/frame/ToolApplication.hpp>

#include <map>

namespace tool::cterm {

CommonBoxFrameStyleTool::CommonBoxFrameStyleTool() :
    FrameGeneratorTool{"src/erbsland/cterm/impl/CommonBoxFrameBlockCombinationStyle_data.cpp"_el} {
}

auto CommonBoxFrameStyleTool::identifier() const -> el::String {
    return "generate_common_box_frame_style"_el;
}

auto CommonBoxFrameStyleTool::title() const -> el::String {
    return "Generate common box-frame combination data."_el;
}

auto CommonBoxFrameStyleTool::description() const -> el::String {
    return "Generate compact runtime box-frame character attributes from selected UCD characters."_el;
}

auto CommonBoxFrameStyleTool::render(const BoxFrameData &data) const -> el::String {
    auto boxIndexes = std::vector<uint32_t>(128U, 0xFFU);
    auto characters = std::vector<uint32_t>{};
    auto attributes = std::vector<uint32_t>{};
    auto specialCodePoints = std::vector<uint32_t>{};
    auto specialIndexes = std::vector<uint32_t>{};
    auto exact = std::map<uint32_t, uint32_t>{};
    for (auto index = 0U; index < data.characters().size(); ++index) {
        const auto &character = data.characters()[index];
        const auto codePoint = static_cast<uint32_t>(character.codePoint);
        const auto packed = BoxFrameData::packed(character.attributes);
        characters.push_back(codePoint);
        attributes.push_back(packed);
        exact[packed] = codePoint;
        if (codePoint >= 0x2500U && codePoint <= 0x257FU) {
            boxIndexes[codePoint - 0x2500U] = index;
        } else {
            specialCodePoints.push_back(codePoint);
            specialIndexes.push_back(index);
        }
    }
    auto exactAttributes = std::vector<uint32_t>{};
    auto exactCharacters = std::vector<uint32_t>{};
    for (const auto &[attribute, character] : exact) {
        exactAttributes.push_back(attribute);
        exactCharacters.push_back(character);
    }
    auto arrays = el::text::render::ValueList{};
    arrays.append(arrayContext("boxOffsetToCharacterIndex"_el, "uint8_t"_el, boxIndexes, 2U));
    arrays.append(arrayContext("characters"_el, "CodePointData"_el, characters, 4U));
    arrays.append(arrayContext("attributes"_el, "AttributeData"_el, attributes, 7U));
    arrays.append(arrayContext("exactAttributes"_el, "AttributeData"_el, exactAttributes, 7U));
    arrays.append(arrayContext("exactCharacters"_el, "CodePointData"_el, exactCharacters, 4U));
    arrays.append(arrayContext("positionWeights"_el, "uint8_t"_el, {4U, 4U, 4U, 4U, 5U, 5U, 6U}, 2U));
    arrays.append(arrayContext("specialCodePoints"_el, "CodePointData"_el, specialCodePoints, 4U));
    arrays.append(arrayContext("specialCharacterIndexes"_el, "uint8_t"_el, specialIndexes, 2U));
    auto context = el::text::render::Context{};
    context.set("arrays"_el, arrays);
    return frame::app()
        .createRenderEnvironment(frame::TemplateLanguage::Cpp)
        ->render("cterm/common_box_frame_style.cpp"_el, context);
}

}
