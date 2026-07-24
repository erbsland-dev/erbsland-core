// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "InputStreamFactory.hpp"

#include "BufferedByteInputStream.hpp"
#include "EncodedTextInputStream.hpp"

#include "../StreamError.hpp"

#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::stream::impl {

using namespace text::literals;

auto createBufferedByteInputStream(NativeByteStreamPtr nativeStream, const InputStreamSettings settings)
    -> ByteInputStreamPtr {
    return std::make_shared<BufferedByteInputStream>(std::move(nativeStream), settings);
}

auto createEncodedTextInputStream(
    ByteInputStreamPtr byteInputStream,
    const text::StringEncoding encoding,
    const text::StringBomMode bomMode,
    const text::EncodingMode mode) -> TextInputStreamPtr {
    if (!byteInputStream) {
        throw StreamError{StreamErrorContext{
            "Failed to create the text input stream."_el, "The required byte input stream was not provided."_el}};
    }
    return std::make_shared<EncodedTextInputStream>(std::move(byteInputStream), encoding, bomMode, mode);
}

}
