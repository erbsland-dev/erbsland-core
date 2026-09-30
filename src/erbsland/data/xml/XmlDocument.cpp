// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "XmlDocument.hpp"

#include "impl/XmlCodec.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::data::xml {

using namespace text::literals;

auto XmlDocument::root() const noexcept -> XmlNodePtr {
    for (const auto &node : _nodes) {
        if (node->type() == XmlNodeType::Element) {
            return node;
        }
    }
    return {};
}

auto XmlDocument::add(XmlNodePtr node) -> XmlNodePtr {
    if (!node) {
        throw err::ParameterError{"XML document node cannot be null."_el, "node"_el};
    }
    _nodes.append(node);
    return node;
}

auto XmlDocument::addRoot(text::String name, text::String text) -> XmlNodePtr {
    if (root()) {
        throw err::ParameterError{"XML document already has a root."_el, "name"_el};
    }
    return add(XmlNode::createElement(std::move(name), std::move(text)));
}

auto XmlDocument::toString(XmlFormatOptions options) const -> text::String {
    return impl::XmlCodec::encode(*this, options);
}

auto XmlDocument::fromString(const text::String &text, XmlParseOptions options) noexcept -> std::optional<XmlDocument> {
    try {
        return fromStringOrThrow(text, options);
    } catch (...) {
        return std::nullopt;
    }
}

auto XmlDocument::fromStringOrThrow(const text::String &text, XmlParseOptions options) -> XmlDocument {
    return impl::XmlCodec::decode(text, options);
}

}
