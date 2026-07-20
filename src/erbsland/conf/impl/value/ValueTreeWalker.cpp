// Copyright (c) 2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ValueTreeWalker.hpp"

#include <ranges>
#include <vector>

namespace erbsland::conf::impl {

void ValueTreeWalker::walk(const Visit &visit) const {
    if (!_root) {
        return;
    }
    // Explicit stack for iterative DFS (preorder).
    // We push children in reverse declaration order to maintain the original order when popping.
    std::vector<conf::ValuePtr> stack;
    stack.reserve(32);
    stack.push_back(_root);

    while (!stack.empty()) {
        auto node = stack.back();
        stack.pop_back();
        if (!node) {
            continue;
        }

        if (!_filter || _filter(node)) {
            if (visit) {
                visit(node);
            }
            for (const auto &rit : std::ranges::reverse_view(*node)) {
                stack.push_back(rit);
            }
        }
    }
}

}
