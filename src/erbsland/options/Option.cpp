// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Option.hpp"

#include "OptionChoices.hpp"

#include "impl/AsciiOptionText.hpp"

#include "../text/Literals.hpp"

#include <compare>
#include <memory>

namespace erbsland::options {

using namespace text::literals;

using text::Char;
using text::String;
using text::StringSide;

Option::Option(std::initializer_list<String> names) {
    setNames(names);
}

auto Option::create() -> OptionPtr {
    return std::make_shared<Option>();
}

auto Option::create(std::initializer_list<String> names) -> OptionPtr {
    return std::make_shared<Option>(names);
}

auto Option::isLongName(const String &name) noexcept -> bool {
    return name.startsWith("--"_el);
}

auto Option::isShortName(const String &name) noexcept -> bool {
    return name.startsWith("-"_el) && !isLongName(name);
}

auto Option::isOptionName(const String &name) noexcept -> bool {
    return isLongName(name) || isShortName(name);
}

auto Option::isPositionalName(const String &name) noexcept -> bool {
    return !isOptionName(name);
}

auto Option::isValidLongName(const String &name) noexcept -> bool {
    if (!isLongName(name)) {
        return false;
    }
    const auto token = name.slice({unit::ByteIndex{2U}, unit::ByteLength::infinite()});
    return impl::isAsciiNameToken(token);
}

auto Option::isValidShortName(const String &name) noexcept -> bool {
    if (!isShortName(name)) {
        return false;
    }

    const auto [character, rest] =
        name.slice({unit::ByteIndex::one(), unit::ByteLength::infinite()}).slice(StringSide::Front);
    return character.isAsciiAlphanumeric() && rest.isEmpty();
}

auto Option::isValidOptionName(const String &name) noexcept -> bool {
    return isLongName(name) ? isValidLongName(name) : isValidShortName(name);
}

auto Option::isValidPositionalName(const String &name) noexcept -> bool {
    if (!isPositionalName(name)) {
        return false;
    }
    return impl::isAsciiNameToken(name);
}

void Option::addName(String name) {
    if (isShortName(name)) {
        _names.emplace_back(std::move(name));
    } else {
        _names.emplace_back(name.transformed(Char::toAsciiLowercase));
    }
    updateImplicitTypeFromNames();
}

void Option::setNames(std::initializer_list<String> names) {
    _names.clear();
    for (const auto &name : names) {
        _names.emplace_back(isShortName(name) ? name : name.transformed(Char::toAsciiLowercase));
    }
    updateImplicitTypeFromNames();
}

auto Option::isDisabled() const noexcept -> bool {
    return _flags.isSet(OptionFlag::Disabled);
}

auto Option::hasLongName(const String &name) const -> bool {
    const auto canonicalName = name.transformed(Char::toAsciiLowercase);
    for (const auto &optionName : _names) {
        if (isLongName(optionName) && optionName == canonicalName) {
            return true;
        }
    }
    return false;
}

auto Option::hasShortName(const Char shortName) const -> bool {
    for (const auto &optionName : _names) {
        if (isValidShortName(optionName) && optionName.charAt(unit::ByteIndex::one()) == shortName) {
            return true;
        }
    }
    return false;
}

auto Option::hasOptionName() const -> bool {
    for (const auto &optionName : _names) {
        if (isOptionName(optionName)) {
            return true;
        }
    }
    return false;
}

auto Option::hasPositionalName() const -> bool {
    for (const auto &optionName : _names) {
        if (isPositionalName(optionName)) {
            return true;
        }
    }
    return false;
}

auto Option::hasPositionalName(const String &name) const -> bool {
    const auto canonicalName = name.transformed(Char::toAsciiLowercase);
    for (const auto &optionName : _names) {
        if (isPositionalName(optionName) && optionName == canonicalName) {
            return true;
        }
    }
    return false;
}

auto Option::hasValidOptionNames() const noexcept -> bool {
    for (const auto &optionName : _names) {
        if (isOptionName(optionName) && !isValidOptionName(optionName)) {
            return false;
        }
        if (isPositionalName(optionName) && !isValidPositionalName(optionName)) {
            return false;
        }
    }
    return true;
}

auto Option::hasConflictingOptionName(const Option &other) const -> bool {
    for (const auto &optionName : _names) {
        for (const auto &otherName : other.names()) {
            if (isLongName(optionName) && isLongName(otherName) && optionName == otherName) {
                return true;
            }
            if (isShortName(optionName) && isShortName(otherName) && optionName == otherName) {
                return true;
            }
            if (isPositionalName(optionName) && isPositionalName(otherName) && optionName == otherName) {
                return true;
            }
        }
    }
    return false;
}

auto Option::matchingChoiceText(const String &text) const -> std::optional<String> {
    if (_choices == nullptr) {
        return {};
    }
    for (const auto &choice : _choices->choices()) {
        if (choice->text().compare(text, Char::compareAsciiFolded) == std::strong_ordering::equal) {
            return choice->text();
        }
    }
    return {};
}

void Option::updateImplicitTypeFromNames() noexcept {
    if (_explicitType) {
        return;
    }
    _type = isPositionalArgument() ? OptionType::Text : OptionType::Flag;
}

}
