// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FileLogWriterOptions.hpp"

namespace erbsland::log {

auto FileLogWriterOptions::setMode(const LogFileMode value) noexcept -> FileLogWriterOptions & {
    _mode = value;
    return *this;
}

auto FileLogWriterOptions::setRotation(const LogFileRotation value) noexcept -> FileLogWriterOptions & {
    _rotation = value;
    return *this;
}

auto FileLogWriterOptions::setMaximumSize(const unit::ByteLength value) noexcept -> FileLogWriterOptions & {
    _maximumSize = value;
    return *this;
}

auto FileLogWriterOptions::setRetention(const std::size_t value) noexcept -> FileLogWriterOptions & {
    _retention = value;
    return *this;
}

}
