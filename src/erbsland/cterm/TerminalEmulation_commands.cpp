// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TerminalEmulation.hpp"

#include "../block/Rectangle.hpp"

#include <algorithm>

namespace erbsland::cterm {

using block::Coordinate;
using block::Position;

void TerminalEmulation::executeCsi(const char32_t command) {
    auto &target = buffer();
    if (_privateMode) {
        if (command == U'h' || command == U'l') {
            for (const auto mode : _parameters) {
                setPrivateMode(mode, command == U'h');
            }
        }
        return;
    }
    const auto count = Coordinate{parameter(0)};
    const auto position = target.position();
    switch (command) {
    case U'A':
        target.moveUp(count);
        break;
    case U'B':
    case U'e':
        target.moveDown(count);
        break;
    case U'C':
    case U'a':
        target.moveRight(count);
        break;
    case U'D':
        target.moveLeft(count);
        break;
    case U'E':
        target.moveTo({Coordinate{0}, position.y() + count});
        break;
    case U'F':
        target.moveTo({Coordinate{0}, position.y() - count});
        break;
    case U'G':
    case U'`':
        target.moveTo({count - 1, position.y()});
        break;
    case U'd':
        target.moveTo({position.x(), count - 1});
        break;
    case U'H':
    case U'f':
        target.moveTo({Coordinate{parameter(1) - 1}, count - 1});
        break;
    case U'J':
        erase(true, parameter(0, 0));
        break;
    case U'K':
        erase(false, parameter(0, 0));
        break;
    case U'S':
        target.shift(
            block::Direction::North,
            Block::space().withBase(target.style()),
            std::min(parameter(0), target.size().height().toRawValue()));
        break;
    case U'T':
        target.shift(
            block::Direction::South,
            Block::space().withBase(target.style()),
            std::min(parameter(0), target.size().height().toRawValue()));
        break;
    case U'L':
        target.insertRows(
            position.y(),
            Block::space().withBase(target.style()),
            std::min(parameter(0), (target.size().height() - position.y()).toRawValue()));
        break;
    case U'M':
        target.eraseRows(
            position.y(),
            Block::space().withBase(target.style()),
            std::min(parameter(0), (target.size().height() - position.y()).toRawValue()));
        break;
    case U'X':
        target.fill(
            block::Rectangle{position, block::Size{count, Coordinate{1}}}, Block::space().withBase(target.style()));
        break;
    case U'm':
        applyStyle();
        break;
    case U's':
        _scoSaved[screenIndex()] = savedState();
        break;
    case U'u':
        restoreState(_scoSaved[screenIndex()]);
        break;
    default:
        break;
    }
}

void TerminalEmulation::erase(const bool entireScreen, const int mode) {
    auto &target = buffer();
    const auto position = target.position();
    const auto fill = Block::space().withBase(target.style());
    const auto width = target.size().width();
    if (entireScreen && mode == 3) {
        clearScrollback();
        return;
    }
    if (mode == 2) {
        if (entireScreen) {
            target.fill(fill);
        } else {
            target.fill(block::Rectangle{Coordinate{0}, position.y(), width, Coordinate{1}}, fill);
        }
        return;
    }
    if (mode == 0) {
        target.fill(block::Rectangle{position, block::Size{width - position.x(), Coordinate{1}}}, fill);
        if (entireScreen) {
            target.fill(
                block::Rectangle{Coordinate{0}, position.y() + 1, width, target.size().height() - position.y() - 1},
                fill);
        }
    } else if (mode == 1) {
        target.fill(block::Rectangle{Coordinate{0}, position.y(), position.x() + 1, Coordinate{1}}, fill);
        if (entireScreen) {
            target.fill(block::Rectangle{Coordinate{0}, Coordinate{0}, width, position.y()}, fill);
        }
    }
}

void TerminalEmulation::setPrivateMode(const int mode, const bool enabled) {
    if (mode == 7) {
        buffer().setAutoWrap(enabled);
        return;
    }
    if (mode == 1048) {
        if (enabled) {
            _decSaved[screenIndex()] = savedState();
        } else {
            restoreState(_decSaved[screenIndex()]);
        }
        return;
    }
    if (mode != 47 && mode != 1047 && mode != 1049) {
        return;
    }
    if (enabled == _alternateActive) {
        return;
    }
    if (enabled) {
        if (mode == 1049) {
            _regularSaved = savedState();
        }
        if (mode == 1049) {
            _alternate.clearScreen();
            _alternate.setStyle(_regular.style());
        }
        _alternateActive = true;
    } else {
        if (mode == 1047) {
            _alternate.clearScreen();
        }
        _alternateActive = false;
        if (mode == 1049) {
            restoreState(_regularSaved);
        }
    }
}

void TerminalEmulation::applyStyle() {
    auto &target = buffer();
    auto attributes = target.blockAttributes();
    for (auto index = std::size_t{0}; index < _parameters.size(); ++index) {
        const auto code = _parameters[index];
        if (code == 0) {
            target.setColor(Color::reset());
            attributes = BlockAttributes::reset();
        } else if (code == 1) {
            attributes.setBold(true);
        } else if (code == 2) {
            attributes.setDim(true);
        } else if (code == 3) {
            attributes.setItalic(true);
        } else if (code == 4) {
            attributes.setUnderline(true);
        } else if (code == 5) {
            attributes.setBlink(true);
        } else if (code == 7) {
            attributes.setReverse(true);
        } else if (code == 8) {
            attributes.setHidden(true);
        } else if (code == 9) {
            attributes.setStrikethrough(true);
        } else if (code == 22) {
            attributes.setBold(false);
            attributes.setDim(false);
        } else if (code == 23) {
            attributes.setItalic(false);
        } else if (code == 24) {
            attributes.setUnderline(false);
        } else if (code == 25) {
            attributes.setBlink(false);
        } else if (code == 27) {
            attributes.setReverse(false);
        } else if (code == 28) {
            attributes.setHidden(false);
        } else if (code == 29) {
            attributes.setStrikethrough(false);
        } else if (code >= 30 && code <= 37) {
            target.setForeground(Foreground::fromIndex16(code - 30));
        } else if (code >= 40 && code <= 47) {
            target.setBackground(Background::fromIndex16(code - 40));
        } else if (code >= 90 && code <= 97) {
            target.setForeground(Foreground::fromIndex16(code - 90 + 8));
        } else if (code >= 100 && code <= 107) {
            target.setBackground(Background::fromIndex16(code - 100 + 8));
        } else if (code == 39) {
            target.setForeground(Foreground::Default);
        } else if (code == 49) {
            target.setBackground(Background::Default);
        } else if (code == 38 || code == 48) {
            // Consume unsupported extended colors as one unit, never as attributes.
            if (index + 1 < _parameters.size()) {
                const auto kind = _parameters[++index];
                if (kind == 5) {
                    ++index;
                } else if (kind == 2) {
                    index += 3;
                } else {
                    break;
                }
            }
        }
    }
    target.setBlockAttributes(attributes);
}

}
