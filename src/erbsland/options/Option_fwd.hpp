// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::options {

class Option;
using OptionPtr = std::shared_ptr<Option>;
using OptionWeakPtr = std::weak_ptr<Option>;

}
