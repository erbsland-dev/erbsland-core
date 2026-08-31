// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::log {

class LastErrorsLogWriter;
using LastErrorsLogWriterPtr = std::shared_ptr<LastErrorsLogWriter>;

}
