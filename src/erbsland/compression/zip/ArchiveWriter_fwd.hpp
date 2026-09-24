// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::compression::zip {

class ArchiveWriter;
using ArchiveWriterPtr = std::shared_ptr<ArchiveWriter>;
using ArchiveWriterWeakPtr = std::weak_ptr<ArchiveWriter>;

}
