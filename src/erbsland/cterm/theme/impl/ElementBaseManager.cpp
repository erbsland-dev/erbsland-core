// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ElementBaseManager.hpp"

#include <stdexcept>

namespace erbsland::cterm::theme::impl {

void ElementBaseManager::registerElement(const Element element, const Element base) {
    if (!element.isValid()) {
        throw std::invalid_argument{"Theme element must be valid."};
    }
    if (!base.isValid()) {
        throw std::invalid_argument{"Theme element base must be valid."};
    }
    if (builtInBaseElementFor(element).isValid() || element == Element::Base) {
        throw std::logic_error{"Built-in theme elements cannot be registered."};
    }
    if (const auto it = _baseElements.find(element); it != _baseElements.end()) {
        if (it->second == base) {
            return;
        }
        throw std::logic_error{"Theme element is already registered with a different base."};
    }
    _baseElements.emplace(element, base);
}

auto ElementBaseManager::baseElementFor(const Element element) const -> Element {
    if (const auto builtInBase = builtInBaseElementFor(element); builtInBase.isValid()) {
        return builtInBase;
    }
    if (const auto it = _baseElements.find(element); it != _baseElements.end()) {
        return it->second;
    }
    return Element::None;
}

auto ElementBaseManager::builtInBaseElementFor(const Element element) noexcept -> Element {
    switch (element.value()) {
    case 2:
    case 3:
    case 4:
        return Element::Base;
    case 5:
    case 6:
    case 7:
    case 10:
    case 11:
    case 12:
    case 13:
    case 16:
        return Element::Surface;
    case 8:
    case 9:
        return Element::StatusLine;
    case 14:
    case 15:
    case 17:
        return Element::Layout;
    case 18:
    case 19:
        return Element::Page;
    case 20:
        return Element::Surface;
    case 21:
    case 22:
        return Element::StaticText;
    default:
        return Element::None;
    }
}

}
