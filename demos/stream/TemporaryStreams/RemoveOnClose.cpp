// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// A successful temporary-stream close removes the file synchronously when `removeOnClose()` is enabled.
/// This gives the caller a precise point after which the path no longer exists.
void removeOnClose() {
    const auto output = el::Path::systemTempDirectoryOrThrow().operations().openTempTextOutputStreamOrThrow();
    const auto path = output->path();
    output->writeLine("Λύγκας"_el);
    el::io::printLine("Existed before close: "_el, path.info().exists());
    output->close();
    el::io::printLine("Exists after close: "_el, path.info().exists());
}

}
