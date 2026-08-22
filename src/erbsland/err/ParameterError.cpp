// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ParameterError.hpp"

#include "../text/StringEditor.hpp"

#include <utility>

namespace erbsland::err {

using namespace text::literals;

ParameterError::ParameterError(text::String reason, text::String parameter) noexcept :
    LogicError{std::move(reason)}, _parameterName{std::move(parameter)} {
}

ParameterError::ParameterError(const std::string_view reason, const std::string_view parameterName) noexcept :
    LogicError{reason}, _parameterName{parameterName} {
}

auto ParameterError::toString() const noexcept -> text::String {
    return text::String::fromJoined({reason(), " (parameter: "_el, _parameterName, ")"_el});
}

}
