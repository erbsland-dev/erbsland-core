// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Document.hpp"
#include "Value.hpp"

#include "../utilities/InternalError.hpp"

#include <memory>
#include <utility>

namespace erbsland::conf::impl {

/// Get the implementation value behind a public value pointer.
[[nodiscard]] inline auto getImplValue(const conf::ValuePtr &value) -> ValuePtr {
    if (value == nullptr) {
        return {};
    }
    auto valueImpl = std::dynamic_pointer_cast<Value>(value);
    if (valueImpl == nullptr) {
        throwInternalError("The value tree contains a value with an unknown implementation"_el);
    }
    return valueImpl;
}

/// Invoke a callable with the implementation value behind a public value pointer.
template <class Fn>
void callImplValueFn(const conf::ValuePtr &value, Fn &&fn) {
    if (auto documentImpl = std::dynamic_pointer_cast<Document>(value); documentImpl != nullptr) {
        std::forward<Fn>(fn)(documentImpl);
    } else if (auto valueImpl = std::dynamic_pointer_cast<Value>(value); valueImpl != nullptr) {
        std::forward<Fn>(fn)(valueImpl);
    } else {
        throwInternalError("The value tree contains a value with an unknown implementation"_el);
    }
}

}
