// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EnvironmentVariableBackend.hpp"

#include "../../../text/Literals.hpp"

namespace erbsland::system::impl {

using namespace text::literals;

auto EnvironmentVariableBackend::getPathVariable(const text::String &name) const -> std::optional<text::String> {
    const auto value = get(name);
    if (value && value->isEncodingValid()) {
        return value;
    }
    return {};
}

auto EnvironmentVariableBackend::getPath() const -> std::optional<text::String> {
    return get(cPathVariable);
}

}
