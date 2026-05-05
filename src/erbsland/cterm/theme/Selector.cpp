// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Selector.hpp"

namespace erbsland::cterm::theme {

auto Selector::requireState(const State state) noexcept -> Selector & {
    _states.add(state);
    return *this;
}

auto Selector::requireStates(const States states) noexcept -> Selector & {
    _states |= states;
    return *this;
}

auto Selector::requireTag(const Tag tag) noexcept -> Selector & {
    _tags.add(tag);
    return *this;
}

auto Selector::requireTags(const Tags tags) noexcept -> Selector & {
    _tags |= tags;
    return *this;
}

auto Selector::matches(const States state, const Tags tags) const noexcept -> bool {
    return state.containsAll(_states) && tags.containsAll(_tags);
}

auto Selector::specificity() const noexcept -> int {
    return _states.size() + _tags.size();
}

auto Selector::hash() const noexcept -> std::size_t {
    return impl::hashCreate(_element.hash(), _part.hash(), _states.hash(), _tags.hash());
}

}
