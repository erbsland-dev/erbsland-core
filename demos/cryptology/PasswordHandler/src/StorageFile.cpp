// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StorageFile.hpp"

#include <erbsland/EscapeAmount.hpp>
#include <erbsland/EscapeFormat.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathAccessProfile.hpp>
#include <erbsland/PathCollisionMode.hpp>
#include <erbsland/PathCreateDirectoryOptions.hpp>
#include <erbsland/PathCreateMode.hpp>
#include <erbsland/PathMoveOptions.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/PathTempFileOptions.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/TempTextOutputStream.hpp>

namespace demo {

using namespace el::text::literals;

void appendElclText(el::StringEditor &output, const el::String &name, const el::String &value) {
    output.append(name);
    output.append(": \""_el);
    output.append(value.toEscaped(el::EscapeFormat::Config, el::EscapeAmount::Required));
    output.append("\"\n"_el);
}

void writeStorageFile(const el::Path &path, const el::String &text, const el::PathCollisionMode collisionMode) {
    auto directoryOptions = el::PathCreateDirectoryOptions{};
    directoryOptions.setCreateParents(true)
        .setCreationMode(el::PathCreateMode::CreateOrOverwrite)
        .setAccessProfile(el::PathAccessProfile::UserOnly);
    const auto directory = path.parent();
    directory.operations().createDirectoryOrThrow(directoryOptions);

    auto temporaryOptions = el::PathTempFileOptions{};
    temporaryOptions.setPrefix(".password-handler-"_el)
        .setSuffix(".tmp"_el)
        .setAccessProfile(el::PathAccessProfile::UserOnly);
    const auto temporary = directory.operations().openTempTextOutputStreamOrThrow(temporaryOptions);
    auto releasedPath = el::Path{};
    try {
        if (!temporary->write(text).isSuccess() || !temporary->flush().isSuccess()) {
            throw el::RuntimeError{"Writing a password storage file timed out."_el};
        }
        releasedPath = temporary->release();
        if (!temporary->close().isClosed()) {
            throw el::RuntimeError{"Closing a password storage file timed out."_el};
        }
        auto moveOptions = el::PathMoveOptions{};
        moveOptions.setCollisionMode(collisionMode);
        releasedPath.operations().moveToOrThrow(path, moveOptions);
        releasedPath = {};
    } catch (...) {
        temporary->abort();
        if (!releasedPath.isEmpty()) {
            if (releasedPath.operations().remove().isSuccessful()) {
                releasedPath = {};
            }
        }
        throw;
    }
}

}
