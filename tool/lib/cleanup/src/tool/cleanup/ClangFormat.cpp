// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ClangFormat.hpp"

#include "CleanupText.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringSplitter.hpp>
#include <erbsland/Subprocess.hpp>

namespace tool::cleanup {

using namespace el::text::literals;

auto ClangFormat::executable(const el::conf::ValuePtr &value, const el::Path &root) -> el::Path {
    const auto name = value->asTextOrThrow();
    const auto candidates = el::EnvironmentVariables{}.executableCandidatesOrThrow(name);
    for (const auto &candidate : candidates) {
        const auto resolved = candidate.resolve();
        if (!resolved.isEmpty() && !resolved.isRelativeTo(root) && resolved.info().isRegularFile() &&
            resolved.info().isExecutable()) {
            return resolved;
        }
    }
    throw el::conf::ConfError{
        value,
        "Formatter Executable Unavailable"_el,
        "Cannot find clang-format outside the project. Install it or configure Format.Executable."_el};
}

ClangFormat::ClangFormat(const CleanupPolicy &policy, const frame::ProjectFiles &files, const bool full) :
    _policy{policy}, _files{files}, _executable{executable(policy.formatterValue, files.projectRoot())} {
    const auto style = files.readText(policy.style);
    // The explicit style file avoids reading uncontrolled per-directory styles in a subprocess.
    _signature = text::digest(
        el::join("code-cleanup-v1\n"_el, _executable.toString(), "\n"_el, run(el::StringList{"--version"_el}), style));
    if (full || !policy.cache.info().exists()) {
        return;
    }
    auto lines = el::StringSplitter{files.readText(policy.cache), U'\n'};
    if (lines.isAtEnd() || lines.next().trimmed(el::CharSet{U'\r'}, el::StringSide::Back) != _signature) {
        return;
    }
    static const auto digestPattern = el::RegEx::compile(R"/([0-9a-f]{64})/"_el);
    while (!lines.isAtEnd()) {
        const auto line = lines.next().trimmed(el::CharSet{U'\r'}, el::StringSide::Back);
        if (line.isEmpty() && lines.isAtEnd()) {
            break;
        }
        const auto parts = line.splitAtFirst("\t"_el);
        if (!parts) {
            _fingerprints.clear();
            return;
        }
        const auto &[name, digest] = *parts;
        if (name.isEmpty() || !digestPattern->fullMatch(digest) || _fingerprints.contains(name)) {
            _fingerprints.clear();
            return;
        }
        _fingerprints.set(name, digest);
    }
}

auto ClangFormat::run(const el::StringList &arguments) const -> el::String {
    const auto environment = el::StringList{
        "HOME"_el, "PATH"_el, "LANG"_el, "LC_ALL"_el, "TMPDIR"_el, "TEMP"_el, "TMP"_el, "SystemRoot"_el, "WINDIR"_el};
    const auto options = el::SubprocessOptions{}
                             .setWorkingDirectory(_files.projectRoot())
                             .setInheritStandardInput(false)
                             .setStandardOutputMode(el::SubprocessOutputMode::Capture)
                             .setStandardErrorMode(el::SubprocessOutputMode::Capture)
                             .setCaptureLimit(el::SubprocessOptions::cMaximumCaptureLimit)
                             .setEnvironmentChanges(el::EnvironmentChanges{}.setInherit(environment));
    auto process = el::Subprocess::create(_executable, arguments, options);
    process->start();
    const auto status = process->wait();
    if (!status.isSuccess() || process->wasStandardOutputTruncated() || process->wasStandardErrorTruncated()) {
        text::fail(
            el::join("clang-format failed or produced truncated output:\n"_el, process->standardError()),
            arguments.join(" "_el));
    }
    return process->standardOutput();
}

auto ClangFormat::needsFormat(const el::Path &path, const el::String &fingerprint) const -> bool {
    const auto key = path.toRelativeOrThrow(_files.projectRoot()).toString();
    return _fingerprints.get(key, {}) != fingerprint;
}

auto ClangFormat::format(const el::Path &path) const -> el::String {
    return run(
        el::StringList{
            el::join("--style=file:"_el, _policy.style.toNativeString()),
            "--fail-on-incomplete-format"_el,
            path.toNativeString()});
}

auto ClangFormat::cacheText(const std::vector<std::pair<el::String, el::String>> &fingerprints) const -> el::String {
    auto size = _signature.length() + el::ByteCount::one();
    for (const auto &[path, digest] : fingerprints) {
        size += path.length() + digest.length() + el::ByteCount{2};
    }
    auto output = el::StringEditor{};
    output.reserve(size);
    output.append(_signature).append("\n"_el);
    for (const auto &[path, digest] : fingerprints) {
        output.append(path).append("\t"_el).append(digest).append("\n"_el);
    }
    return output;
}

}
