// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::util::impl {

/// Internal exception used to unwind a cancelled coroutine task.
/// @tested{CoTaskTest}
class CoTaskCancelled final {};

}
