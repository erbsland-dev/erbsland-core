// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DemoExecutor.hpp"

#include "../../cleanup/CleanupText.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/cterm/TerminalEmulation.hpp>
#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/TempDirectory.hpp>
#include <erbsland/U8StringConstIterator.hpp>

#include <algorithm>

namespace tool::doc::impl {

using namespace el::text::literals;

DemoExecutor::DemoExecutor(
    const frame::ProjectFiles &files, std::unique_ptr<DemoCapture> capture, el::Path executableRoot) :
    _files{files}, _executableRoot{std::move(executableRoot)}, _capture{std::move(capture)} {
}

auto DemoExecutor::arguments(const el::String &command) -> el::StringList {
    auto result = el::StringList{};
    auto token = el::StringEditor{};
    auto quote = el::Char{};
    auto escaped = false;
    auto present = false;
    for (const auto character : command) {
        if (escaped) {
            token.append(character);
            escaped = false;
            present = true;
        } else if (character == U'\\' && quote != U'\'') {
            escaped = true;
            present = true;
        } else if (quote != U'\0') {
            if (character == quote) {
                quote = el::Char{};
            } else {
                token.append(character);
            }
        } else if (character == U'"' || character == U'\'') {
            quote = character;
            present = true;
        } else if (character.isAsciiWhitespace()) {
            if (present) {
                result.append(token);
                token.clear();
                present = false;
            }
        } else {
            token.append(character);
            present = true;
        }
    }
    if (escaped || quote != U'\0') {
        throwError("Unclosed quoting in demo arguments."_el);
    }
    if (present) {
        result.append(token);
    }
    return result;
}

auto DemoExecutor::resolveFiles(const el::Path &document, const el::String &value) const -> el::StringMap<el::Path> {
    auto files = el::StringMap<el::Path>{};
    for (const auto &argument : arguments(value)) {
        const auto relative = el::Path{argument};
        if (!relative.isRelative() || relative == el::Path{"."_el}) {
            throwError("Demo file must be relative to its document."_el);
        }
        const auto path = _files.checked(document.parent() / relative);
        if (!path.info().isRegularFile() || files.contains(path.name())) {
            throwError("Demo files must exist and have unique basenames."_el);
        }
        if (path.info().fileSize() > el::ByteCount{1024U * 1024U}) {
            throwError("Demo input exceeds 1 MiB."_el);
        }
        files.set(path.name(), path);
    }
    return files;
}

auto DemoExecutor::run(const el::String &command, const el::StringMap<el::Path> &fixtures, const int expectedExitCode)
    -> DemoRunOutput {
    const auto parsed = arguments(command);
    const auto executable = resolveExecutable(parsed);
    auto temporary = _files.projectRoot().operations().createTempDirectoryOrThrow();
    const auto expanded = expandArguments(parsed, fixtures, temporary->path());
    auto raw = _capture->run(executable, expanded.values, _files.projectRoot(), expectedExitCode, expanded.width);
    return {renderOutput(std::move(raw), expanded.redactions), expanded.displayed.join(" "_el)};
}

auto DemoExecutor::resolveExecutable(const el::StringList &parsed) const -> el::Path {
    static const auto reRelativeCommand = el::RegEx::compile(R"re(^[-A-Za-z0-9_]+(?:/[-A-Za-z0-9_]+)*$)re"_el);
    if (parsed.isEmpty() || !reRelativeCommand->match(parsed.first())) {
        throwError("Invalid demo executable name."_el);
    }
    const auto root = _executableRoot.toAbsoluteOrThrow(_files.projectRoot()).resolveOrThrow();
    const auto candidates =
        el::EnvironmentVariables{}.executableCandidatesOrThrow((root / parsed.first()).toNativeString());
    const auto executable = frame::ProjectFiles{root}.checked(candidates.first());
    if (!executable.info().isRegularFile()) {
        throwError("Demo executable is missing; build the demo first."_el, executable.toString());
    }
    return executable;
}

auto DemoExecutor::createPlaceholder(
    const el::String &kind, const el::String &content, const el::Path &temporary, int &fileIndex) const -> el::String {
    static const auto reFixtureName = el::RegEx::compile(R"re(^[-_a-zA-Z0-9]{1,64}$)re"_el);
    auto name = content;
    if (kind == "file"_el) {
        auto stem = el::String{};
        if (content == "text"_el) {
            stem = "sample-text"_el;
        } else if (content == "empty"_el) {
            stem = "empty-file"_el;
        } else if (content == "none"_el) {
            stem = "missing-file"_el;
        } else {
            throwError("Invalid demo file placeholder."_el);
        }
        if (fileIndex > 0) {
            stem = el::join(stem, "-"_el, el::String::fromInteger(fileIndex + 1));
        }
        ++fileIndex;
        name = el::join(stem, ".txt"_el);
        if (content != "none"_el) {
            auto text = el::String{};
            if (content == "text"_el) {
                text = "This is the first line of demo text.\n"
                       "This is the second line of demo text.\n"
                       "This is the third line of demo text.\n"
                       "This is the fourth line of demo text.\n"
                       "This is the fifth line of demo text.\n"_el;
            }
            _files.updateText(temporary / name, text);
        }
    } else {
        if (!reFixtureName->match(name)) {
            throwError("Invalid demo directory placeholder."_el);
        }
        _files.createDirectories(temporary / name);
    }
    return name;
}

auto DemoExecutor::copyDemoFixture(const el::String &argument, const el::Path &temporary, int &fixtureIndex) const
    -> el::Path {
    if (el::Path{argument}.elements().contains(".."_el)) {
        throwError("Demo fixture must not contain parent traversal."_el);
    }
    const auto source = _files.checked(el::Path{argument});
    if (source.info().fileSize() > el::ByteCount{1024U * 1024U}) {
        throwError("Demo input exceeds 1 MiB."_el);
    }
    if (!source.info().isRegularFile()) {
        throwError("Demo fixture does not exist."_el);
    }
    const auto name = el::join("fixture-"_el, el::String::fromInteger(++fixtureIndex), "-"_el, source.name());
    const auto destination = temporary / name;
    source.operations().copyToOrThrow(destination);
    return destination;
}

auto DemoExecutor::expandArguments(
    const el::StringList &parsed, const el::StringMap<el::Path> &fixtures, const el::Path &temporary) const
    -> ExpandedArguments {
    static const auto rePlaceholder = el::RegEx::compile(R"re((?x)
        ^ \{ (?<kind>file|directory) :      # Supported fixture placeholder kind
        (?<value>[^{}]+)                    # Nonempty fixture reference
        \} $                                # Complete placeholder
    )re"_el);
    static const auto reDemoPath = el::RegEx::compile(R"re((?x)
        ^ demos/[-A-Za-z0-9_.]+             # First directory below demos
        (?: /[-A-Za-z0-9_.]+ )+             # At least one additional path component
        $                                   # Complete demo path
    )re"_el);
    static const auto reSimpleArgument = el::RegEx::compile(R"re(^[-A-Za-z0-9_]+$)re"_el);
    static const auto reBranchArgument = el::RegEx::compile(R"re(^[-A-Za-z0-9_]+(?:\.[-A-Za-z0-9_]+)+$)re"_el);

    auto result = ExpandedArguments{};
    result.displayed.append(parsed.first());
    auto fileIndex = 0;
    auto fixtureIndex = 0;
    auto previous = el::String{};
    for (const auto &argument : parsed.slice(el::ItemRange::fromBegin(el::ItemIndex{1}))) {
        auto value = argument;
        auto display = argument;
        if (const auto fixture = fixtures.get(argument)) {
            value = fixture->toNativeString();
            result.redactions.set(value, argument);
        } else if (const auto placeholder = rePlaceholder->match(argument)) {
            const auto name = createPlaceholder(
                placeholder->content("kind"_el), placeholder->content("value"_el), temporary, fileIndex);
            value = (temporary / name).toNativeString();
            display = name;
            result.redactions.set(value, name);
        } else if (reDemoPath->match(argument)) {
            const auto destination = copyDemoFixture(argument, temporary, fixtureIndex);
            value = destination.toNativeString();
            result.redactions.set(value, argument);
        } else if (!reSimpleArgument->match(argument)) {
            if (previous != "--branch"_el || !reBranchArgument->match(argument)) {
                throwError("Demo argument is not allowed."_el);
            }
        }
        if (result.redactions.contains(value)) {
            result.width = std::max(result.width, static_cast<int>(value.length().toSizeT()) + 32);
        }
        result.values.append(value);
        result.displayed.append(display);
        previous = argument;
    }
    return result;
}

auto DemoExecutor::renderOutput(el::String raw, const el::StringMap<el::String> &redactions) -> el::String {
    if (cleanup::text::lines(raw).count() > el::ItemCount{100}) {
        throwError("Demo output exceeds 100 lines."_el);
    }
    for (const auto &[path, name] : redactions) {
        raw = raw.replacedAll(path, name);
        const auto displayPath = el::Path::fromNative(path).toString();
        if (displayPath != path) {
            raw = raw.replacedAll(displayPath, name);
        }
    }
    auto emulator = el::cterm::TerminalEmulation{
        el::cterm::TerminalEmulationOptions{}.setSize({90, 40}).setScrollbackLimit(el::block::Coordinate{2000})};
    emulator.write(raw);
    emulator.finish();
    auto output = emulator.activeBuffer().toString(
        el::cterm::BufferRenderOptions{}.setTrimTrailingSpaces(true).setTrimTrailingEmptyLines(true));
    if (!emulator.isAlternateScreenActive() && emulator.hasScrollback()) {
        const auto history =
            emulator.scrollbackBuffer().toString(el::cterm::BufferRenderOptions{}.setTrimTrailingSpaces(true));
        if (output.isEmpty()) {
            output = history;
        } else {
            output = el::join(history, "\n"_el, output);
        }
    }
    if (!output.isEmpty()) {
        output = el::join(output, "\n"_el);
    }
    return output;
}

void DemoExecutor::throwError(const el::String &description, const el::String &path) {
    throw el::ApplicationError{"Demo Execution Failed"_el, description}.withSourcePath(path);
}

}
