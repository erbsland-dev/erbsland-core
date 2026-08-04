// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::conf {

class Document;
/// Shared pointer for Document.
using DocumentPtr = std::shared_ptr<Document>;

}
