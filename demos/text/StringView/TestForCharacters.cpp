// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <numbers>

/// A validation error that can be thrown by the validation functions.
class ValidationError : public el::Exception {
public:
    explicit ValidationError(const el::StringView &message) : Exception(message) {}
};

/// Validate if the given email address is valid.
void validateEmailAddress(const el::StringView &emailAddress) {
    static const auto requiredAt = "@"_el;
    static const auto allowedDomainChars = el::CharSet::fromPattern("-a-zA-Z0-9."_el);
    static const auto allowedLocalChars = el::CharSet::fromPattern("-a-zA-Z0-9._+!#$%&'*=?^`{|}~"_el);
    if (!emailAddress.contains(requiredAt)) {
        throw ValidationError{"Email address must contain '@' character."_el};
    }
    if (emailAddress.count(requiredAt) > el::ElementCount{1}) {
        throw ValidationError{"Email address can only contain one '@' character."_el};
    }
    const auto indexOfAt = emailAddress.find(requiredAt);
    const auto domain = emailAddress.slice(el::ByteRange{
        indexOfAt + requiredAt.length(),
        el::ByteLength::infinite()});
    if (!domain.containsOnly(allowedDomainChars)) {
        throw ValidationError{"Email domain contains invalid characters."_el};
    }
    if (domain.isEmpty()) {
        throw ValidationError{"Email domain must not be empty."_el};
    }
    const auto local = emailAddress.slice(el::ByteRange{el::ByteIndex::zero(), indexOfAt});
    if (!local.containsOnly(allowedLocalChars)) {
        throw ValidationError{"Email local part contains invalid characters."_el};
    }
    if (local.isEmpty()) {
        throw ValidationError{"Email local part must not be empty."_el};
    }
}

/// This demo shows how text, character-set, and accepted-character tests can be used as an efficient input filter.
void testForCharacters() {
    auto emailAddressesToValidate = el::StringViewList{
        "tree🌲@forest.org"_el,
        "anna.wald@example.com"_el,
        "river@mountain!.org"_el,
        "maria.silva@green-energy.eu"_el,
        "space in@address.com"_el,
        "takashi.yama@tokyo.jp"_el,
        "wolf@nächtlich.de"_el, // fails: unicode domain
        "luca+weather@forest-mail.net"_el,
        "missing-at-symbol.example.com"_el,
        "fatma+birds@forest.example"_el,
        "alice\nbob@example.com"_el,
        "nora+rain@climate.example"_el,
        "double@@example.com"_el, // fails: repeated at sign
        "forest@domain#name.com"_el,
        "carlos.sunrise@weather.es"_el,
        "@empty-local.org"_el,
        "greta.wind@north-sea.dk"_el,
        "user@exa mple.com"_el,
        "sofia.rivera@biology.org"_el,
        "雨@example.jp"_el, // fails: unicode local part
        "mehmet_istanbul@trees.dev"_el,
        "invalid<char>@example.com"_el,
        "élise@fleurs.fr"_el, // fails: unicode local part
        "empty-domain@"_el,
        "jan.kowalski@oakforest.pl"_el,
    };

    el::io::printLine("Validating all "_el, emailAddressesToValidate.count(), " email addresses:"_el);
    emailAddressesToValidate.forEach([](const el::StringView &email) {
        el::io::print("- \"", email.toEscaped(el::EscapeFormat::Cpp), "\": "_el);
        try {
            validateEmailAddress(email);
            el::io::printLine("✅"_el);
        } catch (const ValidationError &error) {
            el::io::printLine("❌ "_el, error);
        }
    });
}
