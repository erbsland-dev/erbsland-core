// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/CompressionFormat.hpp>

#include <type_traits>

// A leaf header must select the namespace view when it is the only Core include.
static_assert(std::is_same_v<el::CompressionFormat, erbsland::compression::CompressionFormat>);
