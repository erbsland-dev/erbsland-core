// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ResourceError.hpp"

#include "../text/String.hpp"

#include <utility>

namespace erbsland::resource {

ResourceError::ResourceError(const ResourceErrorCategory category, text::String message) noexcept :
    err::RuntimeError{std::move(message)}, _category{category} {
}

ResourceError::ResourceError(const ResourceErrorCategory category, const std::string_view message) noexcept :
    err::RuntimeError{message}, _category{category} {
}

}
