// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CppCleanup.hpp"

#include "CleanupText.hpp"
#include "CppSpacing.hpp"

#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>

#include <algorithm>
#include <vector>

namespace tool::cleanup {

using namespace text;
using namespace el::text::literals;

CppCleanup::CppCleanup(const IncludeIndex &index) : _index{index} {
}

auto CppCleanup::leadingEnd(const el::StringList &lines) -> el::ItemIndex {
    auto end = el::ItemIndex::zero();
    for (const auto &line : lines) {
        const auto value = line.trimmed();
        if (value.startsWith("///"_el) || value == "// clang-format off"_el) {
            break; // Documentation belongs to the declaration; manual-format blocks stay intact.
        }
        if (!value.isEmpty() && !value.startsWith("//"_el) && value != "#pragma once"_el &&
            !value.startsWith("#include "_el) && !value.startsWith("#include\t"_el)) {
            break;
        }
        if (value.endsWith("\\"_el)) {
            break; // A continued directive belongs to the untouched suffix.
        }
        ++end;
    }
    return end;
}

auto CppCleanup::group(const el::String &name, const bool global) -> int {
    if (!global) {
        if (name.startsWith("../../"_el)) {
            return 4;
        }
        if (name.startsWith("../"_el)) {
            return 3;
        }
        if (name.contains("/"_el)) {
            return 2;
        }
        return 1;
    }
    if (name.endsWith(".h"_el) || name.endsWith(".hpp"_el)) {
        return 5;
    }
    if (!name.contains("."_el)) {
        return 6;
    }
    return 7;
}

auto CppCleanup::isMain(const el::Path &path, const el::String &name, const bool global) -> bool {
    if (global || path.suffix() != ".cpp"_el || name.contains("/"_el) || !name.endsWith(".hpp"_el)) {
        return false;
    }
    const auto stem = el::Path{name}.stem();
    return path.stem() == stem || path.stem().startsWith(el::join(stem, "_"_el));
}

void CppCleanup::sortRun(
    el::StringList &output, const el::StringList &run, const el::Path &path, const SourceTree &tree) const {
    // Priority, sortable spelling, and complete line. Different comments never deduplicate.
    auto entries = std::vector<Include>{};
    entries.reserve(run.count().toSizeT());
    static const auto expression = el::RegEx::compile(R"/(^[ \t]*#[ \t]*include[ \t]+([<"])([^>"]+)([>"])(.*)$)/"_el);
    for (const auto &line : run) {
        if (line.trimmed().isEmpty()) {
            continue;
        }
        const auto match = expression->match(line);
        if (!match) {
            fail("Invalid literal include."_el, path.toString());
        }
        auto global = match->content(1) == "<"_el;
        if ((global && match->content(3) != ">"_el) || (!global && match->content(3) != "\""_el)) {
            fail("Mismatched include delimiters."_el, path.toString());
        }
        const auto name = _index.resolve(path, match->content(2), global, tree);
        auto priority = group(name, global);
        if (isMain(path, name, global)) {
            priority = 0;
        }
        auto rendered = el::StringEditor{"#include "_el};
        if (global) {
            rendered.append("<"_el).append(name).append(">"_el);
        } else {
            rendered.append("\""_el).append(name).append("\""_el);
        }
        rendered.append(match->content(4));
        entries.push_back({priority, name, el::String{rendered}});
    }
    std::stable_sort(entries.begin(), entries.end(), [](const auto &left, const auto &right) -> bool {
        if (left.priority != right.priority) {
            return left.priority < right.priority;
        }
        return left.name.compare(right.name, &el::Char::compareCaseFolded) < 0;
    });
    auto seen = el::StringHashSet{};
    auto previousGroup = -1;
    for (const auto &[priority, name, line] : entries) {
        if (!seen.tryInsert(line)) {
            continue;
        }
        if (previousGroup != -1 && previousGroup != priority) {
            output.append(el::String{});
        }
        output.append(line);
        previousGroup = priority;
    }
}

auto CppCleanup::copyHeader(el::StringList &output, const el::StringList &lines, const el::ItemIndex end)
    -> el::ItemIndex {
    auto current = el::ItemIndex::zero();
    // Preserve the initial comment header, enforcing no gap before pragma once.
    while (current < end && lines.getRef(current).trimmed().startsWith("//"_el)) {
        output.append(lines.getRef(current++));
    }
    const auto headerEnd = current;
    while (current < end && lines.getRef(current).trimmed().isEmpty()) {
        ++current;
    }
    if (current < end && lines.getRef(current).trimmed() == "#pragma once"_el) {
        output.append("#pragma once"_el);
        output.append(el::String{});
        ++current;
    } else {
        output.append(lines.slice(el::ItemRange{headerEnd, current}));
    }
    return current;
}

void CppCleanup::sortIncludes(
    el::StringList &output,
    const el::StringList &lines,
    el::ItemIndex current,
    const el::ItemIndex end,
    const el::Path &path,
    const SourceTree &tree) const {
    static const auto literal = el::RegEx::compile(R"/(^[ \t]*#[ \t]*include[ \t]+[<"].*$)/"_el);
    while (current < end) {
        auto run = el::StringList{};
        while (current < end && (lines.getRef(current).trimmed().isEmpty() || literal->match(lines.getRef(current)))) {
            run.append(lines.getRef(current++));
        }
        if (!run.isEmpty()) {
            // Keep blank lines separating standalone comments from include runs. Generator
            // notices and include-group explanations must retain their original boundaries.
            if (run.first().trimmed().isEmpty() && !output.isEmpty() && !output.last().isEmpty()) {
                output.append(el::String{});
            }
            sortRun(output, run, path, tree);
            if (current < end && run.last().trimmed().isEmpty() && !output.isEmpty() && !output.last().isEmpty()) {
                output.append(el::String{});
            }
        }
        if (current < end) {
            // Standalone comments and macro includes are ordering barriers.
            output.append(lines.getRef(current++));
        }
    }
}

void CppCleanup::compactMainHeader(el::StringList &output, const el::Path &path) {
    // Keep the established compact copyright/own-header layout for ordinary source files.
    for (auto line = el::ItemIndex{1}; line.incremented().isWithin(output.count()); ++line) {
        if (output.getRef(line).isEmpty() &&
            output.getRef(line.decremented()).startsWith("// SPDX-License-Identifier:"_el)) {
            static const auto mainInclude = el::RegEx::compile(R"/(^#include "([^"]+)".*$)/"_el);
            const auto match = mainInclude->match(output.getRef(line.incremented()));
            if (match && isMain(path, match->content(1), false)) {
                output.remove(line);
            }
            break;
        }
    }
}

auto CppCleanup::apply(const el::Path &path, const el::String &source, const SourceTree &tree) const -> el::String {
    const auto lines = text::lines(source);
    const auto end = leadingEnd(lines);
    auto output = el::StringList{};
    output.reserve(lines.count());
    const auto current = copyHeader(output, lines, end);
    sortIncludes(output, lines, current, end, path, tree);
    if (!output.isEmpty() && !output.last().isEmpty() && !output.last().trimmed().startsWith("//"_el)) {
        output.append(el::String{});
    }
    output.append(lines.slice(el::ItemRange{end, el::ItemIndex::end(lines.count())}));
    compactMainHeader(output, path);
    while (!output.isEmpty() && output.last().trimmed().isEmpty()) {
        output.removeLast();
    }
    return CppSpacing::apply(join(output), path.suffix() == ".cpp"_el);
}

auto CppCleanup::codeFingerprint(const el::String &text) -> el::String {
    auto lines = text::lines(text);
    const auto end = leadingEnd(lines);
    lines.remove(el::ItemRange{el::ItemIndex::zero(), end});
    return digest(join(lines));
}

}
