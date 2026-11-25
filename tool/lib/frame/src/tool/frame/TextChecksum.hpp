// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteBlock.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/String.hpp>

namespace tool::frame {

/// Hash UTF-8 text, replacing CRLF and lone CR with LF.
/// All other bytes, including a BOM and the presence of a final terminator, remain significant.
/// @param text The exact input text, before BOM removal or other transformations.
/// @param algorithm The digest algorithm.
/// @return The binary digest.
[[nodiscard]] auto calculateTextChecksum(const el::String &text, el::HashAlgorithm algorithm) -> el::ByteBlock;

/// Hash UTF-8 bytes without decoding or removing an initial BOM.
/// @param bytes Exact text-file bytes; CRLF and lone CR are represented as LF.
/// @param algorithm The digest algorithm.
/// @return The normalized binary digest.
[[nodiscard]] auto calculateTextChecksum(const el::ByteBlock &bytes, el::HashAlgorithm algorithm) -> el::ByteBlock;

}
