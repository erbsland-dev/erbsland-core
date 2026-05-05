// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Exception.hpp"

#include "../text/impl/UnsafeU8StringViewAccess.hpp"

#include <utility>

namespace erbsland::err {

auto Exception::what() const noexcept -> mem::UnsafeConstCharPtr {
    if (_reason.isEmpty()) {
        const static std::string staticEmpty{};
        return staticEmpty.data();
    }
    return text::impl::UnsafeU8StringViewAccess{_reason}.dataView().dataSpan().data();
}

auto Exception::toString() const noexcept -> text::StringView {
    // For the default implementation, we only have the reason text to return.
    return reason();
}

}
