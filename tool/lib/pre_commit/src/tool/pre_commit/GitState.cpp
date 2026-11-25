// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "GitState.hpp"

#include "impl/NativeGitFile.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteInputStream.hpp>
#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringSplitter.hpp>
#include <tool/frame/ExternalCommand.hpp>

namespace tool::pre_commit {

using namespace el::text::literals;

GitState::GitState(el::Path projectRoot) : _root{std::move(projectRoot)} {
}

auto GitState::gitOutput(const el::StringList &arguments) const -> el::String {
    if (_git.isEmpty()) {
        for (const auto &candidate : el::EnvironmentVariables{}.executableCandidatesOrThrow("git"_el)) {
            const auto path = candidate.resolve();
            if (!path.isEmpty() && path.info().isRegularFile() && path.info().isExecutable()) {
                _git = path;
                break;
            }
        }
        if (_git.isEmpty()) {
            throw el::ApplicationError{"Git Unavailable"_el, "Install Git and make it available on PATH."_el};
        }
    }
    const auto result = frame::ExternalCommand::runExecutable(_git, arguments, _root, false);
    if (!result.status.isSuccess()) {
        throw el::ApplicationError{
            "Git Command Failed"_el, el::join("git "_el, arguments.join(" "_el), "\n"_el, result.error)};
    }
    return result.output;
}

auto GitState::untrackedFingerprints(const frame::TaskProgress &progress) const -> el::ByteBlock {
    const auto listing = gitOutput(el::StringList{"ls-files"_el, "--others"_el, "--exclude-standard"_el, "-z"_el});
    auto names = el::StringList{};
    auto split = el::StringSplitter{listing, U'\0'};
    while (!split.isAtEnd()) {
        const auto name = split.next();
        if (!name.isEmpty()) {
            names.append(name);
        }
    }
    progress.beginPhase("Fingerprinting untracked files"_el, names.count());
    auto hasher = el::Hasher{el::HashAlgorithm::Sha2_256};
    hasher.update(listing);
    for (const auto &name : names) {
        const auto relative = el::Path::fromPosix(name);
        if (relative.isEmpty()) {
            impl::NativeGitFile{_root, name}.appendFingerprint(hasher);
            progress.advance();
            continue;
        }
        const auto path = _root / relative;
        const auto info = path.info();
        if (info.isSymlink()) {
            hasher.update("symlink\0"_el);
            hasher.update(impl::NativeGitFile{_root, name}.symlinkFingerprint());
        } else if (!info.exists()) {
            hasher.update("missing\0"_el);
        } else if (!info.isRegularFile()) {
            hasher.update("other\0"_el);
        } else {
            hasher.update("file\0"_el);
            hasher.update(fileFingerprint(path));
        }
        progress.advance();
    }
    return hasher.finalize();
}

auto GitState::fileFingerprint(const el::Path &path) -> el::ByteBlock {
    const auto input =
        path.content().openByteInputStream(el::PathReadDataOptions{}.setSymlinkMode(el::SymlinkMode::Skip));
    auto hasher = el::Hasher{el::HashAlgorithm::Sha2_256};
    while (true) {
        const auto chunk = input->read(el::ByteCount{64 * 1024});
        if (chunk == el::StreamReadStatus::Finished) {
            break;
        }
        if (chunk == el::StreamReadStatus::Timeout) {
            throw el::ApplicationError{"Git Fingerprint Failed"_el, "Reading an untracked file timed out."_el}
                .withSourcePath(path.toString());
        }
        hasher.update(chunk.data());
    }
    return hasher.finalize();
}

auto GitState::snapshot(const frame::TaskProgress &progress) const -> GitStateSnapshot {
    progress.beginPhase("Reading Git status and diffs"_el);
    auto state = GitStateSnapshot{};
    state.status = gitOutput(el::StringList{"status"_el, "--porcelain=v1"_el, "-z"_el, "--untracked-files=all"_el});
    state.unstagedDiff = gitOutput(el::StringList{"diff"_el, "--binary"_el, "--no-ext-diff"_el});
    state.stagedDiff = gitOutput(el::StringList{"diff"_el, "--cached"_el, "--binary"_el, "--no-ext-diff"_el});
    state.untrackedFiles = untrackedFingerprints(progress);
    return state;
}

void GitState::capture(const frame::TaskProgress &progress) {
    _initial.reset();
    _initial = snapshot(progress);
}

void GitState::verify(const frame::TaskProgress &progress) const {
    if (!_initial) {
        throw el::ApplicationError{
            "Git State Comparison Unavailable"_el,
            "Initial Git state capture failed; repository changes cannot be verified."_el};
    }
    if (snapshot(progress) == *_initial) {
        return;
    }
    const auto status = gitOutput(el::StringList{"status"_el, "--short"_el, "--untracked-files=all"_el});
    throw el::ApplicationError{
        "Pre-commit Made Changes"_el,
        el::join("Review the changes, then run pre-commit again.\n\nCurrent git status:\n"_el, status)};
}

}
