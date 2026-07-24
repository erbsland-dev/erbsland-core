// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Encoding failures are text-domain errors rather than stream failures.
/// Configure strict decoding when malformed marine-observation text must be rejected and handle `EncodingError`
/// separately from native I/O failures.
void handleEncodingError() {
    const auto directory = createStreamDemoDirectory("codificación"_el);
    const auto path = directory->path() / "muestra.txt"_el;
    path.content().writeDataOrThrow(el::ByteBlock({0xf0U, 0x28U, 0x8cU, 0x28U}));
    auto options = el::PathReadTextOptions{el::StringEncoding::Utf8};
    options.setEncodingMode(el::EncodingMode::Strict);

    try {
        static_cast<void>(path.content().openTextInputStream(options)->readAll());
    } catch (const el::EncodingError &) {
        el::io::printLine("The malformed UTF-8 sequence was rejected."_el);
    }
}

}
