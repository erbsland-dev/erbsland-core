// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

void writeStudyLabel(const el::TextOutputStreamPtr &output, const el::StringView &subject) {
    output->printLine("Motiv: "_el, subject);
}

/// Pass shared stream interfaces to components that produce or consume data.
/// Library-created streams use shared ownership so decorators and coroutine operations can safely keep their backing
/// stream alive. The caller that owns the complete operation remains responsible for flushing or closing the stream.
void shareStreamWithWriter() {
    const auto output = el::StringBuilderStream::create();

    // The helper depends only on text output, not on a particular destination.
    writeStudyLabel(output, "Birken im Morgennebel"_el);
    writeStudyLabel(output, "Felsen nach dem Regen"_el);

    el::io::print(output->takeString());
}

}
