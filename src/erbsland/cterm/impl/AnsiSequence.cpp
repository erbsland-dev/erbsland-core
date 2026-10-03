// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AnsiSequence.hpp"

#include "../../text/StringEditor.hpp"
#include "../../text/StringFormat.hpp"

#include <vector>

namespace erbsland::cterm::impl::ansi_sequence {

using namespace text::literals;
using text::String;
using text::StringFormat;

auto color(const int code) -> String {
    static const auto cFormat = StringFormat{"\x1b[{}m"};
    return cFormat.build(code);
}

auto color(const int foregroundCode, const int backgroundCode) -> String {
    static const auto cFormat = StringFormat{"\x1b[{};{}m"};
    return cFormat.build(foregroundCode, backgroundCode);
}

auto cursorPosition(const int row, const int column) -> String {
    static const auto cFormat = StringFormat{"\x1b[{};{}H"};
    return cFormat.build(row, column);
}

auto moveLeft(const int count) -> String {
    static const auto cFormat = StringFormat{"\x1b[{}D"};
    return cFormat.build(count);
}

auto moveRight(const int count) -> String {
    static const auto cFormat = StringFormat{"\x1b[{}C"};
    return cFormat.build(count);
}

auto moveUp(const int count) -> String {
    static const auto cFormat = StringFormat{"\x1b[{}A"};
    return cFormat.build(count);
}

auto moveDown(const int count) -> String {
    static const auto cFormat = StringFormat{"\x1b[{}B"};
    return cFormat.build(count);
}

auto attributes(const BlockAttributes previous, const BlockAttributes next) -> text::String {
    auto codes = std::vector<int>{};
    if (previous.isBold() != next.isBold() || previous.isDim() != next.isDim()) {
        if (!next.isBold() && !next.isDim()) {
            codes.push_back(22);
        } else {
            if (previous.isBold() || previous.isDim()) {
                codes.push_back(22);
            }
            if (next.isBold()) {
                codes.push_back(1);
            }
            if (next.isDim()) {
                codes.push_back(2);
            }
        }
    }
    if (previous.isItalic() != next.isItalic()) {
        if (next.isItalic()) {
            codes.push_back(3);
        } else {
            codes.push_back(23);
        }
    }
    if (previous.isUnderline() != next.isUnderline()) {
        if (next.isUnderline()) {
            codes.push_back(4);
        } else {
            codes.push_back(24);
        }
    }
    if (previous.isBlink() != next.isBlink()) {
        if (next.isBlink()) {
            codes.push_back(5);
        } else {
            codes.push_back(25);
        }
    }
    if (previous.isReverse() != next.isReverse()) {
        if (next.isReverse()) {
            codes.push_back(7);
        } else {
            codes.push_back(27);
        }
    }
    if (previous.isHidden() != next.isHidden()) {
        if (next.isHidden()) {
            codes.push_back(8);
        } else {
            codes.push_back(28);
        }
    }
    if (previous.isStrikethrough() != next.isStrikethrough()) {
        if (next.isStrikethrough()) {
            codes.push_back(9);
        } else {
            codes.push_back(29);
        }
    }
    if (codes.empty()) {
        return {};
    }
    auto sequence = text::StringEditor{"\x1b["_el};
    auto firstCode = true;
    for (const auto code : codes) {
        if (!firstCode) {
            sequence.append(U';');
        }
        sequence.append(text::String::fromInteger(code));
        firstCode = false;
    }
    sequence.append(U'm');
    return sequence;
}

}
