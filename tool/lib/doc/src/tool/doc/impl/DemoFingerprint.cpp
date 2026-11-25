// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DemoFingerprint.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteArray.hpp>
#include <erbsland/Endianness.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/PathInfo.hpp>

namespace tool::doc::impl {

using namespace el::text::literals;

DemoFingerprint::DemoFingerprint(const frame::ProjectFiles &files) :
    _files{files}, _hasher{el::HashAlgorithm::Sha2_256} {
    addText("erbsland-doc-demo-v2"_el);
}

void DemoFingerprint::addLength(const el::ByteCount count) {
    auto length = el::ByteArray<8>{};
    length.setIntegerOrThrow(el::ByteIndex{0}, count.toRawValue(), el::Endianness::Big);
    _hasher.update(length.span());
}

void DemoFingerprint::addText(const el::String &text) {
    addLength(text.length());
    _hasher.update(text);
}

void DemoFingerprint::addOptions(const el::StringMap<el::String> &options) {
    for (const auto &[key, value] : options) {
        if (key != "sha256"_el) {
            addText("option"_el);
            addText(key);
            addText(value);
        }
    }
}

void DemoFingerprint::addFile(const el::StringLiteral kind, const el::Path &path, const el::String &name) {
    if (_files.checked(path).info().fileSize() > el::ByteCount{1024U * 1024U}) {
        throw el::ApplicationError{"Demo Fingerprinting Failed"_el, "Demo input exceeds 1 MiB."_el}.withSourcePath(
            path.toString());
    }
    const auto data = _files.readData(path);
    addText(kind);
    addText(name);
    addLength(data.length());
    _hasher.update(data.span());
}

auto DemoFingerprint::finish() -> el::String {
    return el::String::fromByteBlock(_hasher.finalize());
}

}
