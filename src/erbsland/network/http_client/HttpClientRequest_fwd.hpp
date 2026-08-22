// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network {

class HttpClientRequest;
using HttpClientRequestPtr = std::shared_ptr<HttpClientRequest>;
using HttpClientRequestWeakPtr = std::weak_ptr<HttpClientRequest>;

}
