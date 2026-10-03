// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileHasherApp.hpp"

#include <DemoTerminal.hpp>
#include <erbsland/all_cryptology.hpp>
#include <erbsland/all_mem.hpp>
#include <erbsland/all_text_base_n.hpp>
#include <erbsland/ApplicationError.hpp>
#include <erbsland/base_n/BaseNEncoder.hpp>
#include <erbsland/base_n/BaseNFormat.hpp>
#include <erbsland/Byte.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathResolveMode.hpp>
#include <erbsland/PathType.hpp>
#include <erbsland/PathWalkOptions.hpp>
#include <erbsland/PathWalkStatus.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/SymlinkMode.hpp>
#include <erbsland/Version.hpp>

#include <array>
#include <span>

namespace demo {

void FileHasherApp::initialize() {
    DemoTerminal::enable(*this);
    info().setApplicationName("File Hasher"_el);
    info().setApplicationVersion(el::Version{1, 0, 0});
}

void FileHasherApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->addOption(cDirectoryOption)
        .setRequired()
        .setValueName("directory"_el)
        .setHelp("Directory to hash recursively."_el);
}

void FileHasherApp::handleCommandLineOptions(const erbsland::OptionValuesPtr &values) {
    _root =
        el::Path::fromNativeOrThrow(values->getText(cDirectoryOption)).resolveOrThrow(el::PathResolveMode::Physical);
}

void FileHasherApp::collectFiles() {
    _paths = collectFiles(_root);
    _totalFiles = _paths.count().toSizeT();
    _results.reserve(_totalFiles);
}

void FileHasherApp::printResults() {
    el::io::printLine();
    std::ranges::sort(_results, {}, &HashResult::path);
    for (const auto &result : _results) {
        el::io::printLine(result.digest, "  "_el, result.path.toRelativeOrThrow(_root));
    }
}

auto FileHasherApp::collectFiles(const el::Path &root) -> el::PathList {
    if (!root.info().isDirectory()) {
        throw el::ApplicationError{"The path must name a directory."_el};
    }
    auto paths = el::PathList{};
    auto options = el::PathWalkOptions{};
    options.setTypes(el::PathType::RegularFile).setSymlinkMode(el::SymlinkMode::Skip);
    root.walker().walkOrThrow(
        [&paths](const el::Path &path) -> el::PathWalkStatus {
            paths.append(path);
            return el::PathWalkStatus::Continue;
        },
        options);
    return paths;
}

auto FileHasherApp::hashFile(const el::Path &path) -> HashResult {
    const auto input = path.content().openByteInputStream();
    auto hasher = el::Hasher{el::HashAlgorithm::Sha2_256};
    auto buffer = std::array<el::Byte, 64U * 1024U>{};
    while (true) {
        const auto read = input->read(buffer);
        if (read.isFinished()) {
            break;
        }
        if (read.isTimeout()) {
            throw el::ApplicationError{"Reading a file timed out."_el};
        }
        hasher.update(std::span<const el::Byte>{buffer.data(), read.data().toSizeT()});
    }
    return HashResult{path, el::base_n::BaseNEncoder{hasher.finalize(), el::base_n::BaseNFormat::base16()}.toString()};
}

}
