// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/debug/StringDebug.hpp>

namespace demo {

/// Manual detaching makes an automatic copy-on-write step explicit.
///
/// A write operation detaches shared string storage automatically. Calling
/// `detach()` directly is therefore rare. It is mainly useful when low-level
/// code wants to make exclusive storage visible before a group of edits, or
/// when diagnostics need to show exactly where sharing ends.
void manualDetach() {
    auto fieldNote = el::String{"Fjord station: lichen sample 17, lumière froide"_el};
    auto archiveCopy = fieldNote;
    const auto yesNo = el::BooleanFormat::yesNo();

    el::io::printLine("Before detach:"_el);
    el::io::printLine("  same visible storage : "_el, yesNo, fieldNote.storageId() == archiveCopy.storageId());

    fieldNote.detach();

    el::io::printLine("After detach:"_el);
    el::io::printLine("  same visible storage : "_el, yesNo, fieldNote.storageId() == archiveCopy.storageId());

    fieldNote.replaceAll("sample 17"_el, "sample 17A"_el);
    fieldNote.append(" | checked"_el);

    el::io::printLine();
    el::io::printLine("Edited note : "_el, fieldNote);
    el::io::printLine("Archive copy: "_el, archiveCopy);

    constexpr auto debugDetails =
        el::DebugViewDetail::BackingStore | el::DebugViewDetail::Size | el::DebugViewDetail::Range;

    el::io::printLine();
    el::io::printLine("Debug view after editing:"_el);
    el::io::printLine(el::toDebugString(fieldNote, debugDetails));
    el::io::printLine(el::toDebugString(archiveCopy, debugDetails));
}

}
