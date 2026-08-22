// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::network {

class HttpStaticContentHandler;
using HttpStaticContentHandlerPtr = std::shared_ptr<HttpStaticContentHandler>;
using HttpStaticContentHandlerConstPtr = std::shared_ptr<const HttpStaticContentHandler>;

}
