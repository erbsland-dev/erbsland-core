// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <concepts>
#include <functional>

namespace erbsland::util {

template <typename tKey, typename tHash = std::hash<tKey>, typename tEqual = std::equal_to<tKey>, typename tSelf = void>
    requires std::default_initializable<tKey> && std::copyable<tKey>
class HashSet;

}
