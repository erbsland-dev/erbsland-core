// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Check `supportsPositioning()` before using byte positions.
/// Regular files normally support it, while append-only files, pipes, terminals, and standard-stream proxies do not.
void checkPositioning() {
    const auto directory = createStreamDemoDirectory("野生动物"_el);
    const auto path = directory->path() / "observations.bin"_el;
    const auto regular = path.content().openByteOutputStream();
    el::io::printLine("Regular file supports positioning: "_el, regular->supportsPositioning());
    regular->close();

    auto appendOptions = el::PathWriteDataOptions{};
    appendOptions.setCreationMode(el::PathCreateMode::CreateOrAppend);
    const auto append = path.content().openByteOutputStream(appendOptions);
    el::io::printLine("Append stream supports positioning: "_el, append->supportsPositioning());
    append->close();
}

}
