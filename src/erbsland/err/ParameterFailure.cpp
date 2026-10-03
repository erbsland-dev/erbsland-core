// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ParameterFailure.hpp"

#include "../text/StringEditor.hpp"
#include "../text/StringJoins.hpp"

#include <utility>

namespace erbsland::err {

using namespace text;
using namespace text::literals;

ParameterFailure::ParameterFailure(String reason, String parameter) noexcept :
    LogicFailure{std::move(reason)}, _parameterName{std::move(parameter)} {
}

ParameterFailure::ParameterFailure(const std::string_view reason, const std::string_view parameterName) noexcept :
    LogicFailure{reason}, _parameterName{parameterName} {
}

auto ParameterFailure::toString() const noexcept -> String {
    return text::join(reason(), " (parameter: "_el, _parameterName, ")"_el);
}

}
