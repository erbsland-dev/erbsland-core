// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network::impl {

class Http1Transaction;
using Http1TransactionPtr = std::shared_ptr<Http1Transaction>;
using Http1TransactionWeakPtr = std::weak_ptr<Http1Transaction>;

}
