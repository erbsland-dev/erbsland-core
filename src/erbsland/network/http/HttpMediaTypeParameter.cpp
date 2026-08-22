// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpMediaTypeParameter.hpp"

#include "../impl/http/HttpGrammar.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Char.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::network {

using namespace text::literals;

HttpMediaTypeParameter::HttpMediaTypeParameter(text::String name, text::String value) {
    if (!impl::http_grammar::isToken(name)) {
        throw err::ParameterError{"A media-type parameter name must be a non-empty ASCII token."_el, "name"_el};
    }
    if (!value.isValidUtf8() || !impl::http_grammar::isFieldValue(value)) {
        throw err::ParameterError{
            "A media-type parameter value must be valid UTF-8 without prohibited controls."_el, "value"_el};
    }
    _name = name.transformed(text::Char::toAsciiLowercase);
    _value = std::move(value);
}

}
