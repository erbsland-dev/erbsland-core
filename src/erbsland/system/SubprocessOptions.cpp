// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SubprocessOptions.hpp"

namespace erbsland::system {

using namespace unit;

auto SubprocessOptions::maximumOutputChunkSize() const noexcept -> ByteCount {
    return _maximumOutputChunkSize;
}

auto SubprocessOptions::setMaximumOutputChunkSize(ByteCount value) noexcept -> SubprocessOptions & {
    _maximumOutputChunkSize = value;
    return *this;
}

auto SubprocessOptions::workingDirectory() const noexcept -> const std::optional<path::Path> & {
    return _workingDirectory;
}

auto SubprocessOptions::setWorkingDirectory(path::Path value) noexcept -> SubprocessOptions & {
    _workingDirectory = std::move(value);
    return *this;
}

auto SubprocessOptions::clearWorkingDirectory() noexcept -> SubprocessOptions & {
    _workingDirectory.reset();
    return *this;
}

auto SubprocessOptions::environmentChanges() const noexcept -> const EnvironmentChanges & {
    return _environmentChanges;
}

auto SubprocessOptions::environmentChanges() noexcept -> EnvironmentChanges & {
    return _environmentChanges;
}

auto SubprocessOptions::setEnvironmentChanges(EnvironmentChanges value) noexcept -> SubprocessOptions & {
    _environmentChanges = std::move(value);
    return *this;
}

auto SubprocessOptions::inheritsStandardInput() const noexcept -> bool {
    return _inheritStandardInput;
}

auto SubprocessOptions::setInheritStandardInput(const bool value) noexcept -> SubprocessOptions & {
    _inheritStandardInput = value;
    return *this;
}

auto SubprocessOptions::standardOutputMode() const noexcept -> SubprocessOutputMode {
    return _standardOutputMode;
}

auto SubprocessOptions::setStandardOutputMode(const SubprocessOutputMode value) noexcept -> SubprocessOptions & {
    _standardOutputMode = value;
    return *this;
}

auto SubprocessOptions::standardErrorMode() const noexcept -> SubprocessOutputMode {
    return _standardErrorMode;
}

auto SubprocessOptions::setStandardErrorMode(const SubprocessOutputMode value) noexcept -> SubprocessOptions & {
    _standardErrorMode = value;
    return *this;
}

auto SubprocessOptions::mergesStandardError() const noexcept -> bool {
    return _mergeStandardError;
}

auto SubprocessOptions::setMergeStandardError(const bool value) noexcept -> SubprocessOptions & {
    _mergeStandardError = value;
    return *this;
}

auto SubprocessOptions::setRemoveOutputBom(const bool value) noexcept -> SubprocessOptions & {
    _removeOutputBom = value;
    return *this;
}

auto SubprocessOptions::captureLimit() const noexcept -> ByteCount {
    return _captureLimit;
}

auto SubprocessOptions::setCaptureLimit(const ByteCount value) noexcept -> SubprocessOptions & {
    _captureLimit = value;
    return *this;
}

}
