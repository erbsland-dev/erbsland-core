// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathOperations.hpp"

#include "Path.hpp"
#include "PathContent.hpp"
#include "PathError.hpp"
#include "TempDirectory.hpp"

#include "impl/BackendFactory.hpp"
#include "impl/PathOperations.hpp"

#include "../core/Application.hpp"
#include "../err/Exception.hpp"
#include "../random/Random.hpp"
#include "../random/RandomError.hpp"
#include "../stream/TempByteOutputStream.hpp"
#include "../stream/TempTextOutputStream.hpp"
#include "../system/PlatformErrorCategory.hpp"
#include "../system/PlatformErrorContext.hpp"
#include "../text/CharSet.hpp"
#include "../text/Literals.hpp"

#include <exception>
#include <memory>

namespace erbsland::path {

using namespace text::literals;

PathOperations::PathOperations() = default;

PathOperations::PathOperations(const Path &path) {
    if (!path.isEmpty()) {
        _impl = std::make_unique<impl::PathOperations>(path);
    }
}

PathOperations::~PathOperations() {
}

PathOperations::PathOperations(PathOperations &&) noexcept = default;

auto PathOperations::operator=(PathOperations &&) noexcept -> PathOperations & = default;

auto PathOperations::isEmpty() const -> bool {
    return _impl == nullptr;
}

auto PathOperations::path() const -> const Path & {
    return _impl == nullptr ? Path::empty() : _impl->path();
}

auto PathOperations::remove(const PathRemoveOptions options, const PathProgressFn &progressFn) noexcept
    -> util::Result {
    try {
        if (isEmpty()) {
            return util::Result::Failure;
        }
        _impl->removeOrThrow(options, progressFn);
        return util::Result::Success;
    } catch (const err::Exception &) {
        return util::Result::Failure;
    }
}

void PathOperations::removeOrThrow(const PathRemoveOptions options, const PathProgressFn &progressFn) {
    if (isEmpty()) {
        throw PathError{"Path removal has no source path"_el};
    }
    if (options.ignoreErrors()) {
        try {
            _impl->removeOrThrow(options, progressFn);
        } catch (const PathError &) {}
        return;
    }
    _impl->removeOrThrow(options, progressFn);
}

auto PathOperations::copyTo(
    const Path &destination, const PathCopyOptions options, const PathProgressFn &progressFn) const noexcept
    -> util::Result {
    try {
        if (isEmpty()) {
            return util::Result::Failure;
        }
        _impl->copyToOrThrow(destination, options, progressFn);
        return util::Result::Success;
    } catch (const err::Exception &) {
        return util::Result::Failure;
    }
}

void PathOperations::copyToOrThrow(
    const Path &destination, const PathCopyOptions options, const PathProgressFn &progressFn) const {
    if (isEmpty()) {
        throw PathError{"Path copy has no source path"_el};
    }
    if (options.ignoreErrors()) {
        try {
            _impl->copyToOrThrow(destination, options, progressFn);
        } catch (const PathError &) {}
        return;
    }
    _impl->copyToOrThrow(destination, options, progressFn);
}

auto PathOperations::moveTo(const Path &destination, const PathMoveOptions options) const noexcept -> util::Result {
    try {
        if (isEmpty()) {
            return util::Result::Failure;
        }
        _impl->moveToOrThrow(destination, options);
        return util::Result::Success;
    } catch (const err::Exception &) {
        return util::Result::Failure;
    }
}

void PathOperations::moveToOrThrow(const Path &destination, const PathMoveOptions options) const {
    if (isEmpty()) {
        throw PathError{"Path move has no source path"_el};
    }
    if (options.ignoreErrors()) {
        try {
            _impl->moveToOrThrow(destination, options);
        } catch (const PathError &) {}
        return;
    }
    _impl->moveToOrThrow(destination, options);
}

auto PathOperations::createFile(const PathCreateFileOptions options) const noexcept -> util::Result {
    try {
        createFileOrThrow(options);
        return util::Result::Success;
    } catch (const err::Exception &) {
        return util::Result::Failure;
    }
}

void PathOperations::createFileOrThrow(const PathCreateFileOptions options) const {
    if (isEmpty()) {
        throw PathError{"File creation has no path"_el};
    }
    _impl->createFileOrThrow(options);
}

auto PathOperations::createDirectory(const PathCreateDirectoryOptions options) const noexcept -> util::Result {
    try {
        createDirectoryOrThrow(options);
        return util::Result::Success;
    } catch (const err::Exception &) {
        return util::Result::Failure;
    }
}

void PathOperations::createDirectoryOrThrow(const PathCreateDirectoryOptions options) const {
    if (isEmpty()) {
        throw PathError{"Directory creation has no path"_el};
    }
    _impl->createDirectoryOrThrow(options);
}

auto PathOperations::createTempDirectory(PathTempDirectoryOptions options) const noexcept -> TempDirectoryPtr {
    try {
        return createTempDirectoryOrThrow(options);
    } catch (const err::Exception &) {
        return {};
    }
}

auto PathOperations::createTempDirectoryOrThrow(PathTempDirectoryOptions options) const -> TempDirectoryPtr {
    if (isEmpty()) {
        throw PathError{
            PathErrorContext{"Temporary directory could not be created"_el, "No parent directory was provided."_el}};
    }
    if (options.randomLength().isZero() || options.randomLength().isInfinite() || options.maximumAttempts().isZero() ||
        options.maximumAttempts().isInfinite()) {
        throw PathError{PathErrorContext{
            "Temporary directory could not be created"_el, "The random length or attempt count is invalid."_el}};
    }
    auto probeText = text::String{options.prefix()};
    probeText.append("x"_el);
    probeText.append(options.suffix());
    const auto nameProbe = Path{probeText};
    if (nameProbe.isEmpty() || nameProbe.isAbsolute() || nameProbe.elementCount() != unit::ElementCount::one()) {
        throw PathError{PathErrorContext{
            "Temporary directory could not be created"_el, "The prefix and suffix must form one valid path name."_el}};
    }
    try {
        auto &random = core::application().secureRandom();
        const auto alphabet = text::CharSet{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"_el};
        for (auto attempt = unit::ElementCount{}; attempt < options.maximumAttempts(); ++attempt) {
            auto name = text::String{options.prefix()};
            name.append(random.buildString(options.randomLength(), alphabet));
            name.append(options.suffix());
            const auto temporaryPath = path() / name;
            try {
                impl::pathBackend().createDirectoryEntryOrThrow(temporaryPath, options.accessProfile());
                return TempDirectoryPtr{new TempDirectory{temporaryPath, options.removeOnDestroy()}};
            } catch (const PathError &error) {
                if (!isAlreadyExistsError(error)) {
                    throw;
                }
            }
        }
    } catch (const random::RandomError &) {
        throw PathError{
            PathErrorContext{"Temporary directory could not be created"_el, "Secure random data is unavailable."_el},
            std::current_exception()};
    }
    throw PathError{PathErrorContext{
        "Temporary directory could not be created"_el, "No unique temporary directory name could be created."_el}};
}

auto PathOperations::openTempByteOutputStream(PathTempFileOptions options) const noexcept
    -> stream::TempByteOutputStreamPtr {
    try {
        return openTempByteOutputStreamOrThrow(options);
    } catch (const err::Exception &) {
        return {};
    }
}

auto PathOperations::openTempByteOutputStreamOrThrow(PathTempFileOptions options) const
    -> stream::TempByteOutputStreamPtr {
    if (isEmpty()) {
        throw PathError{
            PathErrorContext{"Temporary file could not be created"_el, "No parent directory was provided."_el}};
    }
    if (options.randomLength().isZero() || options.randomLength().isInfinite() || options.maximumAttempts().isZero() ||
        options.maximumAttempts().isInfinite()) {
        throw PathError{PathErrorContext{
            "Temporary file could not be created"_el, "The random length or attempt count is invalid."_el}};
    }
    auto probeText = text::String{options.prefix()};
    probeText.append("x"_el);
    probeText.append(options.suffix());
    const auto nameProbe = Path{probeText};
    if (nameProbe.isEmpty() || nameProbe.isAbsolute() || nameProbe.elementCount() != unit::ElementCount::one()) {
        throw PathError{PathErrorContext{
            "Temporary file could not be created"_el, "The prefix and suffix must form one valid path name."_el}};
    }
    try {
        auto &random = core::application().secureRandom();
        const auto alphabet = text::CharSet{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"_el};
        for (auto attempt = unit::ElementCount{}; attempt < options.maximumAttempts(); ++attempt) {
            auto name = text::String{options.prefix()};
            name.append(random.buildString(options.randomLength(), alphabet));
            name.append(options.suffix());
            const auto temporaryPath = path() / name;
            try {
                auto writeOptions = PathWriteDataOptions{};
                writeOptions.setCreationMode(PathCreateMode::CreateNew);
                writeOptions.setAccessProfile(options.accessProfile());
                auto stream = temporaryPath.content().openByteOutputStream(writeOptions);
                return stream::TempByteOutputStreamPtr{
                    new stream::TempByteOutputStream{temporaryPath, std::move(stream), options.removeOnClose()}};
            } catch (const PathError &error) {
                if (!isAlreadyExistsError(error)) {
                    throw;
                }
            }
        }
    } catch (const random::RandomError &) {
        throw PathError{
            PathErrorContext{"Temporary file could not be created"_el, "Secure random data is unavailable."_el},
            std::current_exception()};
    }
    throw PathError{PathErrorContext{
        "Temporary file could not be created"_el, "No unique temporary file name could be created."_el}};
}

auto PathOperations::openTempTextOutputStream(
    PathTempFileOptions temporaryOptions, PathWriteTextOptions writeOptions) const noexcept
    -> stream::TempTextOutputStreamPtr {
    try {
        return openTempTextOutputStreamOrThrow(temporaryOptions, writeOptions);
    } catch (const err::Exception &) {
        return {};
    }
}

auto PathOperations::openTempTextOutputStreamOrThrow(
    PathTempFileOptions temporaryOptions, PathWriteTextOptions writeOptions) const -> stream::TempTextOutputStreamPtr {
    if (isEmpty()) {
        throw PathError{
            PathErrorContext{"Temporary file could not be created"_el, "No parent directory was provided."_el}};
    }
    if (temporaryOptions.randomLength().isZero() || temporaryOptions.randomLength().isInfinite() ||
        temporaryOptions.maximumAttempts().isZero() || temporaryOptions.maximumAttempts().isInfinite()) {
        throw PathError{PathErrorContext{
            "Temporary file could not be created"_el, "The random length or attempt count is invalid."_el}};
    }
    auto probeText = text::String{temporaryOptions.prefix()};
    probeText.append("x"_el);
    probeText.append(temporaryOptions.suffix());
    const auto nameProbe = Path{probeText};
    if (nameProbe.isEmpty() || nameProbe.isAbsolute() || nameProbe.elementCount() != unit::ElementCount::one()) {
        throw PathError{PathErrorContext{
            "Temporary file could not be created"_el, "The prefix and suffix must form one valid path name."_el}};
    }
    try {
        auto &random = core::application().secureRandom();
        const auto alphabet = text::CharSet{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"_el};
        for (auto attempt = unit::ElementCount{}; attempt < temporaryOptions.maximumAttempts(); ++attempt) {
            auto name = text::String{temporaryOptions.prefix()};
            name.append(random.buildString(temporaryOptions.randomLength(), alphabet));
            name.append(temporaryOptions.suffix());
            const auto temporaryPath = path() / name;
            try {
                writeOptions.setCreationMode(PathCreateMode::CreateNew);
                writeOptions.setAccessProfile(temporaryOptions.accessProfile());
                auto stream = temporaryPath.content().openTextOutputStream(writeOptions);
                return stream::TempTextOutputStreamPtr{new stream::TempTextOutputStream{
                    temporaryPath, std::move(stream), temporaryOptions.removeOnClose()}};
            } catch (const PathError &error) {
                if (!isAlreadyExistsError(error)) {
                    throw;
                }
            }
        }
    } catch (const random::RandomError &) {
        throw PathError{
            PathErrorContext{"Temporary file could not be created"_el, "Secure random data is unavailable."_el},
            std::current_exception()};
    }
    throw PathError{PathErrorContext{
        "Temporary file could not be created"_el, "No unique temporary file name could be created."_el}};
}

auto PathOperations::setAccessProfile(const PathAccessProfile profile, const PathChangeOptions options) const noexcept
    -> util::Result {
    try {
        if (isEmpty()) {
            return util::Result::Failure;
        }
        return _impl->setAccessProfile(profile, options) ? util::Result::Success : util::Result::Failure;
    } catch (const err::Exception &) {
        return util::Result::Failure;
    }
}

void PathOperations::setAccessProfileOrThrow(const PathAccessProfile profile, const PathChangeOptions options) const {
    if (isEmpty()) {
        throw PathError{PathErrorContext{
            "File permissions could not be changed"_el, "No path was provided for the permission change."_el}
                .setHelp("Provide a non-empty path."_el)};
    }
    static_cast<void>(_impl->setAccessProfile(profile, options));
}

auto PathOperations::addAttributes(const PathAttributes attributes, const PathChangeOptions options) const noexcept
    -> util::Result {
    try {
        if (isEmpty()) {
            return util::Result::Failure;
        }
        return _impl->addAttributes(attributes, options) ? util::Result::Success : util::Result::Failure;
    } catch (const err::Exception &) {
        return util::Result::Failure;
    }
}

void PathOperations::addAttributesOrThrow(const PathAttributes attributes, const PathChangeOptions options) const {
    if (isEmpty()) {
        throw PathError{PathErrorContext{
            "File attributes could not be changed"_el, "No path was provided for the attribute change."_el}
                .setHelp("Provide a non-empty path."_el)};
    }
    static_cast<void>(_impl->addAttributes(attributes, options));
}

auto PathOperations::clearAttributes(const PathAttributes attributes, const PathChangeOptions options) const noexcept
    -> util::Result {
    try {
        if (isEmpty()) {
            return util::Result::Failure;
        }
        return _impl->clearAttributes(attributes, options) ? util::Result::Success : util::Result::Failure;
    } catch (const err::Exception &) {
        return util::Result::Failure;
    }
}

void PathOperations::clearAttributesOrThrow(const PathAttributes attributes, const PathChangeOptions options) const {
    if (isEmpty()) {
        throw PathError{PathErrorContext{
            "File attributes could not be changed"_el, "No path was provided for the attribute change."_el}
                .setHelp("Provide a non-empty path."_el)};
    }
    static_cast<void>(_impl->clearAttributes(attributes, options));
}

auto PathOperations::isAlreadyExistsError(const PathError &error) noexcept -> bool {
    return error.platformContext() != nullptr &&
        error.platformContext()->category() == system::PlatformErrorCategory::AlreadyExists;
}

}
