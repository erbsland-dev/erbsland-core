// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DemoSynchronizer.hpp"

#include "SourceSelectors.hpp"

#include "../../cleanup/CleanupText.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::doc::impl {

using namespace el::text::literals;

DemoSynchronizer::DemoSynchronizer(
    const frame::ProjectFiles &files, el::Path executableRoot, TextReplacements replacements) :
    _files{files},
    _executor{files, DemoCapture::create(), std::move(executableRoot)},
    _replacements{std::move(replacements)} {
}

auto DemoSynchronizer::sourcePath(const el::String &name) const -> el::Path {
    const auto root = _files.checked(el::Path{"demos"_el});
    const auto relative = el::Path{name};
    const auto path = _files.checked(root / relative);
    if (name.isEmpty() || !relative.isRelative() || path == root || !path.isRelativeTo(root)) {
        throwError("Demo source must remain below demos."_el);
    }
    if (path.info().fileSize() > el::ByteCount{1024U * 1024U}) {
        throwError("Demo source exceeds 1 MiB."_el);
    }
    return path;
}

auto DemoSynchronizer::source(const el::String &name) const -> el::String {
    return _files.readText(sourcePath(name));
}

auto DemoSynchronizer::fingerprint(const el::Path &document, const el::StringMap<el::String> &options) const
    -> el::String {
    auto fingerprint = DemoFingerprint{_files};
    fingerprint.addOptions(options);
    fingerprint.addFile("source"_el, sourcePath(options.get(cSource, {})));
    addFixtureInputs(fingerprint, document, options.get(cFiles, {}));
    addExecutionInputs(fingerprint, options);
    return fingerprint.finish();
}

void DemoSynchronizer::addFixtureInputs(
    DemoFingerprint &fingerprint, const el::Path &document, const el::String &filesText) const {
    if (!filesText.isEmpty()) {
        const auto resolved = _executor.resolveFiles(document, filesText);
        for (const auto &argument : DemoExecutor::arguments(filesText)) {
            fingerprint.addFile("fixture"_el, resolved.get(el::Path{argument}.name()).value(), argument);
        }
    }
}

void DemoSynchronizer::addExecutionInputs(
    DemoFingerprint &fingerprint, const el::StringMap<el::String> &options) const {
    static const auto reExecutionKey = el::RegEx::compile(R"re(^exec(?:-[0-9]+)?$)re"_el);
    static const auto reDemoPath = el::RegEx::compile(R"re((?x)
        ^ demos/[-A-Za-z0-9_.]+    # First directory below demos
        (?: /[-A-Za-z0-9_.]+ )+    # At least one additional path component
        $                          # Complete demo path
    )re"_el);

    for (const auto &[key, command] : options) {
        if (!reExecutionKey->match(key)) {
            continue;
        }
        for (const auto &argument : DemoExecutor::arguments(command)) {
            if (reDemoPath->match(argument)) {
                fingerprint.addFile("execution-file"_el, el::Path{argument}, argument);
            }
        }
    }
}

void DemoSynchronizer::appendDirective(
    el::StringList &output, const el::String &indent, const el::String &directive, const el::StringList &content) {
    output.append(el::join(indent, directive));
    if (directive == ".. erbsland-ansi::"_el) {
        output.append(el::join(indent, "    :escape-char: ␛"_el));
    }
    output.append(""_el);
    for (const auto &text : content) {
        if (text.isEmpty()) {
            output.append(""_el);
        } else {
            output.append(el::join(indent, "    "_el, text.replacedAll("\x1b"_el, "␛"_el)));
        }
    }
    output.append(""_el);
}

auto DemoSynchronizer::generate(
    const el::Path &document,
    const el::StringMap<el::String> &options,
    const el::StringList &order,
    const el::String &indent,
    el::StringList &issues) -> el::StringList {
    auto body = el::StringList{};
    auto localIssues = el::StringList{};
    auto hash = el::String{};
    try {
        hash = fingerprint(document, options);
        appendSource(body, options, indent);
    } catch (const el::RuntimeError &error) {
        localIssues.append(error.diagnostic()->toString());
    }
    auto fixtures = el::StringMap<el::Path>{};
    try {
        fixtures = _executor.resolveFiles(document, options.get(cFiles, {}));
    } catch (const el::RuntimeError &error) {
        localIssues.append(error.diagnostic()->toString());
    }
    appendExecutions(body, options, fixtures, indent, localIssues);
    auto output = el::StringList{el::join(indent, ".. erbsland-demo::"_el)};
    appendOptions(output, options, order, indent);
    if (!hash.isEmpty() && localIssues.isEmpty()) {
        output.append(el::join(indent, "    :sha256: "_el, hash));
    }
    output.append(""_el);
    for (const auto &text : body) {
        output.append(text);
    }
    if (!localIssues.isEmpty()) {
        appendDirective(output, indent, ".. note::"_el, localIssues);
        for (const auto &issue : localIssues) {
            issues.append(el::join(document.toString(), ": "_el, issue));
        }
    }
    output.append(el::join(indent, ".. erbsland-demo-end::"_el));
    return output;
}

void DemoSynchronizer::appendSource(
    el::StringList &body, const el::StringMap<el::String> &options, const el::String &indent) const {
    const auto selection = SourceSelectors{source(options.get(cSource, {}))}.select(
        options.get("show"_el, {}), options.get("hide"_el, {}));
    if (!selection.isEmpty()) {
        auto text = selection.join("\n"_el);
        for (const auto &rule : _replacements) {
            text = rule.pattern->replaceAll(text, rule.replacement);
        }
        appendDirective(body, indent, ".. code-block:: cpp"_el, cleanup::text::lines(text));
    }
}

auto DemoSynchronizer::executionKeys(const el::StringMap<el::String> &options) -> el::StringList {
    static const auto reExecutionKey = el::RegEx::compile(R"re(^exec(?:-[0-9]+)?$)re"_el);
    auto keys = el::StringList{};
    for (const auto &[key, value] : options) {
        if (reExecutionKey->match(key)) {
            keys.append(key);
        }
    }
    auto runNumbers = el::StringMap<int>{};
    for (const auto &key : keys) {
        auto suffix = key.slice(el::StringSide::Back, el::ByteIndex{4});
        if (suffix.startsWith("-"_el)) {
            suffix = suffix.slice(el::StringSide::Back, el::ByteIndex{1});
        }
        runNumbers.set(key, suffix.toInteger<int>(1));
    }
    keys.sort([&runNumbers](const el::String &left, const el::String &right) -> bool {
        return runNumbers.get(left, 1) < runNumbers.get(right, 1);
    });
    return keys;
}

void DemoSynchronizer::appendExecutions(
    el::StringList &body,
    const el::StringMap<el::String> &options,
    const el::StringMap<el::Path> &fixtures,
    const el::String &indent,
    el::StringList &issues) {
    const auto keys = executionKeys(options);
    for (const auto &key : keys) {
        try {
            const auto expected = options.get(el::join(key, "-exit-code"_el), "0"_el).toIntegerOrThrow<int>();
            if (expected < 0) {
                throwError("Expected demo exit code must be nonnegative."_el);
            }
            const auto run = _executor.run(options.get(key, {}), fixtures, expected);
            body.append(el::join(indent, ".. erbsland-demo-run::"_el)).append(""_el);
            const auto runIndent = el::join(indent, "    "_el);
            if (options.contains("show-cmd-line"_el) || keys.count() > el::ItemCount{1}) {
                appendDirective(
                    body, runIndent, ".. code-block:: console"_el, el::StringList{el::join("$ "_el, run.command)});
            }
            if (!run.text.isEmpty()) {
                appendDirective(body, runIndent, ".. erbsland-ansi::"_el, cleanup::text::lines(run.text));
            } else {
                body.append(el::join(runIndent, "No output."_el)).append(""_el);
            }
        } catch (const el::RuntimeError &error) {
            issues.append(error.diagnostic()->toString());
        }
    }
}

void DemoSynchronizer::appendOptions(
    el::StringList &output,
    const el::StringMap<el::String> &options,
    const el::StringList &order,
    const el::String &indent) {
    for (const auto &key : order) {
        if (key == "sha256"_el) {
            continue;
        }
        auto option = el::StringEditor{el::join(indent, "    :"_el, key, ":"_el)};
        if (const auto value = options.get(key, {}); !value.isEmpty()) {
            option.append(U' ');
            option.append(value);
        }
        output.append(option);
    }
}

auto DemoSynchronizer::process(
    const el::Path &document, const el::String &text, const bool force, el::StringList &issues) -> el::String {
    static const auto reDemoStart = el::RegEx::compile(R"re((?x)
        ^ (?<indent>\s*) \.\. \s+         # Retained indentation and directive marker
        erbsland-demo:: \s* $             # Complete opening directive
    )re"_el);
    static const auto reDemoEnd = el::RegEx::compile(R"re(^\s*\.\.\s+erbsland-demo-end::)re"_el);
    const auto lines = cleanup::text::lines(text);
    auto output = el::StringList{};
    for (auto index = el::ItemIndex{0}; index.isWithin(lines.count()); ++index) {
        const auto start = reDemoStart->match(lines.getRef(index));
        if (!start) {
            if (reDemoEnd->match(lines.getRef(index))) {
                throwError("Unexpected erbsland-demo-end directive."_el);
            }
            output.append(lines.getRef(index));
            continue;
        }
        const auto first = index;
        const auto block = readBlock(lines, index);
        auto unchanged = false;
        if (!force) {
            try {
                unchanged = block.options.get("sha256"_el, {}) == fingerprint(document, block.options);
            } catch (const el::RuntimeError &) {
                unchanged = false;
            }
        }
        if (unchanged) {
            for (auto current = first; current <= index; ++current) {
                output.append(lines.getRef(current));
            }
        } else {
            for (
                const auto &generated :
                generate(document, block.options, block.order, start->content("indent"_el), issues)) {
                output.append(generated);
            }
        }
    }
    auto result = output.join("\n"_el);
    if (text.endsWith("\n"_el)) {
        result = el::join(result, "\n"_el);
    }
    return result;
}

auto DemoSynchronizer::readBlock(const el::StringList &lines, el::ItemIndex &index) -> DemoBlock {
    static const auto reDemoEndLine = el::RegEx::compile(R"re(^\s*\.\.\s+erbsland-demo-end::\s*$)re"_el);
    static const auto reDemoOption = el::RegEx::compile(R"re((?x)
        ^ \s+ :                                           # Indented option marker
        (?<name>
            files|source|sha256|show|hide|show-cmd-line   # Fixed demo options
          | exec(?:-\d+)?(?:-exit-code)?                  # Execution or expected status option
        )
        : \s* (?<value>.*) $                              # Option separator and remaining value
    )re"_el);

    auto block = DemoBlock{};
    auto inOptions = true;
    for (++index; index.isWithin(lines.count()); ++index) {
        const auto current = lines.getRef(index);
        if (reDemoEndLine->match(current)) {
            break;
        }
        if (current.trimmed().startsWith(".. erbsland-demo::"_el)) {
            throwError("Nested demo directive."_el);
        }
        if (inOptions && current.trimmed().isEmpty()) {
            continue;
        }
        const auto option = reDemoOption->match(current);
        if (inOptions && option) {
            const auto key = option->content("name"_el);
            if (block.options.contains(key)) {
                throwError("Duplicate demo option."_el);
            }
            const auto value = option->content("value"_el).trimmed();
            if ((key == "show"_el || key == "hide"_el) && value.isEmpty()) {
                throwError("Empty demo selector option."_el);
            }
            block.order.append(key);
            block.options.set(key, value);
        } else {
            if (inOptions && current.trimmed().startsWith(":"_el)) {
                throwError("Unknown demo option."_el);
            }
            inOptions = false;
        }
    }
    if (index == el::ItemIndex::end(lines.count())) {
        throwError("Missing erbsland-demo-end directive."_el);
    }
    return block;
}

auto DemoSynchronizer::migrate(const el::String &text) -> el::String {
    static const auto reLegacyOption = el::RegEx::compile(R"re((?x)
        ^ (?<indent>\s*) :        # Retained indentation and option marker
        (?<key>[^:]+) :           # Legacy option name
        \s* (?<value>.*) $        # Remaining option value
    )re"_el);

    auto output = el::StringList{};
    auto inOptions = false;
    for (const auto &current : cleanup::text::lines(text)) {
        if (current.trimmed().startsWith(".. erbsland-demo::"_el)) {
            inOptions = true;
        }
        if (inOptions && !current.trimmed().isEmpty()) {
            const auto option = reLegacyOption->match(current);
            if (option) {
                const auto key = option->content("key"_el);
                if (key == "source-sha256"_el || key == "files-sha256"_el || key == "function-blocks-sha256"_el) {
                    continue;
                }
                if (key == "function-blocks"_el) {
                    auto selectors = el::StringList{};
                    for (
                        const auto &name :
                        el::StringList::fromSplit(option->content("value"_el), el::CharSet{U' ', U'\t'})) {
                        selectors.append(el::join("f:"_el, name));
                    }
                    output.append(el::join(option->content("indent"_el), ":show: "_el, selectors.join(", "_el)));
                    continue;
                }
            } else if (!current.trimmed().startsWith(".. erbsland-demo::"_el)) {
                inOptions = false;
            }
        }
        output.append(current);
    }
    auto result = output.join("\n"_el);
    if (text.endsWith("\n"_el)) {
        result = el::join(result, "\n"_el);
    }
    return result;
}

void DemoSynchronizer::throwError(const el::String &description, const el::String &path) {
    throw el::ApplicationError{"Invalid Demo Directive"_el, description}.withSourcePath(path);
}

}
