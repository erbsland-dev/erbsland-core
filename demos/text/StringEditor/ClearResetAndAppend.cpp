// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

void printDraftState(const el::String &label, const el::StringEditor &editor);

/// `clear()` removes the text while keeping the allocated storage available for
/// reuse. `reset()` returns the string to its initial empty state and releases
/// the reserved storage.
///
/// This pattern is useful when one local editor is reused for several editing
/// passes. Reserve once before predictable growth; do not reserve before each
/// append operation.
void clearResetAndAppend() {
    auto draft = el::StringEditor{"ridge log"_el};
    draft.reserve(el::ByteLength{80U});

    // Append text and repeated code points directly to the editable string.
    draft.append(": "_el).append("lichen"_el).append(", "_el).append("moss"_el);
    draft.append(U'·', el::CpLength{3U});
    printDraftState("Draft"_el, draft);

    // Clear keeps the reserved storage for the next edit pass.
    draft.clear();
    draft.append("reused after clear"_el);
    printDraftState("After clear"_el, draft);

    // Reset releases the storage and starts from the default empty state.
    draft.reset();
    draft.append("fresh after reset"_el);
    printDraftState("After reset"_el, draft);
}

void printDraftState(const el::String &label, const el::StringEditor &editor) {
    el::io::printLine(label, ": "_el, editor);
    el::io::printLine("  bytes="_el, editor.length().toSizeT(), " capacity="_el, editor.capacity().toSizeT());
}

}
