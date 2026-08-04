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

    auto partRange = Range{};
    advanceToNextPart(&partRange);
    return _text.slice(partRange);
}

template <typename tString>
void StringSplitter<tString>::skip() noexcept {
    advanceToNextPart(nullptr);
}

template <typename tString>
void StringSplitter<tString>::advanceToNextPart(Range *const partRange) noexcept {
    if (_isAtEnd) {
        return;
    }

    const auto partStart = _position;
    const auto textEnd = Index::end(_text.length());
    const auto separatorPosition = _text.findFirstOf(_separators, partStart);
    if (separatorPosition.isNoIndex()) {
        _position = textEnd;
        _isAtEnd = true;
        if (partRange != nullptr) {
            *partRange = Range{partStart, textEnd};
        }
        return;
    }

    auto nextPosition = separatorPosition;
    _text.advance(nextPosition);
    _position = nextPosition;
    if (_mode == StringSplitMode::KeepSeparator) {
        _isAtEnd = nextPosition >= textEnd;
        if (partRange != nullptr) {
            *partRange = Range{partStart, nextPosition};
        }
        return;
    }
    if (partRange != nullptr) {
        *partRange = Range{partStart, separatorPosition};
    }
}

template <typename tString>
void StringSplitter<tString>::reset() noexcept {
    _position = Index::zero();
    _isAtEnd = false;
}

}
