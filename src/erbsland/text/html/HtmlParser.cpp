// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HtmlParser.hpp"

#include "impl/HtmlParser.hpp"

#include "../Literals.hpp"

#include "../../err/ParseError.hpp"

#include <memory>
#include <utility>

namespace erbsland::text::html {

using namespace literals;

HtmlParser::HtmlParser(AnyString html) : _impl{std::make_unique<impl::HtmlParser>(std::move(html))} {
}

HtmlParser::~HtmlParser() = default;

HtmlParser::HtmlParser(HtmlParser &&) noexcept = default;

auto HtmlParser::operator=(HtmlParser &&) noexcept -> HtmlParser & = default;

auto HtmlParser::parse() noexcept -> TextDocument {
    try {
        return parseOrThrow();
    } catch (const err::ParseError &error) {
        auto document = TextDocument{};
        document.addError(error.toString());
        return document;
    } catch (...) {
        auto document = TextDocument{};
        document.addError("HTML parser failed."_el);
        return document;
    }
}

auto HtmlParser::parseOrThrow() -> TextDocument {
    return _impl->parse();
}

}
