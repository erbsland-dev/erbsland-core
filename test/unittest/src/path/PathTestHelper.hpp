// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/path/Path.hpp>
#include <erbsland/text/StringConverter.hpp>

namespace erbsland::test::pathtest {

[[nodiscard]] inline auto toStdString(const el::path::Path &path) -> std::string {
    return el::text::StringConverter{path.toString()}.toStdString();
}

[[nodiscard]] inline auto toStdString(const el::text::StringView &text) -> std::string {
    return el::text::StringConverter{text}.toStdString();
}

}
