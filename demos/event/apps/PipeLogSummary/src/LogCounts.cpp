// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LogCounts.hpp"

#include <erbsland/StandardStreams.hpp>

namespace demo {

using namespace el::text::literals;

void LogCounts::consume(const el::ByteBlock &bytes) {
    for (const auto byte : bytes.span()) {
        if (byte == el::Byte{'\n'}) {
            endLine();
        } else if (_prefix.length() < el::ByteCount{6U}) {
            // Only ASCII severity labels are meaningful. The rest of a line can be arbitrary bytes.
            _prefix.append(el::Char{static_cast<char32_t>(byte.toUInt8())});
        }
    }
    if (bytes.isEmpty() && !_prefix.isEmpty()) {
        endLine();
    }
}

void LogCounts::endLine() {
    const auto prefix = el::String{_prefix};
    if (prefix.startsWith("INFO "_el)) {
        ++_info;
    } else if (prefix.startsWith("WARN "_el)) {
        ++_warn;
    } else if (prefix.startsWith("ERROR "_el)) {
        ++_error;
    } else {
        ++_other;
    }
    _prefix.clear();
}

void LogCounts::report(const el::String &label) const {
    el::io::printLine(label, "  INFO="_el, _info, " WARN="_el, _warn, " ERROR="_el, _error, " OTHER="_el, _other);
    el::stdOut()->flush();
}

}
