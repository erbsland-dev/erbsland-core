// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ErrorAdapter.hpp"

namespace erbsland::conf::impl::placeholder {

auto toConfErrorCategory(const text::placeholder::ReplacerErrorCategory value) noexcept -> ConfErrorCategory {
    using text::placeholder::ReplacerErrorCategory;
    switch (value) {
    case ReplacerErrorCategory::Syntax:
        return ConfErrorCategory::Syntax;
    case ReplacerErrorCategory::UnexpectedEnd:
        return ConfErrorCategory::UnexpectedEnd;
    case ReplacerErrorCategory::LimitExceeded:
        return ConfErrorCategory::LimitExceeded;
    case ReplacerErrorCategory::Unsupported:
        return ConfErrorCategory::Unsupported;
    case ReplacerErrorCategory::ValueNotFound:
        return ConfErrorCategory::ValueNotFound;
    case ReplacerErrorCategory::NameConflict:
        return ConfErrorCategory::NameConflict;
    case ReplacerErrorCategory::Validation:
        return ConfErrorCategory::Validation;
    case ReplacerErrorCategory::Access:
        return ConfErrorCategory::Access;
    }
    return ConfErrorCategory::Internal;
}

}
