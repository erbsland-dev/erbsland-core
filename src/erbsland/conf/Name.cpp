// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Name.hpp"

#include "ConfError.hpp"

#include "impl/char/CharClass.hpp"
#include "impl/char/NamedChars.hpp"
#include "impl/constants/Limits.hpp"
#include "impl/vr/RulesConstants.hpp"

#include "../text/EncodingError.hpp"
#include "../text/StringCharReader.hpp"
#include "../text/StringEditor.hpp"
#include "../text/StringFormat.hpp"

#include <cassert>

namespace erbsland::conf {

using namespace text::literals;

auto Name::createRegular(text::String name) -> Name {
    return Name{NameType::Regular, normalize(std::move(name)), impl::PrivateTag{}};
}

auto Name::createText(text::String text) -> Name {
    validateText(text);
    return Name{NameType::Text, std::move(text), impl::PrivateTag{}};
}

auto Name::createIndex(std::size_t index) -> Name {
    return Name{NameType::Index, index, impl::PrivateTag{}};
}

auto Name::createTextIndex(std::size_t index) -> Name {
    return Name{NameType::TextIndex, index, impl::PrivateTag{}};
}

auto Name::isReservedValidationRule() const noexcept -> bool {
    return std::holds_alternative<text::String>(_value) &&
        std::get<text::String>(_value).startsWith(impl::vrc::cReservedPrefix);
}

auto Name::isEscapedReservedValidationRule() const noexcept -> bool {
    return std::holds_alternative<text::String>(_value) &&
        std::get<text::String>(_value).startsWith(impl::vrc::cReservedEscape);
}

auto Name::withReservedVRPrefixRemoved() const noexcept -> Name {
    if (!isReservedValidationRule()) {
        return *this;
    }
    const auto &text = std::get<text::String>(_value);
    const auto prefixLength = impl::vrc::cReservedPrefix.length();
    return Name{
        _type,
        text.slice(unit::ByteRange{unit::ByteIndex::fromSizeT(prefixLength.toSizeT()), text.length() - prefixLength}),
        impl::PrivateTag{}};
}

auto Name::asText() const noexcept -> text::String {
    if (std::holds_alternative<text::String>(_value)) {
        return std::get<text::String>(_value);
    }
    return text::StringEditor::fromInteger(std::get<std::size_t>(_value));
}

auto Name::asIndex() const noexcept -> std::size_t {
    if (std::holds_alternative<std::size_t>(_value)) {
        return std::get<std::size_t>(_value);
    }
    return 0;
}

auto Name::pathTextSize() const noexcept -> std::size_t {
    return toPathText().length().toSizeT();
}

auto Name::toPathText() const noexcept -> text::String {
    switch (_type) {
    case NameType::Regular:
        return std::get<text::String>(_value);
    case NameType::Text:
        return text::StringFormat{"\"{:/config_test}\""_el}.build(std::get<text::String>(_value));
    case NameType::Index:
        return text::StringFormat{"[{}]"_el}.build(std::get<std::size_t>(_value));
    case NameType::TextIndex:
        return text::StringFormat{"\"\"[{}]"_el}.build(std::get<std::size_t>(_value));
    default:
        assert(false);
        return {};
    }
}

auto Name::normalize(text::String inputText) -> text::String {
    if (inputText.isEmpty()) {
        throw ConfError{ConfErrorCategory::Syntax, "Regular names must not be empty."_el};
    }
    if (inputText.length().toSizeT() > limits::maxNameLength) {
        // As regular names must contain only 7-bit characters, this size check is enough.
        // It may give a confusing error message if an API user uses multibyte characters.
        throw ConfError{ConfErrorCategory::LimitExceeded, "The given name is too long."_el};
    }
    auto reader = text::StringCharReader{inputText};
    text::StringEditor result;
    result.reserve(inputText.length());
    std::size_t characterCount = 0;
    bool lastWasWordSeparator = false;
    try {
        while (!reader.isAtEnd()) {
            const auto character = reader.readOrThrow();
            // No "if (characterCount >= limits::maxNameLength) { ... }", as the initial size check is sufficient.
            if (character == impl::nc::space || character == impl::nc::underscore) {
                if (result.isEmpty()) {
                    throw ConfError{ConfErrorCategory::Syntax, "A name must not start with space or underscore."_el};
                }
                if (lastWasWordSeparator) {
                    throw ConfError{
                        ConfErrorCategory::Syntax,
                        "Two subsequent word separators (space, underscore) are not allowed."_el};
                }
                lastWasWordSeparator = true;
            } else if (character == impl::CharClass::DecimalDigit) {
                if (result.isEmpty() ||
                    (result.characterLength() == unit::CpLength::one() &&
                        result.charAt(text::StringSide::Front) == U'@')) {
                    throw ConfError{ConfErrorCategory::Syntax, "A name must not start with a number."_el};
                }
                lastWasWordSeparator = false;
            } else if (character == impl::CharClass::Letter) {
                lastWasWordSeparator = false;
            } else if (characterCount == 0 && character == impl::nc::at) {
                lastWasWordSeparator = false; // Allow the `@` as a first character to create meta-names.
            } else {
                throw ConfError{
                    ConfErrorCategory::Syntax,
                    text::StringFormat{"Invalid character at position {}"_el}.build(characterCount)};
            }
            result.append(character.toIdentifierNormalized());
            characterCount++;
        }
    } catch (const text::EncodingError &) {
        throw ConfError{
            ConfErrorCategory::Encoding,
            "Decoding the Configuration Name Failed"_el,
            "The given name is not correctly UTF-8 encoded."_el,
            std::current_exception()};
    }
    if (result.charAt(text::StringSide::Back) == U'_') {
        throw ConfError{ConfErrorCategory::Syntax, "A name must not end with a space or underscore."_el};
    }
    if (result == "@"_el) {
        throw ConfError{ConfErrorCategory::Syntax, "A meta-name requires at least one letter."_el};
    }
    return result;
}

void Name::validateText(const text::String &inputText) {
    if (inputText.isEmpty()) {
        throw ConfError{ConfErrorCategory::Syntax, "Text-names must not be empty."_el};
    }
    if (inputText.length().toSizeT() > limits::maxLineLength) {
        throw ConfError{ConfErrorCategory::LimitExceeded, "The given text-name exceeds the size limit."_el};
    }
    auto reader = text::StringCharReader{inputText};
    try {
        while (!reader.isAtEnd()) {
            if (reader.readOrThrow() != impl::CharClass::ValidLang) {
                throw ConfError{
                    ConfErrorCategory::Syntax,
                    "The text-name contains a character that is not allowed in a configuration document."_el};
            }
        }
    } catch (const text::EncodingError &) {
        throw ConfError{
            ConfErrorCategory::Encoding,
            "Decoding the Configuration Text Name Failed"_el,
            "The given text-name is not correctly UTF-8 encoded."_el,
            std::current_exception()};
    }
}

auto Name::meta(Meta metaName) -> const Name & {
    if (static_cast<std::size_t>(metaName) > allMetaNames().size()) {
        throw err::LogicError{"Unknown meta-name."};
    }
    return allMetaNames()[static_cast<std::size_t>(metaName)];
}

auto Name::metaVersion() -> const Name & {
    return meta(Meta::ConfVersion);
}

auto Name::metaSignature() -> const Name & {
    return meta(Meta::Signature);
}

auto Name::metaInclude() -> const Name & {
    return meta(Meta::Include);
}

auto Name::metaFeatures() -> const Name & {
    return meta(Meta::Features);
}

auto Name::allMetaNames() -> const MetaNameArray & {
    // the order of these meta-names must match the enum `Meta`.
    const static auto metaNames = MetaNameArray{
        createRegular("@version"_el),
        createRegular("@signature"_el),
        createRegular("@include"_el),
        createRegular("@features"_el),
    };
    return metaNames;
}

auto Name::emptyInstance() noexcept -> const Name & {
    static const auto empty = Name{};
    return empty;
}

auto Name::indexDigitCount() const noexcept -> std::size_t {
    auto value = std::get<std::size_t>(_value);
    std::size_t digits = 1;
    for (; value >= 10; value /= 10) {
        ++digits;
    }
    return digits;
}

#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
auto internalView(const Name &object) -> impl::InternalViewPtr {
    auto result = impl::InternalView::create();
    result->setValue("type", toString(object._type));
    if (std::holds_alternative<std::size_t>(object._value)) {
        result->setValue("index", std::get<std::size_t>(object._value));
    } else {
        result->setValue("name", std::get<text::String>(object._value));
    }
    return result;
}
#endif

}
