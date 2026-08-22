// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HostResolverErrorContext.hpp"

#include "../../../text/EscapeFormat.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/String.hpp"
#include "../../../text/TextDocument.hpp"
#include "../../../text/TextNode.hpp"
#include "../../../text/TextNodeType.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

HostResolverErrorContext::HostResolverErrorContext(
    const int errorCode,
    text::String errorMessage,
    const system::PlatformErrorCategory category,
    const bool isRetryable) noexcept :
    _errorCode{errorCode}, _errorMessage{std::move(errorMessage)}, _category{category}, _isRetryable{isRetryable} {
}

auto HostResolverErrorContext::category() const noexcept -> system::PlatformErrorCategory {
    return _category;
}

auto HostResolverErrorContext::toString() const noexcept -> text::String {
    return _errorMessage;
}

auto HostResolverErrorContext::toTextDocument() const -> text::TextDocument {
    auto document = text::TextDocument{};
    auto list = document.add(text::TextNodeType::FieldList);
    auto code = list->add(text::TextNodeType::FieldItem);
    code->add(text::TextNodeType::FieldLabel)->addText("resolver error code"_el);
    code->add(text::TextNodeType::FieldContent)->addText(text::String::fromInteger(_errorCode));
    if (!_errorMessage.isEmpty()) {
        auto message = list->add(text::TextNodeType::FieldItem);
        message->add(text::TextNodeType::FieldLabel)->addText("message"_el);
        message->add(text::TextNodeType::FieldContent)->addEscapedText(_errorMessage, text::EscapeFormat::Display);
    }
    return document;
}

}
