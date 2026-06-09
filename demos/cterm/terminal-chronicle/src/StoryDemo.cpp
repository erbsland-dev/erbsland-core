// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StoryDemo.hpp"

auto StoryDemo::beforeMain() -> int {
    auto &output = *terminal();
    printHeader(output);
    printTimeline(output);
    printStory(output);
    printOutro(output);
    terminal()->setDefaultColor();
    terminal()->flush();
    return -1;
}

void StoryDemo::printHeader(Terminal &terminal) noexcept {
    terminal.writeLineBreak();
    terminal.printLine(
        bg::Blue,
        fg::BrightWhite,
        " Terminal Chronicle "_el,
        Color::reset(),
        " "_el,
        fg::BrightBlack,
        "Low-level output with readable print calls."_el);
    terminal.writeLineBreak();
}

void StoryDemo::printTimeline(Terminal &terminal) noexcept {
    terminal.print(fg::BrightBlack, "Timeline: "_el);
    terminal.print(
        fg::BrightCyan,
        "teletype"_el,
        fg::BrightBlack,
        " -> "_el,
        fg::BrightGreen,
        "glass terminal"_el,
        fg::BrightBlack,
        " -> "_el,
        fg::BrightYellow,
        "VT100"_el,
        fg::BrightBlack,
        " -> "_el,
        fg::BrightMagenta,
        "modern ANSI consoles"_el);
    terminal.writeLineBreak();
    terminal.writeLineBreak();
}

void StoryDemo::printStory(Terminal &terminal) noexcept {
    terminal.printLine(
        fg::BrightCyan,
        "1968"_el,
        fg::BrightBlack,
        "  Video terminals began to replace noisy paper with phosphor screens."_el);
    terminal.printLine("      A cursor could move, lines could be rewritten, and suddenly the console"_el);
    terminal.printLine("      felt alive."_el);
    terminal.writeLineBreak();
    terminal.printLine(
        fg::BrightGreen,
        "1978"_el,
        fg::BrightBlack,
        "  DEC introduced the "_el,
        fg::BrightWhite,
        "VT100"_el,
        fg::BrightBlack,
        ", a terminal that had a language of escape "_el);
    terminal.printLine(
        "      sequences: "_el,
        fg::Yellow,
        "Move the cursor"_el,
        fg::BrightBlack,
        ", "_el,
        fg::Magenta,
        "change colors"_el,
        fg::BrightBlack,
        ", "_el,
        fg::Cyan,
        "redraw only what changed"_el,
        fg::BrightBlack,
        "."_el);
    terminal.writeLineBreak();

    terminal.printLine(
        fg::BrightYellow,
        "1980s"_el,
        fg::BrightBlack,
        " Bulletin boards, editors, and debuggers discovered that text mode did"_el);
    terminal.printLine("      not have to look dull. Progress bars, highlighted menus, and boxed"_el);
    terminal.printLine("      dialogs were all just carefully printed characters."_el);
    terminal.writeLineBreak();

    terminal.printLine(
        fg::BrightMagenta,
        "Today"_el,
        fg::BrightBlack,
        " The idea is unchanged: emit text, switch styles, keep the code compact,"_el);
    terminal.printLine("      and let the terminal shine."_el);
    terminal.printLine(
        "      This demo uses "_el,
        fg::BrightWhite,
        "printLine("_el,
        fg::BrightYellow,
        "\"text\""_el,
        fg::BrightBlack,
        ", "_el,
        fg::BrightCyan,
        "fg::..."_el,
        fg::BrightBlack,
        ", "_el,
        fg::BrightGreen,
        "bg::..."_el,
        fg::BrightWhite,
        ")"_el,
        fg::BrightBlack,
        " to tell a story"_el);
    terminal.printLine("      without building a custom renderer first."_el);
    terminal.writeLineBreak();
}

void StoryDemo::printOutro(Terminal &terminal) noexcept {
    terminal.printLine(
        bg::BrightBlack,
        fg::BrightWhite,
        " Try the other demos next: "_el,
        Color{fg::Default, bg::Default},
        " "_el,
        fg::BrightCyan,
        "retro-plasma"_el,
        fg::BrightBlack,
        ", "_el,
        fg::BrightGreen,
        "frame-weaver"_el,
        fg::BrightBlack,
        ", "_el,
        fg::BrightMagenta,
        "text-gallery"_el,
        fg::BrightBlack,
        "."_el);
    terminal.writeLineBreak();
}
