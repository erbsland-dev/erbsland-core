// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureRange.hpp"
#include "ErrorCategory.hpp"
#include "Feature.hpp"
#include "Features.hpp"
#include "Flag.hpp"
#include "Flags.hpp"
#include "RegExError.hpp"
#include "Settings.hpp"

#include "impl/diagnostics/Argument.hpp"
#include "impl/diagnostics/AssemblerToken.hpp"
#include "impl/diagnostics/DataSection.hpp"
#include "impl/diagnostics/OperationData.hpp"
#include "impl/diagnostics/OperationModifier.hpp"

#include "../text/StdFormat.hpp"

#include <format>

template <>
struct std::formatter<erbsland::re::CaptureRange> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::CaptureRange &value, std::format_context &ctx) const {
        return Base::format(value.toString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::re::ErrorCategory> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::ErrorCategory value, std::format_context &ctx) const {
        return Base::format(erbsland::re::toString(value), ctx);
    }
};

template <>
struct std::formatter<erbsland::re::Feature> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::Feature value, std::format_context &ctx) const {
        return Base::format(erbsland::re::toString(value), ctx);
    }
};

template <>
struct std::formatter<erbsland::re::Features> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::Features value, std::format_context &ctx) const {
        return Base::format(value.toString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::re::Flag> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::Flag value, std::format_context &ctx) const {
        return Base::format(erbsland::re::toString(value), ctx);
    }
};

template <>
struct std::formatter<erbsland::re::Flags> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::Flags value, std::format_context &ctx) const {
        return Base::format(value.toString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::re::RegExError> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::RegExError &value, std::format_context &ctx) const {
        return Base::format(value.toString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::re::Settings> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::Settings &value, std::format_context &ctx) const {
        return Base::format(value.toString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::re::impl::ArgumentKind> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::impl::ArgumentKind value, std::format_context &ctx) const {
        return Base::format(erbsland::re::impl::toString(value), ctx);
    }
};

template <>
struct std::formatter<erbsland::re::impl::ArgumentType> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::impl::ArgumentType value, std::format_context &ctx) const {
        return Base::format(erbsland::re::impl::toString(value), ctx);
    }
};

template <>
struct std::formatter<erbsland::re::impl::AssemblerToken> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::impl::AssemblerToken &value, std::format_context &ctx) const {
        return Base::format(value.toString(), ctx);
    }
};

template <>
struct std::formatter<erbsland::re::impl::DataSection> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::impl::DataSection value, std::format_context &ctx) const {
        return Base::format(erbsland::re::impl::toString(value), ctx);
    }
};

template <>
struct std::formatter<erbsland::re::impl::Operation> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::impl::Operation value, std::format_context &ctx) const {
        return Base::format(erbsland::re::impl::toString(value), ctx);
    }
};

template <>
struct std::formatter<erbsland::re::impl::OperationModifier> : std::formatter<erbsland::text::String> {
    using Base = std::formatter<erbsland::text::String>;

    auto format(const erbsland::re::impl::OperationModifier value, std::format_context &ctx) const {
        return Base::format(erbsland::re::impl::toString(value), ctx);
    }
};
