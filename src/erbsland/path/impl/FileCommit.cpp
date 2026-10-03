// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileCommit.hpp"

#include "BackendFactory.hpp"
#include "PathBackend.hpp"
#include "PathInfoData.hpp"

#include "../PathContent.hpp"
#include "../PathCreateDirectoryOptions.hpp"
#include "../PathError.hpp"
#include "../PathInfo.hpp"
#include "../PathMoveOptions.hpp"
#include "../PathOperations.hpp"
#include "../PathReadDataOptions.hpp"
#include "../PathResolveMode.hpp"
#include "../PathTempFileOptions.hpp"
#include "../PathWalker.hpp"
#include "../PathWalkFn.hpp"
#include "../PathWalkOptions.hpp"

#include "../../err/RuntimeError.hpp"
#include "../../stream/ByteInputStream.hpp"
#include "../../stream/TempByteOutputStream.hpp"
#include "../../system/PlatformErrorCategory.hpp"
#include "../../system/PlatformErrorContext.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringJoins.hpp"
#include "../../text/StringSide.hpp"
#include "../../unit/ByteCount.hpp"
#include "../../unit/CpIndex.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::path::impl {

using namespace stream;
using namespace text;
using namespace text::literals;

FileCommit::FileCommit(Path source, Path destination, PathCommitOptions options, OutputStreamSettings settings) :
    _source{std::move(source)},
    _destination{std::move(destination)},
    _options{std::move(options)},
    _settings{settings} {
    stage("Resolving the temporary and destination paths failed."_el, _source, _destination, [this]() -> void {
        if (_source.isEmpty() || _destination.isEmpty()) {
            throwPathError("The temporary file lease or destination path is empty."_el, _source, _destination);
        }
        _source = _source.toAbsoluteOrThrow().resolveOrThrow(PathResolveMode::Lexical);
        _destination = _destination.toAbsoluteOrThrow().resolveOrThrow(PathResolveMode::Lexical);
    });
}

void FileCommit::throwPathError(
    String description, const Path &source, const Path &destination, std::exception_ptr cause) {
    throw PathError{
        PathErrorContext{"File replacement failed"_el, std::move(description)}
            .setSourcePath(source.toString())
            .setTargetPath(destination.toString()),
        std::move(cause)};
}

void FileCommit::stage(
    String description, const Path &source, const Path &destination, const std::function<void()> &fn) const {
    try {
        fn();
    } catch (const err::RuntimeError &) {
        throwPathError(std::move(description), source, destination, std::current_exception());
    }
}

auto FileCommit::inspect(const Path &path, const bool allowMissing) const -> PathInfoData {
    try {
        // The bound path is already absolute. Inspect it natively without an existence-resolving step,
        // which would discard the native NotFound context before metadata loading.
        return pathBackend().loadResolvedInfoOrThrow(path, path, PathInfoPart::Type);
    } catch (const PathError &error) {
        if (allowMissing && error.platformContext() != nullptr &&
            error.platformContext()->category() == system::PlatformErrorCategory::NotFound) {
            return {};
        }
        throw;
    }
}

auto FileCommit::validateDestination() const -> bool {
    const auto info = inspect(_destination, true);
    if (info.exists && info.type != PathType::RegularFile) {
        throwPathError(
            "The destination must be absent or a regular file; links and reparse points are rejected."_el,
            _source,
            _destination);
    }
    if (info.exists && _options.collisionMode() == PathCollisionMode::Stop) {
        throwPathError("The destination already exists and the collision policy is Stop."_el, _source, _destination);
    }
    return info.exists;
}

void FileCommit::prepare() const {
    stage("Validating the replacement paths and policies failed."_el, _source, _destination, [this]() -> void {
        if (_options.collisionMode() != PathCollisionMode::Stop &&
            _options.collisionMode() != PathCollisionMode::Overwrite) {
            throwPathError("Only Stop and Overwrite collision policies are supported."_el, _source, _destination);
        }
        if (_options.backupCount().isInfinite()) {
            throwPathError("The backup count must be finite."_el, _source, _destination);
        }
        if (_source == _destination || inspect(_source).type != PathType::RegularFile) {
            throwPathError(
                "The source must be a regular temporary file at a different path."_el, _source, _destination);
        }
        validateDestination();
    });
    stage("Preparing the destination parent directory failed."_el, _source, _destination, [this]() -> void {
        if (_options.createParents()) {
            _destination.parent().operations().createDirectoryOrThrow(
                PathCreateDirectoryOptions{}.setCreateParents(true).setCreationMode(PathCreateMode::CreateOrOverwrite));
        }
        if (inspect(_destination.parent()).type != PathType::Directory) {
            throwPathError("The destination parent is not an accessible directory."_el, _source, _destination);
        }
    });
}

void FileCommit::timeout(String operation, const Path &source, const Path &destination) const {
    throwPathError(
        text::join(
            operation, " did not complete within the configured timeout: "_el, _settings.timeout().toString(), "."_el),
        source,
        destination);
}

void FileCommit::finalize(OutputStream &stream) const {
    if (stream.state() != StreamState::Closing && stream.state() != StreamState::Closed) {
        stage("Flushing the temporary output failed."_el, _source, _destination, [this, &stream]() -> void {
            if (!stream.flush().isSuccess()) {
                timeout("Flushing the temporary output"_el, _source, _destination);
            }
        });
    }
    stage("Closing the temporary output failed."_el, _source, _destination, [this, &stream]() -> void {
        if (!stream.close().isClosed()) {
            timeout("Closing the temporary output"_el, _source, _destination);
        }
    });
}

auto FileCommit::check() const -> bool {
    stage("The caller's pre-commit check failed."_el, _source, _destination, [this]() -> void {
        if (_options.beforeCommit()) {
            _options.beforeCommit()(_source, _destination);
        }
    });
    auto exists = false;
    stage(
        "Revalidating the destination before replacement failed."_el, _source, _destination, [this, &exists]() -> void {
            exists = validateDestination();
        });
    return exists;
}

auto FileCommit::backupPath(const std::size_t generation) const -> Path {
    return _destination.withName(text::join(_destination.name(), ".bak."_el, String::fromInteger(generation)));
}

auto FileCommit::backupGenerations() const -> std::vector<std::size_t> {
    auto generations = std::vector<std::size_t>{};
    const auto parent = _destination.parent();
    const auto prefix = text::join(_destination.name(), ".bak."_el);
    parent.walker().walkOrThrow(
        PathWalkFn{[this, &generations, &prefix, &parent](const Path &entry) -> PathWalkStatus {
            if (entry == parent) {
                return PathWalkStatus::Continue;
            }
            const auto name = entry.name();
            if (name.startsWith(prefix)) {
                const auto suffix =
                    name.slice(StringSide::Back, unit::CpIndex::fromSizeT(prefix.characterLength().toSizeT()));
                const auto generation = suffix.toInteger<std::size_t>();
                if (generation && generation > 0 && suffix == String::fromInteger(generation)) {
                    const auto info = inspect(entry);
                    if (info.type != PathType::RegularFile) {
                        throwPathError(
                            "A managed backup is not a regular file; links and reparse points are rejected."_el,
                            entry,
                            _destination);
                    }
                    generations.push_back(generation);
                }
            }
            return PathWalkStatus::Skip;
        }},
        PathWalkOptions{}.setTypes(PathType::All).setSymlinkMode(SymlinkMode::Use));
    std::ranges::sort(generations, std::greater<>{});
    return generations;
}

void FileCommit::backups() {
    auto staged = TempByteOutputStreamPtr{};
    stage(
        "Copying and finalizing the old destination for backup failed."_el,
        _destination,
        backupPath(1),
        [this, &staged]() -> void {
            staged = _destination.parent().operations().openTempByteOutputStreamOrThrow(
                PathTempFileOptions{}
                    .setAccessProfile(PathAccessProfile::UserOnly)
                    .setPrefix(".erbsland-backup-"_el)
                    .setSuffix(".tmp"_el)
                    .setStreamSettings(_settings));
            const auto inputPath =
                _destination.parent().resolveOrThrow(PathResolveMode::Physical).joinedNative(_destination.name());
            auto input = inputPath.content().openByteInputStream(
                PathReadDataOptions{}.setSymlinkMode(SymlinkMode::Skip).setTimeout(_settings.timeout()));
            while (true) {
                const auto block = input->read(std::min(unit::ByteCount{16U * 1024U}, _settings.backBufferLimit()));
                if (block.isFinished()) {
                    break;
                }
                if (block.isTimeout()) {
                    timeout("Reading the old destination for backup"_el, _destination, staged->path());
                }
                if (block.data().isEmpty()) {
                    throwPathError(
                        "The backup input reported data without providing bytes."_el, _destination, staged->path());
                }
                if (!staged->write(block.data().span()).isSuccess()) {
                    timeout("Writing the backup staging file"_el, _destination, staged->path());
                }
            }
            const auto cleanup = staged->removeOnClose();
            staged->setRemoveOnClose(false);
            try {
                FileCommit{staged->path(), backupPath(1), PathCommitOptions{}, _settings}.finalize(*staged);
            } catch (const PathError &) {
                staged->setRemoveOnClose(cleanup);
                throw;
            }
            staged->setRemoveOnClose(cleanup);
        });
    auto generations = std::vector<std::size_t>{};
    stage(
        "Inspecting the numbered backup files failed."_el, _destination, backupPath(1), [this, &generations]() -> void {
            generations = backupGenerations();
        });
    const auto retained = _options.backupCount().toSizeT();
    for (const auto generation : generations) {
        const auto source = backupPath(generation);
        if (generation >= retained) {
            stage(
                "Removing a backup beyond the retained generations failed."_el,
                source,
                _destination,
                [&source]() -> void { source.operations().removeOrThrow(); });
        } else {
            const auto destination = backupPath(generation + 1);
            stage("Rotating a numbered backup failed."_el, source, destination, [&source, &destination]() -> void {
                source.operations().moveToOrThrow(
                    destination, PathMoveOptions{}.setCollisionMode(PathCollisionMode::Overwrite));
            });
        }
    }
    stage("Installing the newest backup failed."_el, staged->path(), backupPath(1), [this, &staged]() -> void {
        staged->commitToOrThrow(backupPath(1));
    });
}

void FileCommit::commit(OutputStream *stream) {
    prepare();
    if (stream != nullptr) {
        finalize(*stream);
    }
    const auto destinationExists = check();
    if (!_options.backupCount().isZero()) {
        if (destinationExists) {
            try {
                backups();
            } catch (const PathError &) {
                if (!_options.ignoreBackupErrors()) {
                    throw;
                }
            }
        }
        check();
    }
    stage(
        "Moving the completed temporary file into the destination failed."_el, _source, _destination, [this]() -> void {
            _source.operations().moveToOrThrow(
                _destination, PathMoveOptions{}.setCollisionMode(_options.collisionMode()));
        });
}

}
