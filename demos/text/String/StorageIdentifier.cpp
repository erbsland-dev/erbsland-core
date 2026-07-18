// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `storageId()` lets low-level code verify that a cached native index still
/// belongs to the same visible storage range.
///
/// This is useful when a byte index outlives the immediate operation that
/// produced it. Even when two strings contain the same decoded text, a native
/// index from one storage range must not be applied to another one.
void storageIdentifier() {
    struct CachedRange final {
        el::StorageIdentifier storageId;
        el::ByteIndex index;
        el::ByteLength length;
    };

    const auto report = el::String{"温度計A: 21℃; 気圧計B: 1012hPa; 湿度計C: 45%"_el};
    const auto token = el::String{"気圧計"_el};
    const auto cachedToken = CachedRange{report.storageId(), report.find(token), token.length()};
    const auto booleanFormat = el::BooleanFormat::yesNo();

    const auto tryUseCachedRange = [&](const el::String &label, const el::String &candidate) -> void {
        const auto sameStorage = candidate.storageId() == cachedToken.storageId;
        el::io::printLine(label, ":"_el);
        el::io::printLine("  same visible storage range: "_el, booleanFormat, sameStorage);
        if (sameStorage && !cachedToken.index.isNoIndex()) {
            el::io::printLine(
                "  cached range reads ........: "_el,
                candidate.slice(el::ByteRange{cachedToken.index, cachedToken.length}));
        } else {
            el::io::printLine("  cached range reads ........: <not used>"_el);
        }
    };

    const auto copiedReport = report.copy();
    const auto tail = report.slice(el::ByteRange{report.find(token), el::ByteLength::infinite()});

    el::io::printLine("Report: "_el, report);
    el::io::printLine("Cached token: "_el, token);
    el::io::printLine("Cached byte index: "_el, cachedToken.index);
    el::io::printLine();

    tryUseCachedRange("Original string"_el, report);
    tryUseCachedRange("Copied text"_el, copiedReport);
    tryUseCachedRange("Tail slice"_el, tail);
}

}
