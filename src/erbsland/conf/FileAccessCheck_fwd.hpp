// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::conf {

class FileAccessCheck;
/// Shared pointer for FileAccessCheck.
using FileAccessCheckPtr = std::shared_ptr<FileAccessCheck>;

}
