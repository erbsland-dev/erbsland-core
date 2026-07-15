// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "PositioningDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("CheckPositioning"_el, checkPositioning);
    app.registerDemo("CreateSparseRecord"_el, createSparseRecord);
    app.registerDemo("HandlePositionTimeout"_el, handlePositionTimeout);
    app.registerDemo("MoveRelativeToEnd"_el, moveRelativeToEnd);
    app.registerDemo("PositionEncodedText"_el, positionEncodedText);
    app.registerDemo("ReadRecordAtOffset"_el, readRecordAtOffset);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
