// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "GitStatusReport.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteIndex.hpp>
#include <erbsland/ByteRange.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/StringSide.hpp>

namespace demo {

using namespace el::text::literals;

void GitStatusReport::append(const el::String &chunk) {
    // git separates filenames using a zero byte when using the '-z' option.
    auto start = el::ByteIndex{};
    while (start.isWithin(chunk.length())) {
        const auto end = chunk.findFirstOf(el::CharSet{U'\0'}, start);
        if (end.isNoIndex()) {
            _pending.append(chunk.slice(el::ByteRange::fromBegin(start)));
            return;
        }
        _pending.append(chunk.slice(el::ByteRange{start, end}));
        consumeRecord(el::String{_pending});
        _pending.clear();
        start = end.incremented();
    }
}

void GitStatusReport::finish() const {
    if (!_pending.isEmpty() || _needsOriginalPath) {
        throw el::ApplicationError{"Incomplete Git status output."_el};
    }
}

void GitStatusReport::consumeRecord(const el::String &record) {
    if (_needsOriginalPath) {
        if (record.isEmpty()) {
            throw el::ApplicationError{"Missing original path in Git status output."_el};
        }
        _needsOriginalPath = false;
        return;
    }
    if (record.length() < el::ByteCount{4} || record[el::ByteIndex{2}] != U' ') {
        throw el::ApplicationError{"Invalid Git status record."_el};
    }
    const auto status = record.slice(el::StringSide::Front, el::ByteCount{2});
    const auto directory = directoryOf(record.slice(el::ByteRange::fromBegin(el::ByteIndex{3})));
    auto counts = _directories.get(directory, ChangeCounts{});
    if (status == "??"_el) {
        ++counts.untracked;
    } else if (status == "!!"_el) {
        return; // Ignored entries are not changes.
    } else if (isConflict(status)) {
        ++counts.conflicted;
    } else {
        if (!status.containsOnly(el::CharSet{" MTADRC"_el}) || status == "  "_el) {
            throw el::ApplicationError{"Unknown Git status code."_el};
        }
        if (status[el::ByteIndex{0}] != U' ') {
            ++counts.staged;
        }
        if (status[el::ByteIndex{1}] != U' ') {
            ++counts.unstaged;
        }
        _needsOriginalPath = status.containsOneOf(el::CharSet{U'R', U'C'});
    }
    _directories.set(directory, counts);
}

auto GitStatusReport::directoryOf(const el::String &path) -> el::String {
    const auto separator = path.find("/"_el);
    if (separator.isNoIndex()) {
        return "(root)"_el;
    }
    return path.slice(el::StringSide::Front, separator.incremented());
}

auto GitStatusReport::isConflict(const el::String &status) -> bool {
    return status == "DD"_el || status == "AU"_el || status == "UD"_el || status == "UA"_el || status == "DU"_el ||
        status == "AA"_el || status == "UU"_el;
}

}
