// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::conf {

class FileSourceResolver;
/// Shared pointer for FileSourceResolver.
using FileSourceResolverPtr = std::shared_ptr<FileSourceResolver>;

}
