// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::conf::syntax {

class Document;
/// Shared ownership of a syntax document.
using DocumentPtr = std::shared_ptr<Document>;

}
