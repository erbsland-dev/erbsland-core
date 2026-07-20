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
using err::Exception;
using util::Result;

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

auto PathOperations::remove(const PathRemoveOptions options, const PathProgressFn &progressFn) noexcept -> Result {
    try {
        if (isEmpty()) {
            return Result::Failure;
        }
        _impl->removeOrThrow(options, progressFn);
        return Result::Success;
    } catch (const Exception &) {
        return Result::Failure;
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
    const Path &destination, const PathCopyOptions options, const PathProgressFn &progressFn) const noexcept -> Result {
    try {
        if (isEmpty()) {
            return Result::Failure;
        }
        _impl->copyToOrThrow(destination, options, progressFn);
        return Result::Success;
    } catch (const Exception &) {
        return Result::Failure;
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

auto PathOperations::moveTo(const Path &destination, const PathMoveOptions options) const noexcept -> Result {
    try {
        if (isEmpty()) {
            return Result::Failure;
        }
        _impl->moveToOrThrow(destination, options);
        return Result::Success;
    } catch (const Exception &) {
        return Result::Failure;
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

auto PathOperations::createFile(const PathCreateFileOptions options) const noexcept -> Result {
    try {
        createFileOrThrow(options);
        return Result::Success;
    } catch (const Exception &) {
        return Result::Failure;
    }
}

void PathOperations::createFileOrThrow(const PathCreateFileOptions options) const {
    if (isEmpty()) {
        throw PathError{"File creation has no path"_el};
    }
    _impl->createFileOrThrow(options);
}

auto PathOperations::createDirectory(const PathCreateDirectoryOptions options) const noexcept -> Result {
    try {
        createDirectoryOrThrow(options);
        return Result::Success;
    } catch (const Exception &) {
        return Result::Failure;
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
    } catch (const Exception &) {
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
    const auto probeText = text::String::fromJoined({options.prefix(), "x"_el, options.suffix()});
    const auto nameProbe = Path{probeText};
    if (nameProbe.isEmpty() || nameProbe.isAbsolute() || nameProbe.elementCount() != unit::ElementCount::one()) {
        throw PathError{PathErrorContext{
            "Temporary directory could not be created"_el, "The prefix and suffix must form one valid path name."_el}};
    }
    try {
        auto &random = core::application().secureRandom();
        static const auto alphabet = text::CharSet{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"_el};
        for (auto attempt = unit::ElementCount{}; attempt < options.maximumAttempts(); ++attempt) {
            const auto name = text::String::fromJoined(
                {options.prefix(), random.buildString(options.randomLength(), alphabet), options.suffix()});
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
    } catch (const Exception &) {
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
    const auto probeText = text::String::fromJoined({options.prefix(), "x"_el, options.suffix()});
    const auto nameProbe = Path{probeText};
    if (nameProbe.isEmpty() || nameProbe.isAbsolute() || nameProbe.elementCount() != unit::ElementCount::one()) {
        throw PathError{PathErrorContext{
            "Temporary file could not be created"_el, "The prefix and suffix must form one valid path name."_el}};
    }
    try {
        auto &random = core::application().secureRandom();
        static const auto alphabet = text::CharSet{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"_el};
        for (auto attempt = unit::ElementCount{}; attempt < options.maximumAttempts(); ++attempt) {
            const auto name = text::String::fromJoined(
                {options.prefix(), random.buildString(options.randomLength(), alphabet), options.suffix()});
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
    } catch (const Exception &) {
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
    const auto probeText = text::String::fromJoined({temporaryOptions.prefix(), "x"_el, temporaryOptions.suffix()});
    const auto nameProbe = Path{probeText};
    if (nameProbe.isEmpty() || nameProbe.isAbsolute() || nameProbe.elementCount() != unit::ElementCount::one()) {
        throw PathError{PathErrorContext{
            "Temporary file could not be created"_el, "The prefix and suffix must form one valid path name."_el}};
    }
    try {
        auto &random = core::application().secureRandom();
        static const auto alphabet = text::CharSet{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"_el};
        for (auto attempt = unit::ElementCount{}; attempt < temporaryOptions.maximumAttempts(); ++attempt) {
            const auto name = text::String::fromJoined(
                {temporaryOptions.prefix(),
                    random.buildString(temporaryOptions.randomLength(), alphabet),
                    temporaryOptions.suffix()});
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
    -> Result {
    try {
        if (isEmpty()) {
            return Result::Failure;
        }
        return _impl->setAccessProfile(profile, options) ? Result::Success : Result::Failure;
    } catch (const Exception &) {
        return Result::Failure;
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
    -> Result {
    try {
        if (isEmpty()) {
            return Result::Failure;
        }
        return _impl->addAttributes(attributes, options) ? Result::Success : Result::Failure;
    } catch (const Exception &) {
        return Result::Failure;
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
    -> Result {
    try {
        if (isEmpty()) {
            return Result::Failure;
        }
        return _impl->clearAttributes(attributes, options) ? Result::Success : Result::Failure;
    } catch (const Exception &) {
        return Result::Failure;
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
