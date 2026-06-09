// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DisplayAllAttributesApp.hpp"

auto DisplayAllAttributesApp::attributeSpecs() -> const std::array<AttributeSpec, 8> & {
    static const auto cAttributeSpecs = std::array<AttributeSpec, 8>{{
        {"Bold"_el, BlockAttributes::Bold, "1 / 22"_el, "stronger emphasis"_el, "The quick brown fox 0123"_el},
        {"Dim"_el, BlockAttributes::Dim, "2 / 22"_el, "reduced intensity"_el, "The quick brown fox 0123"_el},
        {"Italic"_el,
            BlockAttributes::Italic,
            "3 / 23"_el,
            "often unsupported in basic terminals"_el,
            "The quick brown fox 0123"_el},
        {"Underline"_el,
            BlockAttributes::Underline,
            "4 / 24"_el,
            "classic link-style emphasis"_el,
            "The quick brown fox 0123"_el},
        {"Blink"_el,
            BlockAttributes::Blink,
            "5 / 25"_el,
            "often disabled by terminal emulators"_el,
            "The quick brown fox 0123"_el},
        {"Reverse"_el,
            BlockAttributes::Reverse,
            "7 / 27"_el,
            "swaps foreground and background"_el,
            "The quick brown fox 0123"_el},
        {"Hidden"_el,
            BlockAttributes::Hidden,
            "8 / 28"_el,
            "text should disappear between the markers"_el,
            "hidden sample"_el},
        {"Strikethrough"_el,
            BlockAttributes::Strikethrough,
            "9 / 29"_el,
            "ANSI crossed-out text"_el,
            "The quick brown fox 0123"_el},
    }};
    return cAttributeSpecs;
}

auto DisplayAllAttributesApp::beforeMain() -> int {
    terminal()->input().setMode(Input::Mode::ReadLine);
    printHeader();
    printAttributeTable();
    printCombinations();
    printPausePrompt();
    terminal()->flush();
    if (terminal()->isInteractive()) {
        static_cast<void>(terminal()->input().readLine());
    }
    return -1;
}

void DisplayAllAttributesApp::printHeader() noexcept {
    terminal()->printLine(fg::BrightWhite, BlockAttributes{BlockAttributes::Bold}, "Display All Attributes"_el);
    terminal()->printLine(
        fg::BrightBlack,
        "Visual differences depend on your terminal emulator and theme. "_el,
        "If a sample looks unchanged, that attribute is likely ignored."_el);
    terminal()->printLine(
        fg::BrightBlack,
        "The backend support column shows what the library backend can emit, not what your terminal actually renders."_el);
    terminal()->writeLineBreak();
    terminal()->printLine(
        fg::BrightWhite,
        "Attribute       Backend   ANSI      Sample                                               Note"_el);
    terminal()->printLine(
        fg::BrightBlack,
        "---------------------------------------------------------------------------------------------------"_el);
}

void DisplayAllAttributesApp::printAttributeTable() noexcept {
    const auto supported = terminal()->supportedBlockAttributes();
    for (const auto &spec : attributeSpecs()) {
        printAttributeRow(spec, supported.isEnabled(spec.flag));
    }
    terminal()->writeLineBreak();
}

void DisplayAllAttributesApp::printAttributeRow(const AttributeSpec &spec, const bool supported) noexcept {
    terminal()->print(
        fg::BrightCyan,
        padded(spec.name, 15),
        fg::Default,
        padded(supportLabel(supported), 10),
        fg::BrightBlack,
        padded(spec.sgrCodes, 10));
    if (spec.flag.value == BlockAttributes::Hidden.value) {
        terminal()->print(
            fg::Default,
            " |"_el,
            sampleAttributes(spec.flag),
            spec.sampleText,
            BlockAttributes::reset(),
            "|"_el,
            fg::BrightBlack,
            " "_el,
            spec.note);
    } else {
        terminal()->print(
            fg::BrightWhite,
            bg::BrightBlack,
            " "_el,
            sampleAttributes(spec.flag),
            spec.sampleText,
            BlockAttributes::reset(),
            bg::Default,
            " "_el,
            fg::BrightBlack,
            " "_el,
            spec.note);
    }
    terminal()->writeLineBreak();
}

void DisplayAllAttributesApp::printCombinations() noexcept {
    auto emphatic = BlockAttributes{};
    emphatic.setBold(true);
    emphatic.setUnderline(true);
    auto editorial = BlockAttributes{};
    editorial.setItalic(true);
    editorial.setStrikethrough(true);
    auto alert = BlockAttributes{};
    alert.setBold(true);
    alert.setBlink(true);
    alert.setReverse(true);

    terminal()->printLine(fg::BrightWhite, BlockAttributes{BlockAttributes::Bold}, "Common Combinations"_el);
    terminal()->printLine(
        fg::BrightBlack,
        "These examples help spot interactions between attributes, especially reset handling for bold and dim."_el);
    terminal()->printLine(
        fg::BrightCyan,
        padded("Bold + Underline"_el, 28),
        fg::BrightWhite,
        emphatic,
        "Highlighted heading sample"_el,
        BlockAttributes::reset());
    terminal()->printLine(
        fg::BrightCyan,
        padded("Italic + Strikethrough"_el, 28),
        fg::BrightWhite,
        editorial,
        "Edited-out annotation sample"_el,
        BlockAttributes::reset());
    terminal()->printLine(
        fg::BrightCyan,
        padded("Bold + Blink + Reverse"_el, 28),
        fg::BrightWhite,
        bg::Red,
        alert,
        "High-attention sample"_el,
        BlockAttributes::reset(),
        Color::reset());
    terminal()->writeLineBreak();
}

void DisplayAllAttributesApp::printPausePrompt() noexcept {
    if (!terminal()->isInteractive()) {
        return;
    }
    terminal()->printLine(fg::BrightBlack, "Press Enter to close the demo."_el);
}

auto DisplayAllAttributesApp::padded(const el::StringView text, const std::size_t width) -> el::String {
    return el::String{text}.aligned(el::CpLength::fromSizeT(width), Alignment::Left, U' ');
}

auto DisplayAllAttributesApp::supportLabel(const bool supported) -> el::StringView {
    if (supported) {
        return "yes"_el;
    }
    return "no"_el;
}

auto DisplayAllAttributesApp::sampleAttributes(const BlockAttributes::Flag flag) -> BlockAttributes {
    auto attributes = BlockAttributes{};
    if (flag.value == BlockAttributes::Bold.value) {
        attributes.setBold(true);
    } else if (flag.value == BlockAttributes::Dim.value) {
        attributes.setDim(true);
    } else if (flag.value == BlockAttributes::Italic.value) {
        attributes.setItalic(true);
    } else if (flag.value == BlockAttributes::Underline.value) {
        attributes.setUnderline(true);
    } else if (flag.value == BlockAttributes::Blink.value) {
        attributes.setBlink(true);
    } else if (flag.value == BlockAttributes::Reverse.value) {
        attributes.setReverse(true);
    } else if (flag.value == BlockAttributes::Hidden.value) {
        attributes.setHidden(true);
    } else if (flag.value == BlockAttributes::Strikethrough.value) {
        attributes.setStrikethrough(true);
    }
    return attributes;
}
