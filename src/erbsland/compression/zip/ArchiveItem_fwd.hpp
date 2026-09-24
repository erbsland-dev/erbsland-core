// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../util/List_fwd.hpp"

#include <memory>

namespace erbsland::compression::zip {

class ArchiveItem;
using ArchiveItemPtr = std::shared_ptr<ArchiveItem>;
using ArchiveItemWeakPtr = std::weak_ptr<ArchiveItem>;
using ArchiveItemList = util::List<ArchiveItemPtr>;

}
