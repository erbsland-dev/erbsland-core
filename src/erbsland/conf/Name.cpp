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
    return Name{NameType::Regular, normalize(std::move(name)), PrivateTag{}};
}

auto Name::createText(text::String text) -> Name {
    validateText(text);
    return Name{NameType::Text, std::move(text), PrivateTag{}};
}

auto Name::createIndex(std::size_t index) -> Name {
    return Name{NameType::Index, index, PrivateTag{}};
}

auto Name::createTextIndex(std::size_t index) -> Name {
    return Name{NameType::TextIndex, index, PrivateTag{}};
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
        PrivateTag{}};
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

auto Name::normalize(const text::String &inputText) -> text::String {
    if (inputText.isEmpty()) {
        throw ConfError{ConfErrorCategory::Syntax, "Regular names must not be empty."_el};
    }
    if (inputText.length().toSizeT() > impl::limits::maxNameLength) {
        // As regular names must contain only 7-bit characters, this size check is enough.
        // It may give a confusing error message if an API user uses multibyte characters.
        throw ConfError{ConfErrorCategory::LimitExceeded, "The given name is too long."_el};
    }
    // transform the text as efficient as possible, ideally, this is a zero copy operation.
    auto result = inputText.transformed(text::Char::toIdentifierNormalized);
    // check the syntax, for later checks, remove any meta `@`.
    auto resultNamePart = result;
    if (result.startsWith("@"_el)) {
        resultNamePart = result.slice({unit::CpIndex::one(), unit::CpLength::infinite()});
        if (resultNamePart.isEmpty()) {
            throw ConfError{ConfErrorCategory::Syntax, "A meta-name requires at least one letter."_el};
        }
    }
    // using a forEach with index is the simplest and fastest scan for syntax and used characters.
    text::Char lastChar;
    resultNamePart.forEach([&](const text::Char character, const unit::CpIndex index) -> util::LoopStatus {
        if (character != U'_' && !character.isAsciiAlphanumeric()) {
            throw ConfError{
                ConfErrorCategory::Syntax, text::StringFormat{"Invalid character at position {}"_el}.build(index)};
        }
        if (character == U'_') {
            if (index.isZero()) {
                throw ConfError{ConfErrorCategory::Syntax, "A name must not start with space or underscore."_el};
            }
            if (lastChar == U'_') {
                throw ConfError{
                    ConfErrorCategory::Syntax,
                    "Two subsequent word separators (space, underscore) are not allowed."_el};
            }
        }
        if (index.isZero() && character.isAsciiDigit()) {
            throw ConfError{ConfErrorCategory::Syntax, "A name must not start with a number."_el};
        }
        lastChar = character;
        return util::LoopStatus::Continue;
    });
    if (lastChar == U'_') {
        throw ConfError{ConfErrorCategory::Syntax, "A name must not end with a space or underscore."_el};
    }
    return result;
}

void Name::validateText(const text::String &inputText) {
    if (inputText.isEmpty()) {
        throw ConfError{ConfErrorCategory::Syntax, "Text-names must not be empty."_el};
    }
    if (inputText.length().toSizeT() > impl::limits::maxLineLength) {
        throw ConfError{ConfErrorCategory::LimitExceeded, "The given text-name exceeds the size limit."_el};
    }
    auto reader = text::StringCharReader{inputText};
    while (!reader.isAtEnd()) {
        if (reader.read() != impl::CharClass::ValidLang) {
            throw ConfError{
                ConfErrorCategory::Syntax,
                "The text-name contains a character that is not allowed in a configuration document."_el};
        }
    }
}

auto Name::meta(Meta metaName) -> const Name & {
    if (static_cast<std::size_t>(metaName) >= static_cast<std::size_t>(Meta::_count)) {
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
        Name(NameType::Regular, "@version"_el, PrivateTag{}),
        Name(NameType::Regular, "@signature"_el, PrivateTag{}),
        Name(NameType::Regular, "@include"_el, PrivateTag{}),
        Name(NameType::Regular, "@features"_el, PrivateTag{}),
    };
    return metaNames;
}

auto Name::emptyInstance() noexcept -> const Name & {
    static const auto empty = Name{};
    return empty;
}

auto Name::vrName(VR vrName) -> const Name & {
    if (static_cast<std::size_t>(vrName) >= static_cast<std::size_t>(VR::_count)) {
        throw err::LogicError{"Unknown VR name."};
    }
    return allVrNames()[static_cast<std::size_t>(vrName)];
}

auto Name::allVrNames() -> const VrNameArray & {
    // the order of these names must match the enum `VR`.
    const static auto vrNames = VrNameArray{
        Name(NameType::Regular, "vr_any"_el, PrivateTag{}),
        Name(NameType::Regular, "vr_template"_el, PrivateTag{}),
        Name(NameType::Regular, "vr_name"_el, PrivateTag{}),
        Name(NameType::Regular, "vr_entry"_el, PrivateTag{}),
        Name(NameType::Regular, "vr_key"_el, PrivateTag{}),
        Name(NameType::Regular, "vr_dependency"_el, PrivateTag{}),
        Name(NameType::Regular, "use_template"_el, PrivateTag{}),
        Name(NameType::Regular, "type"_el, PrivateTag{}),
        Name(NameType::Regular, "case_sensitive"_el, PrivateTag{}),
        Name(NameType::Regular, "name"_el, PrivateTag{}),
        Name(NameType::Regular, "key"_el, PrivateTag{}),
        Name(NameType::Regular, "mode"_el, PrivateTag{}),
        Name(NameType::Regular, "source"_el, PrivateTag{}),
        Name(NameType::Regular, "target"_el, PrivateTag{}),
        Name(NameType::Regular, "error"_el, PrivateTag{}),
    };
    return vrNames;
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
