// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationPartIdentifier.hpp"

#include "impl/ApplicationPartIdentifier.hpp"

#include "../err/ParameterFailure.hpp"
#include "../text/AsciiCategory.hpp"
#include "../text/Literals.hpp"
#include "../unit/CpCount.hpp"

namespace erbsland::core {

using namespace text::literals;

auto ApplicationPartIdentifier::create(text::String name) -> ApplicationPartIdentifierPtr {
    if (name.isEmpty() || !name.containsOnly(text::AsciiCategory::DottedName)) {
        throw err::ParameterFailure{
            "The application-part identifier must be a non-empty ASCII reverse-domain token."_el, "name"_el};
    }
    if (name.characterLength() > unit::CpCount{200}) {
        throw err::ParameterFailure{"The application-part identifier exceeds 200 characters."_el, "name"_el};
    }
    return std::make_shared<impl::ApplicationPartIdentifier>(std::move(name));
}

}
