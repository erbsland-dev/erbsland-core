// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "BaseNDemos.hpp"

using namespace demo;

auto main(const int argc, char *argv[]) -> int {
    auto app = DemoApplication{argc, argv};
    app.registerDemo("RoundTripSoundTag"_el, roundTripSoundTag);
    app.registerDemo("EncodeSoundTag"_el, encodeSoundTag);
    app.registerDemo("DecodeSoundTag"_el, decodeSoundTag);
    app.registerDemo("ChooseAlphabet"_el, chooseAlphabet);
    app.registerDemo("ChoosePadding"_el, choosePadding);
    app.registerDemo("ChooseWhitespace"_el, chooseWhitespace);
    app.registerDemo("ChooseFlags"_el, chooseFlags);
    app.registerDemo("DecodePaddingPolicy"_el, decodePaddingPolicy);
    app.registerDemo("WrapLines"_el, wrapLines);
    return app.run();
}
