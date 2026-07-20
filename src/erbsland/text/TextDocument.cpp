// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextDocument.hpp"

#include "PlainTextRenderer.hpp"

#include <utility>

namespace erbsland::text {

TextDocument::TextDocument() : _root{TextNode::createDocument()} {
}

auto TextDocument::add(TextNodeType type) -> TextNodePtr {
    return _root->add(type);
}

auto TextDocument::addParagraph() -> TextNodePtr {
    return _root->addParagraph();
}

auto TextDocument::addSection() -> TextNodePtr {
    return _root->addSection();
}

auto TextDocument::addBlockquote() -> TextNodePtr {
    return _root->addBlockquote();
}

auto TextDocument::addLineBreak() -> TextNodePtr {
    return _root->addLineBreak();
}

auto TextDocument::addHeading(const TextNode::Level level) -> TextNodePtr {
    return _root->addHeading(level);
}

auto TextDocument::addBulletList(const TextNode::Level level) -> TextNodePtr {
    return _root->addBulletList(level);
}

auto TextDocument::addNumberedList(const TextNode::Level level) -> TextNodePtr {
    return _root->addNumberedList(level);
}

auto TextDocument::addDefinitionList() -> TextNodePtr {
    return _root->addDefinitionList();
}

auto TextDocument::addCodeBlock(String language) -> TextNodePtr {
    return _root->addCodeBlock(std::move(language));
}

auto TextDocument::addCodeSnippet(CodeSnippet snippet, CodeSnippetMarkerList markers) -> TextNodePtr {
    return _root->addCodeSnippet(std::move(snippet), std::move(markers));
}

auto TextDocument::addHorizontalLine() -> TextNodePtr {
    return _root->addHorizontalLine();
}

auto TextDocument::addText(String text) -> TextNodePtr {
    return _root->addText(std::move(text));
}

auto TextDocument::addUnsupported(String text) -> TextNodePtr {
    return _root->addUnsupported(std::move(text));
}

auto TextDocument::addError(String text) -> TextNodePtr {
    return _root->addError(std::move(text));
}

auto TextDocument::isEmpty() const noexcept -> bool {
    return _root == nullptr || !_root->hasChildren();
}

auto TextDocument::toString() const -> String {
    return PlainTextRenderer{*this}.build();
}

}
