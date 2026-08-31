// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/path/Path.hpp>
#include <erbsland/text/StringConverter.hpp>

namespace erbsland::test::pathtest {

/// Convert an Erbsland Core path to a standard string for test assertions.
[[nodiscard]] inline auto toStdString(const el::path::Path &path) -> std::string {
    return el::text::StringConverter{path.toString()}.toStdString();
}

/// Convert an Erbsland Core string to a standard string for test assertions.
[[nodiscard]] inline auto toStdString(const el::text::String &text) -> std::string {
    return el::text::StringConverter{text}.toStdString();
}

}
