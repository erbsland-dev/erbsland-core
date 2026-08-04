// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::stream::impl {

class NativeOutputStream;
using NativeOutputStreamPtr = std::shared_ptr<NativeOutputStream>;

}
