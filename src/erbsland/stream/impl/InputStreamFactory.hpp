// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "NativeByteStream.hpp"

#include "../ByteInputStream_fwd.hpp"
#include "../InputStreamSettings.hpp"
#include "../TextInputStream_fwd.hpp"

#include "../../text/EncodingMode.hpp"
#include "../../text/StringBomMode.hpp"
#include "../../text/StringEncoding.hpp"

namespace erbsland::stream::impl {

/// Create one immutable ordinary or sensitive buffered byte pipeline from the settings.
/// @tested{BufferedStreamTest SensitiveBufferedStreamTest}
[[nodiscard]] auto createBufferedByteInputStream(NativeByteStreamPtr nativeStream, InputStreamSettings settings)
    -> ByteInputStreamPtr;

/// Create one immutable ordinary or sensitive encoded text pipeline from its byte source settings.
/// @tested{EncodedTextStreamTest SensitiveEncodedTextStreamTest}
[[nodiscard]] auto createEncodedTextInputStream(
    ByteInputStreamPtr byteInputStream,
    text::StringEncoding encoding,
    text::StringBomMode bomMode = text::StringBomMode::Automatic,
    text::EncodingMode mode = text::EncodingMode::Tolerant) -> TextInputStreamPtr;

}
