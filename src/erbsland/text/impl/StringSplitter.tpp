// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../util/impl/LoopControl.hpp"

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
    if (isAtEnd()) {
        return {};
    }
    const auto textEnd = Index::end(_text.length());
    return _text.slice(Range{_position, textEnd});
}

template <typename tString>
auto StringSplitter<tString>::next() noexcept -> String {
    if (isAtEnd()) {
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
auto StringSplitter<tString>::forEach(const ProcessPartFn &function) -> util::LoopResult {
    if (function == nullptr) {
        return util::LoopResult::Success;
    }
    while (!isAtEnd()) {
        const auto part = next();
        const auto status = function(part);
        if (status != util::LoopStatus::Continue) {
            return util::impl::loopStatusToResult(status);
        }
    }
    return util::LoopResult::Success;
}

template <typename tString>
auto StringSplitter<tString>::forEach(const ProcessPartWithIndexFn &function) -> util::LoopResult {
    if (function == nullptr) {
        return util::LoopResult::Success;
    }
    while (!isAtEnd()) {
        const auto index = splitIndex();
        const auto part = next();
        const auto status = function(part, index);
        if (status != util::LoopStatus::Continue) {
            return util::impl::loopStatusToResult(status);
        }
    }
    return util::LoopResult::Success;
}

template <typename tString>
void StringSplitter<tString>::advanceToNextPart(Range *const partRange) noexcept {
    if (isAtEnd()) {
        return;
    }

    const auto partStart = _position;
    const auto textEnd = Index::end(_text.length());
    const auto separatorPosition = _text.findFirstOf(_separators, partStart);
    if (separatorPosition.isNoIndex()) {
        _position = Index::noIndex();
        if (partRange != nullptr) {
            *partRange = Range{partStart, textEnd};
        }
        ++_splitCount;
        return;
    }

    auto nextPosition = separatorPosition;
    _text.advance(nextPosition);
    _position = nextPosition;
    ++_splitCount;
    if (_mode == StringSplitMode::KeepSeparator) {
        if (!nextPosition.isWithin(_text.length())) {
            _position = Index::noIndex();
        }
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
    _splitCount = unit::ItemCount::zero();
}

}
