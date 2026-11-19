// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TokenView.hpp"

namespace tool::anti_patterns::impl {

using namespace el::text::literals;

TokenView::TokenView(const SourceIndex &source) {
    for (const auto &token : source.tokens()) {
        if (token.kind() != el::cpp::CppToken::Kind::Comment) {
            _tokens.append(token);
        }
    }
}

auto TokenView::at(const el::ItemIndex index) const -> const el::cpp::CppToken & {
    if (!index.isWithin(_tokens.count())) {
        return _end;
    }
    return _tokens.getRef(index);
}

auto TokenView::matches(el::ItemIndex index, const std::initializer_list<el::String> words) const -> bool {
    for (const auto &word : words) {
        if (at(index).isEnd() || at(index).text() != word) {
            return false;
        }
        ++index;
    }
    return true;
}

auto TokenView::closing(const el::ItemIndex opening) const -> el::ItemIndex {
    auto stack = el::StringList{};
    for (auto index = opening; index.isWithin(count()); ++index) {
        const auto &token = at(index);
        if (token.kind() == el::cpp::CppToken::Kind::Directive) {
            break;
        }
        const auto &word = token.text();
        if (word == "("_el) {
            stack.append(")"_el);
        } else if (word == "{"_el) {
            stack.append("}"_el);
        } else if (word == "["_el) {
            stack.append("]"_el);
        } else if (word == ")"_el || word == "}"_el || word == "]"_el) {
            if (stack.isEmpty() || stack.last() != word) {
                break;
            }
            stack.removeLast();
            if (stack.isEmpty()) {
                return index;
            }
        } else if (index == opening) {
            break;
        }
    }
    return el::ItemIndex::noIndex();
}

auto TokenView::afterType(el::ItemIndex index, const el::String &name, const el::String &domain) const
    -> el::ItemIndex {
    if (!index.isZero() && at(index.decremented()).text() == "::"_el) {
        return el::ItemIndex::noIndex();
    }
    if (matches(index, {"el"_el, "::"_el}) || matches(index, {"erbsland"_el, "::"_el})) {
        index.advance(el::ItemCount{2});
    }
    if (matches(index, {domain, "::"_el})) {
        index.advance(el::ItemCount{2});
    }
    if (at(index).text() == name) {
        return index.incremented();
    }
    return el::ItemIndex::noIndex();
}

}
