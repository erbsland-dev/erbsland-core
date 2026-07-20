// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::text::impl {

template <typename tString>
StringSplitter<tString>::StringSplitter(String text, const Char separator, const StringSplitMode mode) noexcept :
    StringSplitter{std::move(text), CharSet{separator}, mode} {
}

template <typename tString>
StringSplitter<tString>::StringSplitter(String text, CharSet separators, const StringSplitMode mode) noexcept :
    _text{std::move(text)}, _separators{std::move(separators)}, _mode{mode} {
}

template <typename tString>
auto StringSplitter<tString>::remaining() const noexcept -> String {
    const auto textEnd = Index::end(_text.length());
    return _text.slice(Range{_position, textEnd});
}

template <typename tString>
auto StringSplitter<tString>::next() noexcept -> String {
    if (_isAtEnd) {
        return {};
    }

    const auto partStart = _position;
    const auto textEnd = Index::end(_text.length());
    const auto separatorPosition = _text.findFirstOf(_separators, partStart);
    if (separatorPosition.isNoIndex()) {
        _position = textEnd;
        _isAtEnd = true;
        return _text.slice(Range{partStart, textEnd});
    }

    auto nextPosition = separatorPosition;
    _text.advance(nextPosition);
    _position = nextPosition;
    if (_mode == StringSplitMode::KeepSeparator) {
        _isAtEnd = nextPosition >= textEnd;
        return _text.slice(Range{partStart, nextPosition});
    }
    return _text.slice(Range{partStart, separatorPosition});
}

template <typename tString>
void StringSplitter<tString>::reset() noexcept {
    _position = Index::zero();
    _isAtEnd = false;
}

}
