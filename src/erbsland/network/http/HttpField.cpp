// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpField.hpp"

#include "../impl/http/HttpGrammar.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::network {

using namespace text::literals;

HttpField::HttpField(HttpFieldName name, text::String value) : _name{std::move(name)}, _value{std::move(value)} {
    if (!_name.isValid()) {
        throw err::ParameterError{"An HTTP field requires a valid name."_el, "name"_el};
    }
    if (!impl::http_grammar::isFieldValue(_value)) {
        throw err::ParameterError{"An HTTP field value contains a prohibited control byte."_el, "value"_el};
    }
}

HttpField::HttpField(text::String name, text::String value) :
    HttpField{HttpFieldName{std::move(name)}, std::move(value)} {
}

}
