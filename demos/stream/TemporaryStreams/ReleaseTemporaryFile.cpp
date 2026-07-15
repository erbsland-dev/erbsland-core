// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Call `release()` when the generated file must outlive the temporary stream.
/// Releasing transfers cleanup responsibility to the caller; close the stream and remove the returned path explicitly.
void releaseTemporaryFile() {
    const auto output = el::Path::systemTempDirectoryOrThrow().operations().openTempTextOutputStreamOrThrow();
    output->writeLine("Καταγραφή αρκούδας"_el);
    const auto retainedPath = output->release();
    output->close();

    el::io::printLine("File was retained: "_el, retainedPath.info().exists());
    retainedPath.operations().removeOrThrow();
    el::io::printLine("Caller removed the file: "_el, !retainedPath.info().exists());
}

}
