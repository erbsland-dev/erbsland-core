// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationPartIdentifier.hpp"

#include "impl/ApplicationPartIdentifier.hpp"

#include "../err/ParameterError.hpp"
#include "../text/CharSet.hpp"
#include "../text/Literals.hpp"
#include "../unit/CpLength.hpp"

namespace erbsland::core {

using namespace text::literals;

auto ApplicationPartIdentifier::create(text::String name) -> ApplicationPartIdentifierPtr {
    static const auto validCharacters = text::CharSet::fromPattern("-_.a-zA-Z0-9"_el);
    if (name.isEmpty() || !name.isValidUtf8() || !name.containsOnly(validCharacters)) {
        throw err::ParameterError{
            "The application-part identifier must be a non-empty ASCII reverse-domain token."_el, "name"_el};
    }
    if (name.characterLength() > unit::CpLength{200}) {
        throw err::ParameterError{"The application-part identifier exceeds 200 characters."_el, "name"_el};
    }
    return std::make_shared<impl::ApplicationPartIdentifier>(std::move(name));
}

}
