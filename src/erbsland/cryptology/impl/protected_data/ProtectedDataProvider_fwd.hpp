// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::cryptology::impl {

class ProtectedDataProvider;
using ProtectedDataProviderPtr = std::unique_ptr<ProtectedDataProvider>;

}
