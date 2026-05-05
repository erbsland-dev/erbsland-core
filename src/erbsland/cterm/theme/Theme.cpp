// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Theme.hpp"

#include "ThemeBuilder.hpp"

#include "impl/ElementBaseManager.hpp"

#include <bit>
#include <utility>

namespace erbsland::cterm::theme {

Theme::Theme(Definitions definitions, const uint8_t registeredTagCount) :
    _definitions{std::move(definitions)}, _registeredTagCount{registeredTagCount} {
}

auto Theme::propertySheet(Selector selector) const -> PropertySheetConstPtr {
    selector = normalized(selector);
    if (const auto cacheIt = _cache.find(selector); cacheIt != _cache.end()) {
        return cacheIt->second;
    }

    auto sheet = PropertySheetConstPtr{};
    const auto definition = definitionFor(selector);
    if (selector.element() == Element::Base && selector.part() == Part::None && selector.states().empty() &&
        selector.tags().empty()) {
        const auto rootSheet = PropertySheet{};
        if (definition.has_value()) {
            sheet = std::make_shared<PropertySheet>(rootSheet, definition->get().properties);
        } else {
            sheet = std::make_shared<PropertySheet>(rootSheet);
        }
    } else {
        const auto baseSheet = propertySheet(baseSelectorFor(selector));
        if (definition.has_value()) {
            sheet = std::make_shared<PropertySheet>(*baseSheet, definition->get().properties);
        } else {
            sheet = baseSheet;
        }
    }
    _cache.emplace(selector, sheet);
    return sheet;
}

auto Theme::normalized(Selector selector) noexcept -> Selector {
    if (!selector.element().isValid()) {
        selector = selector.withElement(Element::Base);
    }
    return selector;
}

auto Theme::definitionFor(const Selector &selector) const noexcept -> OptionalDefinition {
    const auto definitionIt = _definitions.find(selector);
    if (definitionIt == _definitions.end()) {
        return std::nullopt;
    }
    return definitionIt->second;
}

auto Theme::baseSelectorFor(const Selector &selector) const noexcept -> Selector {
    if (!selector.tags().empty()) {
        if (const auto subset = bestTagSubset(selector); subset.has_value()) {
            return *subset;
        }
        return selector.withTags(Tags{});
    }
    if (!selector.states().empty()) {
        if (const auto subset = bestStateSubset(selector); subset.has_value()) {
            return *subset;
        }
        return selector.withStates(States{});
    }
    if (selector.part() != Part::None) {
        for (auto element = selector.element();;) {
            element = elementBaseManager().baseElementFor(element);
            if (!element.isValid()) {
                break;
            }
            const auto candidate = selector.withElement(element);
            if (definitionFor(candidate).has_value()) {
                return candidate;
            }
        }
        return selector.withPart(Part::None);
    }
    if (const auto baseElement = elementBaseManager().baseElementFor(selector.element()); baseElement.isValid()) {
        return selector.withElement(baseElement);
    }
    return Selector{Element::Base};
}

auto Theme::bestTagSubset(const Selector &selector) const noexcept -> std::optional<Selector> {
    const auto bits = selector.tags().bits();
    auto bestSelector = std::optional<Selector>{};
    auto bestBitCount = -1;
    auto bestOrder = uint64_t{0};
    for (auto subset = (bits - 1U) & bits;; subset = (subset - 1U) & bits) {
        const auto candidate = selector.withTags(Tags{subset});
        const auto definition = definitionFor(candidate);
        if (definition.has_value()) {
            const auto bitCount = static_cast<int>(std::popcount(subset));
            if (!bestSelector.has_value() || bitCount > bestBitCount ||
                (bitCount == bestBitCount && definition->get().order >= bestOrder)) {
                bestSelector = candidate;
                bestBitCount = bitCount;
                bestOrder = definition->get().order;
            }
        }
        if (subset == 0) {
            break;
        }
    }
    return bestSelector;
}

auto Theme::bestStateSubset(const Selector &selector) const noexcept -> std::optional<Selector> {
    const auto bits = selector.states().bits();
    auto bestSelector = std::optional<Selector>{};
    auto bestBitCount = -1;
    auto bestOrder = uint64_t{0};
    for (auto subset = static_cast<uint8_t>((bits - 1U) & bits);; subset = static_cast<uint8_t>((subset - 1U) & bits)) {
        const auto candidate = selector.withStates(States{subset});
        const auto definition = definitionFor(candidate);
        if (definition.has_value()) {
            const auto bitCount = static_cast<int>(std::popcount(subset));
            if (!bestSelector.has_value() || bitCount > bestBitCount ||
                (bitCount == bestBitCount && definition->get().order >= bestOrder)) {
                bestSelector = candidate;
                bestBitCount = bitCount;
                bestOrder = definition->get().order;
            }
        }
        if (subset == 0) {
            break;
        }
    }
    return bestSelector;
}

auto Theme::dark() noexcept -> const ThemeConstPtr & {
    static const auto theme = ThemeBuilder::dark().build();
    return theme;
}

auto Theme::light() noexcept -> const ThemeConstPtr & {
    static const auto theme = ThemeBuilder::light().build();
    return theme;
}

auto Theme::monochrome() noexcept -> const ThemeConstPtr & {
    static const auto theme = ThemeBuilder::monochrome().build();
    return theme;
}

auto Theme::zero() noexcept -> const ThemeConstPtr & {
    static const auto theme = ThemeBuilder::zero().build();
    return theme;
}

void Theme::registerElement(const Element element, const Element base) {
    elementBaseManager().registerElement(element, base);
}

auto Theme::elementBaseManager() noexcept -> impl::ElementBaseManager & {
    static auto manager = impl::ElementBaseManager{};
    return manager;
}

}
