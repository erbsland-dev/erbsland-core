// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ZipEntryRecord.hpp"

#include "../ZipError.hpp"

#include "../../../compression/CompressionAlgorithm.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../mem/ByteWriter_fwd.hpp"
#include "../../../text/String.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::compression::zip::impl::zipTools {

inline constexpr uint32_t cLocalHeaderSignature = 0x04034b50U;
inline constexpr uint32_t cDataDescriptorSignature = 0x08074b50U;
inline constexpr uint32_t cCentralHeaderSignature = 0x02014b50U;
inline constexpr uint32_t cZip64EndSignature = 0x06064b50U;
inline constexpr uint32_t cZip64LocatorSignature = 0x07064b50U;
inline constexpr uint32_t cEndSignature = 0x06054b50U;
inline constexpr uint16_t cZip64ExtraId = 0x0001U;
inline constexpr uint16_t cExtendedTimestampExtraId = 0x5455U;
inline constexpr uint16_t cUnicodeCommentExtraId = 0x6375U;
inline constexpr uint16_t cUnicodePathExtraId = 0x7075U;
inline constexpr uint16_t cUtf8Flag = 0x0800U;
inline constexpr uint16_t cDescriptorFlag = 0x0008U;

/// Normalize and validate an archive entry path.
[[nodiscard]] auto normalizeEntryPath(const mem::ByteBlock &name, bool directory) -> path::Path;
/// Convert an APPNOTE compression-method number to the public enumeration.
[[nodiscard]] auto methodFromRaw(uint16_t value) -> CompressionMethod;
/// Convert a ZIP method to the matching raw-codec algorithm.
[[nodiscard]] auto algorithmForMethod(CompressionMethod value) -> compression::CompressionAlgorithm;
/// Get the minimum APPNOTE version-needed value for an entry.
[[nodiscard]] auto minimumVersion(CompressionMethod value, bool zip64) noexcept -> uint16_t;
/// Test two normalized paths for duplicate or file-parent conflicts.
[[nodiscard]] auto pathsConflict(
    const path::Path &left, bool leftDirectory, const path::Path &right, bool rightDirectory) noexcept -> bool;
/// Convert the mandatory DOS fields to a UTC timestamp.
[[nodiscard]] auto dateTimeFromDos(uint16_t date, uint16_t time) noexcept -> time::DateTime;
/// Convert a UTC timestamp to the mandatory DOS fields.
void dateTimeToDos(const time::DateTime &dateTime, uint16_t &date, uint16_t &time) noexcept;
/// Parse Extended Timestamp and retain every unmanaged extra field.
[[nodiscard]] auto modificationTimeFromExtra(
    const mem::ByteBlock &extra, time::DateTime fallback, mem::ByteWriter *opaque = nullptr) -> time::DateTime;
/// Append the managed Extended Timestamp modification-time field.
void appendExtendedTimestamp(mem::ByteWriter &extra, const time::DateTime &dateTime);
/// Match a normalized ZIP path against the documented `*`, `?`, and `**` syntax.
[[nodiscard]] auto globMatches(const text::String &pattern, const text::String &value) noexcept -> bool;

/// Throw a malformed-archive error with shared context.
[[noreturn]] void throwMalformed(text::String description);
/// Continue recursive glob matching at the supplied code-point positions.
[[nodiscard]] auto globMatchesAt(
    const text::String &pattern, std::size_t patternIndex, const text::String &value, std::size_t valueIndex) noexcept
    -> bool;
/// Test whether one normalized path component is portable across supported hosts.
[[nodiscard]] auto isPortableComponent(const text::String &component) noexcept -> bool;
/// Test whether one normalized path is a strict component prefix of another.
[[nodiscard]] auto isPathPrefix(const path::Path &prefix, const path::Path &path) noexcept -> bool;

}
