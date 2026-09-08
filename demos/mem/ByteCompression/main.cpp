// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ByteCompressionDemos.hpp"

#include <DemoCommon.hpp>

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("CompressOneShot"_el, compressOneShot);
    app.registerDemo("CompressStreaming"_el, compressStreaming);
    app.registerDemo("DecompressOneShot"_el, decompressOneShot);
    app.registerDemo("DecompressStreaming"_el, decompressStreaming);
    return app.run();
}
