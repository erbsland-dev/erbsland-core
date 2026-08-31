// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::log::impl {

class LogManagerData;
using LogManagerDataPtr = std::shared_ptr<LogManagerData>;
using LogManagerDataWeakPtr = std::weak_ptr<LogManagerData>;

}
