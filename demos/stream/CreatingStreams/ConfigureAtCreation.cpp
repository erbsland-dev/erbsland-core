// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Configure a stream before opening it.
/// File options combine filesystem policy with immutable stream settings, so every user of the resulting stream sees
/// the same timeout, buffering, creation mode, encoding, and byte-order-mark behavior.
void configureAtCreation() {
    auto streamSettings = el::OutputStreamSettings{};
    streamSettings.setTimeout(el::TimeDelta::milliseconds(250))
        .setBuffering(el::StreamBuffering::Throughput)
        .setBackBufferLimit(el::ByteLength{128U * 1024U});

    auto options = el::PathWriteTextOptions{el::StringEncoding::Utf16LittleEndian};
    options.setCreationMode(el::PathCreateMode::CreateOrOverwrite)
        .setBomMode(el::StringBomMode::Require)
        .setStreamSettings(streamSettings);

    const auto directory = createStreamDemoDirectory("konfiguration"_el);
    const auto output = (directory->path() / "farbenlehre.txt"_el).content().openTextOutputStream(options);
    output->writeLine("Ultramarin neben warmem Ocker"_el);
    output->close();

    el::io::printLine("Encoding configured: UTF-16 LE"_el);
    el::io::printLine("Back-buffer limit: "_el, streamSettings.backBufferLimit().toSizeT(), " bytes"_el);
}

}
