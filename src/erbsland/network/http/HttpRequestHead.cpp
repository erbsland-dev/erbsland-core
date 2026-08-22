// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpRequestHead.hpp"

#include "../impl/http/HttpGrammar.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::network {

using namespace text::literals;

HttpRequestHead::HttpRequestHead(
    HttpMethod method, text::String target, const HttpVersion version, HttpHeaders headers) :
    _method{std::move(method)}, _target{std::move(target)}, _version{version}, _headers{std::move(headers)} {
    if (!_method.isValid()) {
        throw err::ParameterError{"An HTTP request head requires a valid method."_el, "method"_el};
    }
    if (!impl::http_grammar::isRequestTarget(_target)) {
        throw err::ParameterError{"An HTTP request head requires a strict ASCII request-target."_el, "target"_el};
    }
    if (!_version.isValid()) {
        throw err::ParameterError{"An HTTP request head requires a supported version."_el, "version"_el};
    }
}

auto HttpRequestHead::isValid() const noexcept -> bool {
    return _method.isValid() && impl::http_grammar::isRequestTarget(_target) && _version.isValid();
}

}
