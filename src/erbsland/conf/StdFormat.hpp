// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ConfErrorCategory.hpp"
#include "Location.hpp"
#include "Name.hpp"
#include "NamePath.hpp"
#include "NameType.hpp"
#include "SourceIdentifier.hpp"
#include "Value.hpp"
#include "ValueType.hpp"

#include "impl/lexer/TokenType.hpp"
#include "impl/vr/ConfKey.hpp"
#include "impl/vr/DependencyMode.hpp"
#include "vr/ConstraintType.hpp"

#include "../text/StdFormat.hpp"

#include <format>

template <>
struct std::formatter<erbsland::conf::ConfErrorCategory> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::conf::ConfErrorCategory &value, std::format_context &ctx) const {
        return Base::format(value.toText(), ctx);
    }
};

template <>
struct std::formatter<erbsland::conf::Location> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::conf::Location &value, std::format_context &ctx) const {
        return Base::format(value.toText(), ctx);
    }
};

template <>
struct std::formatter<erbsland::conf::Name> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::conf::Name &value, std::format_context &ctx) const {
        return Base::format(value.toPathText(), ctx);
    }
};

template <>
struct std::formatter<erbsland::conf::NamePath> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::conf::NamePath &value, std::format_context &ctx) const {
        return Base::format(value.toText(), ctx);
    }
};

template <>
struct std::formatter<erbsland::conf::NameType> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::conf::NameType value, std::format_context &ctx) const {
        return Base::format(erbsland::conf::toString(value), ctx);
    }
};

template <>
struct std::formatter<erbsland::conf::SourceIdentifier> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::conf::SourceIdentifier &value, std::format_context &ctx) const {
        return Base::format(value.toText(), ctx);
    }
};

template <>
struct std::formatter<erbsland::conf::Value> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::conf::Value &value, std::format_context &ctx) const {
        return Base::format(value.toTextRepresentation(), ctx);
    }
};

template <>
struct std::formatter<erbsland::conf::ValueType> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::conf::ValueType value, std::format_context &ctx) const {
        return Base::format(value.toText(), ctx);
    }
};

template <>
struct std::formatter<erbsland::conf::vr::ConstraintType> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::conf::vr::ConstraintType value, std::format_context &ctx) const {
        return Base::format(value.toText(), ctx);
    }
};

template <>
struct std::formatter<erbsland::conf::impl::TokenType> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::conf::impl::TokenType value, std::format_context &ctx) const {
        return Base::format(erbsland::conf::impl::toString(value), ctx);
    }
};

template <>
struct std::formatter<erbsland::conf::impl::ConfKey> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::conf::impl::ConfKey &value, std::format_context &ctx) const {
        return Base::format(value.toText(), ctx);
    }
};

template <>
struct std::formatter<erbsland::conf::impl::DependencyMode> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::conf::impl::DependencyMode value, std::format_context &ctx) const {
        return Base::format(value.toText(), ctx);
    }
};
