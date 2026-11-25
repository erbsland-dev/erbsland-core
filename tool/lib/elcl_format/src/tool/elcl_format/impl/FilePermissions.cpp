// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FilePermissions.hpp"

#include <erbsland/ApplicationError.hpp>

#include <filesystem>

namespace tool::elcl_format::impl {

using namespace el::text::literals;
void copyFilePermissions(const el::Path &original, const el::Path &temporary) {
    const auto source = original.toStdPath();
    const auto target = temporary.toStdPath();
    auto error = std::error_code{};
    const auto status = std::filesystem::status(source, error);
    if (!error) {
        std::filesystem::permissions(target, status.permissions(), error);
    }
    if (error) {
        throw el::ApplicationError{"Could Not Preserve File Permissions"_el, original.toString()};
    }
}

}
