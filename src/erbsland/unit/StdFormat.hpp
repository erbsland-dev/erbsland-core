// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ExitCode.hpp"
#include "IntegerAmount.hpp"
#include "IntegerUnitCount.hpp"
#include "IntegerUnitIndex.hpp"
#include "IntegerUnitOffset.hpp"
#include "IntegerUnitRange.hpp"
#include "Version.hpp"

#include "../text/StdFormat.hpp"

#include <format>
#include <string>

template <typename tUnit, typename tRatio, typename tValue>
struct std::formatter<erbsland::unit::IntegerAmount<tUnit, tRatio, tValue>>
    : std::formatter<typename erbsland::unit::IntegerAmount<tUnit, tRatio, tValue>::NativeValue> {
    using Amount = erbsland::unit::IntegerAmount<tUnit, tRatio, tValue>;
    using Base = std::formatter<typename Amount::NativeValue>;

    auto format(const Amount value, std::format_context &ctx) const -> std::format_context::iterator {
        return Base::format(value.toRawValue(), ctx);
    }
};

template <erbsland::unit::impl::ValidIntegerUnit tIntegerUnit>
struct std::formatter<erbsland::unit::IntegerUnitIndex<tIntegerUnit>>
    : std::formatter<typename erbsland::unit::IntegerUnitIndex<tIntegerUnit>::Value> {
    using Index = erbsland::unit::IntegerUnitIndex<tIntegerUnit>;
    using Base = std::formatter<typename Index::Value>;

    auto format(const Index value, std::format_context &ctx) const -> std::format_context::iterator {
        return Base::format(value.toRawValue(), ctx);
    }
};

template <erbsland::unit::impl::ValidIntegerUnit tIntegerUnit>
struct std::formatter<erbsland::unit::IntegerUnitCount<tIntegerUnit>>
    : std::formatter<typename erbsland::unit::IntegerUnitCount<tIntegerUnit>::Value> {
    using Amount = erbsland::unit::IntegerUnitCount<tIntegerUnit>;
    using Base = std::formatter<typename Amount::Value>;

    auto format(const Amount value, std::format_context &ctx) const -> std::format_context::iterator {
        return Base::format(value.toRawValue(), ctx);
    }
};

template <erbsland::unit::impl::ValidIntegerUnit tIntegerUnit>
struct std::formatter<erbsland::unit::IntegerUnitOffset<tIntegerUnit>>
    : std::formatter<typename erbsland::unit::IntegerUnitOffset<tIntegerUnit>::Value> {
    using Offset = erbsland::unit::IntegerUnitOffset<tIntegerUnit>;
    using Base = std::formatter<typename Offset::Value>;

    auto format(const Offset value, std::format_context &ctx) const -> std::format_context::iterator {
        return Base::format(value.toRawValue(), ctx);
    }
};

template <erbsland::unit::impl::ValidIntegerUnit tIntegerUnit>
struct std::formatter<erbsland::unit::IntegerUnitRange<tIntegerUnit>> : std::formatter<std::string> {
    using Range = erbsland::unit::IntegerUnitRange<tIntegerUnit>;
    using Base = std::formatter<std::string>;

    auto format(const Range value, std::format_context &ctx) const -> std::format_context::iterator {
        return Base::format(std::format("{}:{}", value.beginIndex(), value.length()), ctx);
    }
};

template <erbsland::unit::VersionPart tPart>
struct std::formatter<erbsland::unit::VersionComponent<tPart>> : std::formatter<uint16_t> {
    auto format(erbsland::unit::VersionComponent<tPart> value, std::format_context &ctx) const
        -> std::format_context::iterator {
        return std::formatter<uint16_t>::format(value.toRawValue(), ctx);
    }
};

template <>
struct std::formatter<erbsland::unit::ExitCode> : std::formatter<erbsland::unit::ExitCode::Value> {
    using Base = std::formatter<erbsland::unit::ExitCode::Value>;

    auto format(const erbsland::unit::ExitCode &value, std::format_context &ctx) const
        -> std::format_context::iterator {
        return Base::format(value.toRawValue(), ctx);
    }
};

template <>
struct std::formatter<erbsland::unit::Version> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::unit::Version &value, std::format_context &ctx) const -> std::format_context::iterator {
        return Base::format(value.toString(), ctx);
    }
};
