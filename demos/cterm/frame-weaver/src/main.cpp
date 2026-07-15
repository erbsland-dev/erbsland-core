// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "FrameWeaverApp.hpp"

namespace demo {

auto main(const int argc, char **argv) -> int {
    FrameWeaverApp app{argc, argv};
    return app.run();
}

}

auto main(const int argc, char **argv) -> int {
    return demo::main(argc, argv);
}
