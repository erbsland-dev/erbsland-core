// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathContent.hpp"

#include "Path.hpp"
#include "PathCreateDirectoryOptions.hpp"
#include "PathError.hpp"
#include "PathOperations.hpp"
#include "PathResolveMode.hpp"
#include "PathTempFileOptions.hpp"

#include "impl/FileCommit.hpp"

#include "../err/RuntimeError.hpp"
#include "../stream/TempByteOutputStream.hpp"
#include "../stream/TempTextOutputStream.hpp"
#include "../text/Literals.hpp"
#include "../text/StringJoins.hpp"
#include "../text/StringSide.hpp"
#include "../unit/ByteIndex.hpp"
#include "../unit/CpCount.hpp"
#include "../unit/CpIndex.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace erbsland::path {

using namespace text;
using namespace unit;
using namespace text::literals;

auto PathContent::replaceData(const mem::ByteBlock &data, PathReplaceDataOptions options) const noexcept
    -> util::Result {
    try {
        replaceDataOrThrow(data, std::move(options));
        return util::Result::Success;
    } catch (const PathError &) {
        return util::Result::Failure;
    }
}

void PathContent::replaceDataOrThrow(const mem::ByteBlock &data, PathReplaceDataOptions options) const {
    auto destination = path();
    auto temporary = stream::TempByteOutputStreamPtr{};
    auto stage = String{"Resolving the replacement destination failed."_el};
    try {
        destination = destination.toAbsoluteOrThrow().resolveOrThrow(PathResolveMode::Lexical);
        stage = "Preparing the replacement destination parent failed."_el;
        if (options.createParents()) {
            destination.parent().operations().createDirectoryOrThrow(
                PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(PathCreateMode::CreateOrOverwrite));
        }
        stage = "Creating the sibling temporary replacement file failed."_el;
        auto tempOptions = PathTempFileOptions{}
                               .setPrefix(".erbsland-replace-"_el)
                               .setSuffix(".tmp"_el)
                               .setAccessProfile(options.accessProfile())
                               .setStreamSettings(options.streamSettings());
        temporary = destination.parent().operations().openTempByteOutputStreamOrThrow(tempOptions);
        stage = "Validating the replacement policies and paths failed."_el;
        impl::FileCommit{temporary->path(), destination, options.commitOptions(), options.streamSettings()}.prepare();
        stage = "Writing the complete replacement content failed."_el;
        const auto chunkSize = std::min(ByteCount{16U * 1024U}, options.streamSettings().backBufferLimit());
        if (chunkSize.isZero()) {
            impl::FileCommit::throwPathError(
                "The output back-buffer limit must be nonzero."_el, temporary->path(), destination);
        }
        for (auto offset = ByteIndex{}; offset < data.endIndex(); offset += chunkSize) {
            if (!temporary->write(data.span(offset, chunkSize)).isSuccess()) {
                impl::FileCommit::throwPathError(
                    text::join(
                        "Writing replacement content did not complete within the configured timeout: "_el,
                        options.timeout().toString(),
                        "."_el),
                    temporary->path(),
                    destination);
            }
        }
        stage = "Committing the completed replacement content failed."_el;
        temporary->commitToOrThrow(destination, options.commitOptions());
    } catch (const err::RuntimeError &) {
        auto source = Path{};
        if (temporary != nullptr) {
            source = temporary->path();
        }
        impl::FileCommit::throwPathError(std::move(stage), source, destination, std::current_exception());
    }
}

auto PathContent::replaceText(const String &text, PathReplaceTextOptions options) const noexcept -> util::Result {
    try {
        replaceTextOrThrow(text, std::move(options));
        return util::Result::Success;
    } catch (const PathError &) {
        return util::Result::Failure;
    }
}

void PathContent::replaceTextOrThrow(const String &text, PathReplaceTextOptions options) const {
    auto destination = path();
    auto temporary = stream::TempTextOutputStreamPtr{};
    auto stage = String{"Resolving the replacement destination failed."_el};
    try {
        destination = destination.toAbsoluteOrThrow().resolveOrThrow(PathResolveMode::Lexical);
        stage = "Preparing the replacement destination parent failed."_el;
        if (options.createParents()) {
            destination.parent().operations().createDirectoryOrThrow(
                PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(PathCreateMode::CreateOrOverwrite));
        }
        stage = "Creating the sibling temporary replacement file failed."_el;
        auto tempOptions = PathTempFileOptions{}
                               .setPrefix(".erbsland-replace-"_el)
                               .setSuffix(".tmp"_el)
                               .setAccessProfile(options.accessProfile())
                               .setStreamSettings(options.streamSettings());
        temporary = destination.parent().operations().openTempTextOutputStreamOrThrow(
            tempOptions,
            PathWriteTextOptions{}
                .setEncoding(options.encoding())
                .setBomMode(options.bomMode())
                .setStreamSettings(options.streamSettings()));
        stage = "Validating the replacement policies and paths failed."_el;
        impl::FileCommit{temporary->path(), destination, options.commitOptions(), options.streamSettings()}.prepare();
        stage = "Writing the complete replacement content failed."_el;
        const auto byteLimit = std::min(std::size_t{16U * 1024U}, options.streamSettings().backBufferLimit().toSizeT());
        auto cpLimit = std::size_t{1U};
        if (byteLimit > 4U) {
            cpLimit = std::max(std::size_t{1U}, (byteLimit - 4U) / 4U);
        }
        auto remaining = text;
        do {
            const auto chunk = remaining.slice(StringSide::Front, CpCount::fromSizeT(cpLimit));
            if (!temporary->write(chunk).isSuccess()) {
                impl::FileCommit::throwPathError(
                    text::join(
                        "Writing replacement content did not complete within the configured timeout: "_el,
                        options.timeout().toString(),
                        "."_el),
                    temporary->path(),
                    destination);
            }
            remaining = remaining.slice(StringSide::Back, CpIndex::fromSizeT(cpLimit));
        } while (!remaining.isEmpty());
        stage = "Committing the completed replacement content failed."_el;
        temporary->commitToOrThrow(destination, options.commitOptions());
    } catch (const err::RuntimeError &) {
        auto source = Path{};
        if (temporary != nullptr) {
            source = temporary->path();
        }
        impl::FileCommit::throwPathError(std::move(stage), source, destination, std::current_exception());
    }
}

}
