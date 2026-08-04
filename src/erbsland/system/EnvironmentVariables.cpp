// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EnvironmentVariables.hpp"

#include "PlatformError.hpp"

#include "../err/Exception.hpp"
#include "../err/ParameterError.hpp"
#include "../text/impl/UnsafeU8StringAccess.hpp"
#include "../text/Literals.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::system {

using namespace text::literals;

EnvironmentVariables::EnvironmentVariables() : EnvironmentVariables{impl::createEnvironmentVariableBackend()} {
}

EnvironmentVariables::EnvironmentVariables(impl::EnvironmentVariableBackendPtr backend) : _backend{std::move(backend)} {
    if (_backend == nullptr) {
        throw err::ParameterError{"The environment-variable backend is missing."_el, "backend"_el};
    }
}

auto EnvironmentVariables::get(const text::String &name) const noexcept -> std::optional<text::String> {
    try {
        validateName(name);
        return _backend->get(name);
    } catch (const err::Exception &) {
        return std::nullopt;
    }
}

auto EnvironmentVariables::get(const text::String &name, text::String defaultValue) const noexcept -> text::String {
    if (auto value = get(name); value.has_value()) {
        return std::move(*value);
    }
    return defaultValue;
}

auto EnvironmentVariables::getOrThrow(const text::String &name) const -> text::String {
    validateName(name);
    auto value = _backend->get(name);
    if (!value.has_value()) {
        throw PlatformError{"The environment variable does not exist."_el};
    }
    return std::move(*value);
}

auto EnvironmentVariables::set(const text::String &name, const text::String &value) noexcept -> bool {
    try {
        setOrThrow(name, value);
        return true;
    } catch (const err::Exception &) {
        return false;
    }
}

void EnvironmentVariables::setOrThrow(const text::String &name, const text::String &value) {
    validateName(name);
    validateValue(value);
    _backend->set(name, value);
}

auto EnvironmentVariables::remove(const text::String &name) noexcept -> bool {
    try {
        removeOrThrow(name);
        return true;
    } catch (const err::Exception &) {
        return false;
    }
}

void EnvironmentVariables::removeOrThrow(const text::String &name) {
    validateName(name);
    _backend->remove(name);
}

void EnvironmentVariables::validateName(const text::String &name) {
    if (name.isEmpty()) {
        throw err::ParameterError{"The environment-variable name must not be empty."_el, "name"_el};
    }
    const auto data = text::impl::UnsafeU8StringAccess{name}.dataView().dataSpan();
    if (std::ranges::find(data, '=') != data.end()) {
        throw err::ParameterError{"The environment-variable name must not contain an equals sign."_el, "name"_el};
    }
    if (containsNull(name)) {
        throw err::ParameterError{"The environment-variable name must not contain a null byte."_el, "name"_el};
    }
}

void EnvironmentVariables::validateValue(const text::String &value) {
    if (containsNull(value)) {
        throw err::ParameterError{"The environment-variable value must not contain a null byte."_el, "value"_el};
    }
}

auto EnvironmentVariables::containsNull(const text::String &value) noexcept -> bool {
    const auto data = text::impl::UnsafeU8StringAccess{value}.dataView().dataSpan();
    return std::ranges::find(data, '\0') != data.end();
}

}
