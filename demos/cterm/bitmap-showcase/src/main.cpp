// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "BitmapShowcaseApp.hpp"

namespace demo {

auto main(const int argc, char **argv) -> int {
    BitmapShowcaseApp app{argc, argv};
    app.info().setApplicationName("Bitmap Showcase"_el);
    app.info().setApplicationVersion(el::Version{1, 0, 0});
    return app.run();
}

}

auto main(const int argc, char **argv) -> int {
    return demo::main(argc, argv);
}
