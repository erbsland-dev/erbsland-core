// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::conf::impl {

/// Private tag to mark constructors for internal use.
/// This is a shared instance of a private tag to mark constructors that are an implementation detail, even if
/// they are declared public. Usually, a private tag would be declared in a private section of the class to
/// actually prevent the constructor from being called from outside the class.
/// Declaring this tag in the `impl` namespace is a more relaxed approach. It does not technically prevent a
/// user from calling constructors meant for internal use. It is merely a strong and clear indicator that an
/// internal API is used that can change at any time.
/// @warning Do not use this class directly in any user code.
class PrivateTag {};

}
