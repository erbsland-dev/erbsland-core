// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509Name.hpp"

#include "../../text/Literals.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringEditor.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

auto X509Name::values(const Asn1ObjectIdentifier &oid) const -> text::StringList {
    auto result = text::StringList{};
    for (const auto &rdn : _rdns) {
        for (const auto &attribute : rdn.attributes()) {
            if (attribute.oid() == oid && !attribute.value().isEmpty()) {
                result.append(attribute.value());
            }
        }
    }
    return result;
}

auto X509Name::commonNames() const -> text::StringList {
    return values(Asn1ObjectIdentifier::fromStringOrThrow("2.5.4.3"_el));
}

auto X509Name::organizations() const -> text::StringList {
    return values(Asn1ObjectIdentifier::fromStringOrThrow("2.5.4.10"_el));
}

auto X509Name::organizationalUnits() const -> text::StringList {
    return values(Asn1ObjectIdentifier::fromStringOrThrow("2.5.4.11"_el));
}

auto X509Name::localities() const -> text::StringList {
    return values(Asn1ObjectIdentifier::fromStringOrThrow("2.5.4.7"_el));
}

auto X509Name::states() const -> text::StringList {
    return values(Asn1ObjectIdentifier::fromStringOrThrow("2.5.4.8"_el));
}

auto X509Name::countries() const -> text::StringList {
    return values(Asn1ObjectIdentifier::fromStringOrThrow("2.5.4.6"_el));
}

auto X509Name::toString() const -> text::String {
    auto result = text::StringEditor{};
    auto firstRdn = true;
    const auto &rdns = _rdns.toRawValue();
    for (auto reverseIndex = rdns.size(); reverseIndex > 0U; --reverseIndex) {
        if (!firstRdn) {
            result.append(U',');
        }
        firstRdn = false;
        auto firstAttribute = true;
        for (const auto &attribute : rdns[reverseIndex - 1U].attributes()) {
            if (!firstAttribute) {
                result.append(U'+');
            }
            firstAttribute = false;
            result.append(attributeName(attribute.oid()));
            result.append(U'=');
            result.append(escapedValue(attribute.value()));
        }
    }
    return text::String{result};
}

auto X509Name::attributeName(const Asn1ObjectIdentifier &oid) -> text::String {
    const auto &value = oid.toString();
    if (value == "2.5.4.3"_el) {
        return "CN"_el;
    }
    if (value == "2.5.4.6"_el) {
        return "C"_el;
    }
    if (value == "2.5.4.7"_el) {
        return "L"_el;
    }
    if (value == "2.5.4.8"_el) {
        return "ST"_el;
    }
    if (value == "2.5.4.10"_el) {
        return "O"_el;
    }
    if (value == "2.5.4.11"_el) {
        return "OU"_el;
    }
    if (value == "1.2.840.113549.1.9.1"_el) {
        return "E"_el;
    }
    return value;
}

auto X509Name::escapedValue(const text::String &value) -> text::String {
    auto reader = text::StringCharReader{value};
    auto result = text::StringEditor{};
    auto index = std::size_t{};
    const auto count = value.characterLength().toSizeT();
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        const auto edgeSpace = character == U' ' && (index == 0U || index + 1U == count);
        const auto needsEscape = edgeSpace || (index == 0U && character == U'#') || character == U',' ||
            character == U'+' || character == U'"' || character == U'\\' || character == U'<' || character == U'>' ||
            character == U';' || character == U'=';
        if (needsEscape) {
            result.append(U'\\');
        }
        result.append(character);
        ++index;
    }
    return text::String{result};
}

}
