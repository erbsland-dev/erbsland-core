// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EnvironmentVariables.hpp"

#include "PlatformError.hpp"

#include "../err/ParameterFailure.hpp"
#include "../err/ParseError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::system {

using namespace err;
using namespace path;
using namespace text::literals;

auto EnvironmentVariables::readPaths() const -> impl::EnvironmentPaths {
    auto result = impl::EnvironmentPaths{*_backend, [this](const text::String &name) {
                                             validateName(name);
                                             return _backend->getPathVariable(name);
                                         }};
    if (const auto value = _backend->getPath()) {
        result.read(*value);
    }
    return result;
}

auto EnvironmentVariables::pathsOrThrow() const -> PathList {
    return readPaths().paths();
}

auto EnvironmentVariables::executableCandidatesOrThrow(const text::String &name) const -> PathList {
    return readPaths().executableCandidates(name);
}

void EnvironmentVariables::prependPathOrThrow(const Path &path) {
    insertPathOrThrow(unit::ItemIndex::zero(), path);
}

void EnvironmentVariables::appendPathOrThrow(const Path &path) {
    auto entries = readPaths();
    entries.append(path);
    setOrThrow(impl::EnvironmentVariableBackend::cPathVariable, entries.text());
}

void EnvironmentVariables::insertPathOrThrow(const unit::ItemIndex index, const Path &path) {
    auto entries = readPaths();
    entries.insert(index, path);
    setOrThrow(impl::EnvironmentVariableBackend::cPathVariable, entries.text());
}

void EnvironmentVariables::removePathOrThrow(const Path &path) {
    auto entries = readPaths();
    entries.remove(path);
    // Preserve the distinction between an absent PATH and an empty PATH (current directory).
    if (_backend->get(impl::EnvironmentVariableBackend::cPathVariable).has_value()) {
        setOrThrow(impl::EnvironmentVariableBackend::cPathVariable, entries.text());
    }
}

void EnvironmentVariables::removePathsInOrThrow(const Path &base) {
    auto entries = readPaths();
    entries.remove(base, true);
    if (_backend->get(impl::EnvironmentVariableBackend::cPathVariable).has_value()) {
        setOrThrow(impl::EnvironmentVariableBackend::cPathVariable, entries.text());
    }
}

auto EnvironmentVariables::paths() const noexcept -> PathList {
    try {
        return pathsOrThrow();
    } catch (const ParameterFailure &) {
        return {};
    } catch (const ParseError &) {
        return {};
    } catch (const PlatformError &) {
        return {};
    }
}

auto EnvironmentVariables::executableCandidates(const text::String &name) const noexcept -> PathList {
    try {
        return executableCandidatesOrThrow(name);
    } catch (const ParameterFailure &) {
        return {};
    } catch (const ParseError &) {
        return {};
    } catch (const PlatformError &) {
        return {};
    }
}

auto EnvironmentVariables::prependPath(const Path &path) noexcept -> bool {
    try {
        prependPathOrThrow(path);
        return true;
    } catch (const ParameterFailure &) {
        return false;
    } catch (const ParseError &) {
        return false;
    } catch (const PlatformError &) {
        return false;
    }
}

auto EnvironmentVariables::appendPath(const Path &path) noexcept -> bool {
    try {
        appendPathOrThrow(path);
        return true;
    } catch (const ParameterFailure &) {
        return false;
    } catch (const ParseError &) {
        return false;
    } catch (const PlatformError &) {
        return false;
    }
}

auto EnvironmentVariables::insertPath(const unit::ItemIndex index, const Path &path) noexcept -> bool {
    try {
        insertPathOrThrow(index, path);
        return true;
    } catch (const ParameterFailure &) {
        return false;
    } catch (const ParseError &) {
        return false;
    } catch (const PlatformError &) {
        return false;
    }
}

auto EnvironmentVariables::removePath(const Path &path) noexcept -> bool {
    try {
        removePathOrThrow(path);
        return true;
    } catch (const ParameterFailure &) {
        return false;
    } catch (const ParseError &) {
        return false;
    } catch (const PlatformError &) {
        return false;
    }
}

auto EnvironmentVariables::removePathsIn(const Path &base) noexcept -> bool {
    try {
        removePathsInOrThrow(base);
        return true;
    } catch (const ParameterFailure &) {
        return false;
    } catch (const ParseError &) {
        return false;
    } catch (const PlatformError &) {
        return false;
    }
}

}
