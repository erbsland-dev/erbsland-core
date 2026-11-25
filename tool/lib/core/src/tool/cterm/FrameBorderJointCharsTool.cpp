// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FrameBorderJointCharsTool.hpp"

#include <erbsland/render/Context.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <tool/frame/ToolApplication.hpp>

#include <array>
#include <map>

namespace tool::cterm {

FrameBorderJointCharsTool::FrameBorderJointCharsTool() :
    FrameGeneratorTool{"src/erbsland/cterm/impl/FrameBorder_data.hpp"_el} {
}

auto FrameBorderJointCharsTool::identifier() const -> el::String {
    return "generate_frame_border_joint_chars"_el;
}

auto FrameBorderJointCharsTool::title() const -> el::String {
    return "Generate frame-border joint characters."_el;
}

auto FrameBorderJointCharsTool::description() const -> el::String {
    return "Generate the table of terminal frame-border joints from selected UCD box characters."_el;
}

auto FrameBorderJointCharsTool::render(const BoxFrameData &data) const -> el::String {
    constexpr auto cStyles = std::array<uint8_t, 5>{0U, 7U, 9U, 8U, 7U};
    auto characters = std::vector<uint32_t>{};
    auto indexes = std::vector<uint32_t>{};
    auto characterIndexes = std::map<uint32_t, uint32_t>{};
    for (auto east = 0U; east < cStyles.size(); ++east) {
        for (auto south = 0U; south < cStyles.size(); ++south) {
            for (auto west = 0U; west < cStyles.size(); ++west) {
                for (auto north = 0U; north < cStyles.size(); ++north) {
                    const auto styles = std::array<std::size_t, 4>{east, south, west, north};
                    auto ideal = BoxAttributes{};
                    auto active = 0U;
                    for (auto direction = 0U; direction < 4U; ++direction) {
                        const auto stroke = cStyles[styles[direction]];
                        ideal.strokes[direction] = stroke;
                        ideal.strokes[6] = std::max(ideal.strokes[6], stroke);
                        if (stroke != 0U) {
                            ++active;
                        }
                    }
                    auto codePoint = data.best(ideal);
                    if (active == 2U && (east == 0U || east == 4U) && (south == 0U || south == 4U) &&
                        (west == 0U || west == 4U) && (north == 0U || north == 4U)) {
                        if (east == 4U && south == 4U) {
                            codePoint = data.codePointForName("BOX DRAWINGS LIGHT ARC DOWN AND RIGHT"_el);
                        } else if (west == 4U && south == 4U) {
                            codePoint = data.codePointForName("BOX DRAWINGS LIGHT ARC DOWN AND LEFT"_el);
                        } else if (east == 4U && north == 4U) {
                            codePoint = data.codePointForName("BOX DRAWINGS LIGHT ARC UP AND RIGHT"_el);
                        } else if (west == 4U && north == 4U) {
                            codePoint = data.codePointForName("BOX DRAWINGS LIGHT ARC UP AND LEFT"_el);
                        }
                    }
                    const auto scalar = static_cast<uint32_t>(codePoint);
                    if (!characterIndexes.contains(scalar)) {
                        characterIndexes.emplace(scalar, static_cast<uint32_t>(characters.size()));
                        characters.push_back(scalar);
                    }
                    indexes.push_back(characterIndexes.at(scalar));
                }
            }
        }
    }
    auto context = el::text::render::Context{};
    context.set("characters"_el, arrayContext("characters"_el, "char32_t"_el, characters, 4U));
    context.set("indexes"_el, arrayContext("indexes"_el, "uint8_t"_el, indexes, 2U));
    return frame::app()
        .createRenderEnvironment(frame::TemplateLanguage::Cpp)
        ->render("cterm/frame_border_joints.cpp"_el, context);
}

}
