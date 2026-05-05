// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StdFormatForUnit.hpp"

#include "../text/StringConverter.hpp"

using namespace erbsland::unit;

auto std::formatter<ExitCode>::format(const ExitCode &value, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<ExitCode::Value>::format(value.toRawValue(), ctx);
}

auto std::formatter<Version>::format(const Version &value, std::format_context &ctx) const
    -> std::format_context::iterator {
    return std::formatter<std::string>::format(erbsland::text::StringConverter{value.toString()}.toStdString(), ctx);
}
