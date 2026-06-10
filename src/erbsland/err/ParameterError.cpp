// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ParameterError.hpp"

#include "../text/StringBuilder.hpp"

#include <utility>

namespace erbsland::err {

using namespace text::literals;

ParameterError::ParameterError(text::StringView reason, text::StringView parameter) noexcept :
    LogicError{std::move(reason)}, _parameterName{std::move(parameter)} {
}

ParameterError::ParameterError(const std::string_view reason, const std::string_view parameterName) noexcept :
    LogicError{reason}, _parameterName{text::U8String(parameterName)} {
}

auto ParameterError::toString() const noexcept -> text::StringView {
    auto builder = text::StringBuilder::basedOn(_reason);
    builder.append(" (parameter: "_el);
    builder.append(_parameterName);
    builder.append(")"_el);
    return builder.toString();
}

}
