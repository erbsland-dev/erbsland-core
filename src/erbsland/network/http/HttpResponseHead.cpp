// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpResponseHead.hpp"

#include "../impl/http/HttpGrammar.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::network {

using namespace text::literals;

HttpResponseHead::HttpResponseHead(
    const HttpVersion version, HttpStatus status, text::String reasonPhrase, HttpHeaders headers) :
    _version{version}, _status{status}, _reasonPhrase{std::move(reasonPhrase)}, _headers{std::move(headers)} {
    if (!_version.isValid()) {
        throw err::ParameterError{"An HTTP response head requires a supported version."_el, "version"_el};
    }
    if (!_status.isValid()) {
        throw err::ParameterError{"An HTTP response head requires a valid status."_el, "status"_el};
    }
    if (!impl::http_grammar::isReasonPhrase(_reasonPhrase)) {
        throw err::ParameterError{
            "An HTTP response reason phrase contains a prohibited control byte."_el, "reasonPhrase"_el};
    }
}

auto HttpResponseHead::isValid() const noexcept -> bool {
    return _version.isValid() && _status.isValid() && impl::http_grammar::isReasonPhrase(_reasonPhrase);
}

}
