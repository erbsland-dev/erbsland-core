// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EnvironmentVariables.hpp"

#include "PlatformError.hpp"

#include "../err/Exception.hpp"
#include "../err/NotFoundError.hpp"
#include "../err/ParameterFailure.hpp"
#include "../err/ParseError.hpp"
#include "../text/impl/UnsafeU8StringAccess.hpp"
#include "../text/Literals.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::system {

using namespace err;
using namespace text;
using namespace text::literals;

EnvironmentVariables::EnvironmentVariables() : EnvironmentVariables{impl::createEnvironmentVariableBackend()} {
}

EnvironmentVariables::EnvironmentVariables(impl::EnvironmentVariableBackendPtr backend) : _backend{std::move(backend)} {
    if (_backend == nullptr) {
        throw ParameterFailure{"The environment-variable backend is missing."_el, "backend"_el};
    }
}

auto EnvironmentVariables::get(const String &name) const noexcept -> std::optional<String> {
    try {
        validateName(name);
        return _backend->get(name);
    } catch (const ParseError &) {
        return std::nullopt;
    } catch (const system::PlatformError &) {
        return std::nullopt;
    }
}

auto EnvironmentVariables::get(const String &name, String defaultValue) const noexcept -> String {
    if (auto value = get(name); value.has_value()) {
        return std::move(*value);
    }
    return defaultValue;
}

auto EnvironmentVariables::getOrThrow(const String &name) const -> String {
    validateName(name);
    auto value = _backend->get(name);
    if (!value.has_value()) {
        throw NotFoundError{"The environment variable does not exist."_el};
    }
    return std::move(*value);
}

auto EnvironmentVariables::set(const String &name, const String &value) noexcept -> bool {
    try {
        setOrThrow(name, value);
        return true;
    } catch (const ParseError &) {
        return false;
    } catch (const system::PlatformError &) {
        return false;
    }
}

void EnvironmentVariables::setOrThrow(const String &name, const String &value) {
    validateName(name);
    validateValue(value);
    _backend->set(name, value);
}

auto EnvironmentVariables::remove(const String &name) noexcept -> bool {
    try {
        removeOrThrow(name);
        return true;
    } catch (const ParseError &) {
        return false;
    } catch (const system::PlatformError &) {
        return false;
    }
}

void EnvironmentVariables::removeOrThrow(const String &name) {
    validateName(name);
    _backend->remove(name);
}

void EnvironmentVariables::validateName(const String &name) {
    if (name.isEmpty()) {
        throw ParseError{"The environment-variable name must not be empty."_el};
    }
    const auto data = text::impl::UnsafeU8StringAccess{name}.dataSpan();
    if (std::ranges::find(data, '=') != data.end()) {
        throw ParseError{"The environment-variable name must not contain an equals sign."_el};
    }
    if (containsNull(name)) {
        throw ParseError{"The environment-variable name must not contain a null byte."_el};
    }
}

void EnvironmentVariables::validateValue(const String &value) {
    if (containsNull(value)) {
        throw ParseError{"The environment-variable value must not contain a null byte."_el};
    }
}

auto EnvironmentVariables::containsNull(const String &value) noexcept -> bool {
    const auto data = text::impl::UnsafeU8StringAccess{value}.dataSpan();
    return std::ranges::find(data, '\0') != data.end();
}

}
