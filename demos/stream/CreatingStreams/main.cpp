// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CreatingStreamsDemos.hpp"

#include <DemoCommon.hpp>

namespace demo {

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("ChooseStreamFamily"_el, chooseStreamFamily);
    app.registerDemo("ConfigureAtCreation"_el, configureAtCreation);
    app.registerDemo("OpenFileStreams"_el, openFileStreams);
    app.registerDemo("ShareStreamWithWriter"_el, shareStreamWithWriter);
    return app.run();
}

}

auto main(const int argc, char *argv[]) -> int {
    return demo::main(argc, argv);
}
