// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../HashAlgorithm.hpp"

#include <memory>

namespace erbsland::cryptology::impl {

class HashWorker;

/// Create a built-in hash worker for an algorithm.
/// @param algorithm The algorithm to implement.
/// @return A new worker in its initial state.
/// @tested{HasherTest Sha3ValidationTest}
auto createHashWorker(HashAlgorithm algorithm) -> std::shared_ptr<HashWorker>;

}
