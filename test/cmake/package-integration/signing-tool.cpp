// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/path/PathWriteTextOptions.hpp>
#include <erbsland/system/EnvironmentVariables.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEditor.hpp>

#include <string_view>

auto main(int argc, char *argv[]) -> int {
    using namespace erbsland;
    using namespace text::literals;
    try {
        const auto environment = system::EnvironmentVariables{};
        const auto path = path::Path{environment.getOrThrow("PACKAGE_SIGN_LOG"_el)};
        auto line = text::StringEditor{};
        if (path.info().isRegularFile()) { line.append(path.content().readTextOrThrow()); }
        for (auto index = 1; index < argc; ++index) {
            line.append(text::StringEditor{std::string_view{argv[index]}});
            line.append(" "_el);
        }
        line.append("\n"_el);
        path.content().writeTextOrThrow(
            line, path::PathWriteTextOptions{}.setCreationMode(path::PathCreateMode::CreateOrOverwrite));
        return 0;
    } catch (...) {
        return 1;
    }
}
