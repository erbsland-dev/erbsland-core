// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>

namespace erbsland::text {

template <typename Fn>
    requires(
        std::invocable<Fn, const TextNode &> &&
        std::same_as<std::invoke_result_t<Fn, const TextNode &>, TextWalkStatus>)
auto TextNode::walk(Fn nodeFn) const -> TextWalkResult {
    switch (std::invoke(nodeFn, *this)) {
    case TextWalkStatus::Continue:
        break;
    case TextWalkStatus::Stop:
        return TextWalkResult::Stopped;
    case TextWalkStatus::Failure:
        return TextWalkResult::Failure;
    }
    for (const auto &child : _children) {
        const auto childResult = child->walk(nodeFn);
        if (childResult != TextWalkResult::Success) {
            return childResult;
        }
    }
    return TextWalkResult::Success;
}

template <typename Fn>
    requires(
        std::invocable<Fn, const TextNode &> && std::convertible_to<std::invoke_result_t<Fn, const TextNode &>, bool>)
auto TextNode::anyOf(Fn nodeFn) const -> bool {
    const auto result = walk([&nodeFn](const TextNode &node) -> TextWalkStatus {
        return std::invoke(nodeFn, node) ? TextWalkStatus::Stop : TextWalkStatus::Continue;
    });
    return result == TextWalkResult::Stopped;
}

}
