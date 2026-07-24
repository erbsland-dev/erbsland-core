// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "TextGalleryApp.hpp"

using namespace demo;

auto main(const int argc, char **argv) -> int {
    TextGalleryApp app{argc, argv};
    return app.run();
}
