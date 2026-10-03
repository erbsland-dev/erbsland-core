// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpMediaTypeParameter.hpp"

#include "../impl/http/HttpGrammar.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../text/Char.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringTransformations.hpp"

namespace erbsland::network {

using namespace text::literals;

HttpMediaTypeParameter::HttpMediaTypeParameter(text::String name, text::String value) {
    if (!impl::http_grammar::isToken(name)) {
        throw err::ParameterFailure{"A media-type parameter name must be a non-empty ASCII token."_el, "name"_el};
    }
    if (!value.isEncodingValid() || !impl::http_grammar::isFieldValue(value)) {
        throw err::ParameterFailure{
            "A media-type parameter value must be valid UTF-8 without prohibited controls."_el, "value"_el};
    }
    _name = text::toAsciiLowercase(name);
    _value = std::move(value);
}

}
