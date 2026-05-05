// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::options {

class OptionSet;
using OptionSetPtr = std::shared_ptr<OptionSet>;
using OptionSetWeakPtr = std::weak_ptr<OptionSet>;

}
