// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Path_fwd.hpp"
#include "PathReadDataOptions.hpp"
#include "PathReadTextOptions.hpp"
#include "PathWriteDataOptions.hpp"
#include "PathWriteTextOptions.hpp"

#include "impl/PathContent_fwd.hpp"

#include "../stream/ByteInputStream_fwd.hpp"
#include "../stream/ByteOutputStream_fwd.hpp"
#include "../stream/TextInputStream_fwd.hpp"
#include "../stream/TextOutputStream_fwd.hpp"
#include "../text/String.hpp"
#include "../text/StringView.hpp"
#include "../unit/ByteLength.hpp"
#include "../util/Result.hpp"

namespace erbsland::path {

/// Allows access to the content of a path.
/// @tested{PathContentTest}
class PathContent final {
public:
    /// Create an empty instance.
    PathContent();
    /// Create a new instance for the given path.
    explicit PathContent(const Path &path);

    // defaults
    ~PathContent();
    PathContent(const PathContent &) = delete;
    PathContent(PathContent &&) noexcept;
    auto operator=(const PathContent &) -> PathContent & = delete;
    auto operator=(PathContent &&) noexcept -> PathContent &;

public: // attributes
    /// Test if the path is empty.
    [[nodiscard]] auto isEmpty() const -> bool;
    /// Access the underlying path.
    [[nodiscard]] auto path() const -> const Path &;

public: // content methods
    /// Read the contents of a file into a string.
    /// If encoding error mode is set to `Throw`, this function returns no string instead of throwing.
    /// @param options The read options.
    /// @return The string read from the file or std::nullopt on any error.
    [[nodiscard]] auto readText(PathReadTextOptions options = {}) const noexcept -> std::optional<text::String>;
    /// Read the contents of a file into a string.
    /// @param options The read options.
    /// @return The string read from the file.
    /// @throws PathError if the operation failed (no file, access errors, etc.)
    /// @throws text::EncodingError if the operation failed due to encoding errors.
    /// @throws err::OutOfRangeError if the file exceeds a set maximum length.
    [[nodiscard]] auto readTextOrThrow(PathReadTextOptions options = {}) const -> text::String;
    /// Read the contents of a file as byte data.
    /// @param options The read options.
    /// @return The byte block read from the file or std::nullopt on any error.
    [[nodiscard]] auto readData(PathReadDataOptions options = {}) const noexcept -> std::optional<mem::ByteBlock>;
    /// Read the contents of a file as byte data.
    /// @param options The read options.
    /// @return The byte block read from the file.
    /// @throws PathError if the operation failed (no file, access errors, etc.)
    /// @throws err::OutOfRangeError if the file exceeds a set maximum length.
    [[nodiscard]] auto readDataOrThrow(PathReadDataOptions options = {}) const -> mem::ByteBlock;
    /// Write text into the file at this path.
    /// @param text The text to write.
    /// @param options The options to use.
    /// @return True on success, false on error.
    auto writeText(const text::StringView &text, PathWriteTextOptions options = {}) const noexcept -> util::Result;
    /// Write text into the file at this path.
    /// @param text The text to write.
    /// @param options The options to use.
    /// @throws PathError if the operation failed (no file, access errors, etc.)
    /// @throws text::EncodingError if the operation failed due to encoding errors.
    void writeTextOrThrow(const text::StringView &text, PathWriteTextOptions options = {}) const;
    /// Write byte data into a file at this path.
    /// @param options The options to use.
    /// @param data The data to write.
    /// @return True on success, false on error.
    auto writeData(const mem::ByteBlock &data, PathWriteDataOptions options = {}) const noexcept -> util::Result;
    /// Write byte data into a file at this path.
    /// @param options The options to use.
    /// @param data The data to write.
    /// @throws PathError if the operation failed (no file, access errors, etc.)
    void writeDataOrThrow(const mem::ByteBlock &data, PathWriteDataOptions options = {}) const;
    /// Open a file as a text stream for reading.
    /// The read limits (bytes and code-points) are ignored when reading from a stream.
    /// @param options The options to use.
    /// @returns The open stream.
    /// @throws PathError if the operation failed (no file, access errors, etc.)
    [[nodiscard]] auto openTextInputStream(PathReadTextOptions options = {}) const -> stream::TextInputStreamPtr;
    /// Open a file as a text stream for writing.
    /// @param options The options to use.
    /// @returns The open stream.
    /// @throws PathError if the operation failed (no file, access errors, etc.)
    [[nodiscard]] auto openTextOutputStream(PathWriteTextOptions options = {}) const -> stream::TextOutputStreamPtr;
    /// Open a file as byte data for reading.
    /// @param options The options to use.
    /// @returns The open stream.
    /// @throws PathError if the operation failed (no file, access errors, etc.)
    [[nodiscard]] auto openByteInputStream(PathReadDataOptions options = {}) const -> stream::ByteInputStreamPtr;
    /// Open a file as byte data for writing.
    /// @param options The options to use.
    /// @returns The open stream.
    /// @throws PathError if the operation failed (no file, access errors, etc.)
    [[nodiscard]] auto openByteOutputStream(PathWriteDataOptions options = {}) const -> stream::ByteOutputStreamPtr;

private:
    impl::PathContentImplPtr _impl;
};

}
