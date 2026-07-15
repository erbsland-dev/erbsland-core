// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ByteInputAndOutputDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ProcessPayloadInChunks"_el, processPayloadInChunks);
    app.registerDemo("ReadAndWriteIntegers"_el, readAndWriteIntegers);
    app.registerDemo("ReadBinaryHeader"_el, readBinaryHeader);
    app.registerDemo("ResumeExactRead"_el, resumeExactRead);
    app.registerDemo("WriteAtomicRecord"_el, writeAtomicRecord);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
