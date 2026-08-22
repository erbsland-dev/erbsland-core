// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../util/List_fwd.hpp"

#include <memory>

namespace erbsland::core {

class ApplicationPartIdentifier;
using ApplicationPartIdentifierPtr = std::shared_ptr<ApplicationPartIdentifier>;
using ApplicationPartIdentifierList = util::List<ApplicationPartIdentifierPtr>;

}
