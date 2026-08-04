// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Encoded and temporary stream wrappers forward locally created errors to their backing stream.
/// As a result, a text-level operation can still report the path associated with its file-backed byte source.
void preserveWrappedContext() {
    const auto directory = createStreamDemoDirectory("océano"_el);
    const auto path = directory->path() / "arrecife.txt"_el;
    const auto output = path.content().openTextOutputStream();
    output->write("coral"_el);
    output->close();
    try {
        [[maybe_unused]] const auto status = output->write("pez"_el);
        // note: real code must handle the returned status.
    } catch (const el::StreamError &error) {
        el::io::printLine("Ruta conservada: "_el, error.path().endsWith("arrecife.txt"_el));
    }
}

}
