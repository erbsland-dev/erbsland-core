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

using namespace text;
using namespace text::literals;

HostResolverErrorContext::HostResolverErrorContext(
    const int errorCode,
    String errorMessage,
    const system::PlatformErrorCategory category,
    const bool isRetryable) noexcept :
    _errorCode{errorCode}, _errorMessage{std::move(errorMessage)}, _category{category}, _isRetryable{isRetryable} {
}

auto HostResolverErrorContext::category() const noexcept -> system::PlatformErrorCategory {
    return _category;
}

auto HostResolverErrorContext::toString() const noexcept -> String {
    return _errorMessage;
}

auto HostResolverErrorContext::toTextDocument() const -> TextDocument {
    auto document = TextDocument{};
    auto list = document.add(TextNodeType::FieldList);
    auto code = list->add(TextNodeType::FieldItem);
    code->add(TextNodeType::FieldLabel)->addText("resolver error code"_el);
    code->add(TextNodeType::FieldContent)->addText(String::fromInteger(_errorCode));
    if (!_errorMessage.isEmpty()) {
        auto message = list->add(TextNodeType::FieldItem);
        message->add(TextNodeType::FieldLabel)->addText("message"_el);
        message->add(TextNodeType::FieldContent)->addEscapedText(_errorMessage, EscapeFormat::Display);
    }
    return document;
}

}
