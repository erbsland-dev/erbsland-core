// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/MakeOneNamespace.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathCollisionMode.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringEditor.hpp>

namespace demo {

/// Append a quoted and escaped ELCL text assignment.
/// @param output The destination text editor.
/// @param name The human-readable ELCL name.
/// @param value The text value to escape and quote.
/// @notest{Compiled and exercised as part of the Password Handler demo.}
void appendElclText(el::StringEditor &output, const el::String &name, const el::String &value);

/// Write a complete storage document through a user-only temporary file.
/// @param path The final destination path.
/// @param text The complete UTF-8 ELCL document.
/// @param collisionMode The collision policy for the final move.
/// @notest{Compiled and exercised as part of the Password Handler demo.}
void writeStorageFile(const el::Path &path, const el::String &text, el::PathCollisionMode collisionMode);

}
