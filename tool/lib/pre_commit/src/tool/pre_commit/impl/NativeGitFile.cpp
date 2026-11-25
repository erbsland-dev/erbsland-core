// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NativeGitFile.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/StringConverter.hpp>

#include <array>
#include <fstream>
#include <span>
#include <utility>

namespace tool::pre_commit::impl {

using namespace el::text::literals;

NativeGitFile::NativeGitFile(const el::Path &root, el::String relative) : _relative{std::move(relative)} {
    try {
        // U8-to-standard conversion preserves bytes. Text decoding would reject U+FEFF and non-UTF-8 POSIX names.
        _path = root.toStdPath() / std::filesystem::path{el::StringConverter{_relative}.toStdU8String()};
    } catch (const std::filesystem::filesystem_error &error) {
        fail(el::StringConverter{error.code().message()}.toString());
    }
}

void NativeGitFile::fail(el::String description) const {
    throw el::ApplicationError{"Git Fingerprint Failed"_el, std::move(description)}.withSourcePath(_relative);
}

auto NativeGitFile::symlinkFingerprint() const -> el::ByteBlock {
    auto error = std::error_code{};
    const auto target = std::filesystem::read_symlink(_path, error);
    if (error) {
        fail(el::StringConverter{error.message()}.toString());
    }
    auto hasher = el::Hasher{el::HashAlgorithm::Sha2_256};
    hasher.update(el::ByteBlock::fromSpan(std::as_bytes(std::span{target.native()})));
    return hasher.finalize();
}

auto NativeGitFile::fileFingerprint() const -> el::ByteBlock {
    auto input = std::ifstream{_path, std::ios::binary};
    if (!input.is_open()) {
        fail("An untracked file could not be opened for reading."_el);
    }
    auto hasher = el::Hasher{el::HashAlgorithm::Sha2_256};
    auto buffer = std::array<char, 64 * 1024>{};
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = static_cast<std::size_t>(input.gcount());
        hasher.update(el::ByteBlock::fromSpan(std::as_bytes(std::span{buffer}.first(count))));
    }
    if (!input.eof()) {
        fail("An untracked file could not be read completely."_el);
    }
    return hasher.finalize();
}

void NativeGitFile::appendFingerprint(el::Hasher &hasher) const {
    auto error = std::error_code{};
    const auto status = std::filesystem::symlink_status(_path, error);
    if (error && error != std::errc::no_such_file_or_directory) {
        fail(el::StringConverter{error.message()}.toString());
    }
    if (std::filesystem::is_symlink(status)) {
        hasher.update("symlink\0"_el);
        hasher.update(symlinkFingerprint());
    } else if (!std::filesystem::exists(status)) {
        hasher.update("missing\0"_el);
    } else if (std::filesystem::is_regular_file(status)) {
        hasher.update("file\0"_el);
        hasher.update(fileFingerprint());
    } else {
        hasher.update("other\0"_el);
    }
}

}
