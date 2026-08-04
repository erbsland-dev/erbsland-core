// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathBackend_fwd.hpp"
#include "PathInfoCache_fwd.hpp"

#include "../PathFormat.hpp"
#include "../PathWindowsFormat.hpp"

#include "../../mem/SharedData.hpp"
#include "../../mem/SharedDataPointer.hpp"
#include "../../text/String.hpp"
#include "../../text/StringEditor_fwd.hpp"
#include "../../text/StringList.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/ItemCount.hpp"

#include <atomic>
#include <memory>
#include <utility>

namespace erbsland::path::impl {

/// The shared data of the path object.
/// @tested{PathConstructionTest PathAccessTest PathModificationTest PathConversionTest}
class PathData final : public mem::SharedData {
public:
    /// Create empty generic path data.
    PathData() = default;

    /// Create shared path data from its normalized components.
    PathData(PathFormat format, text::String root, text::StringList elements, unit::CpLength characterLength);

    /// Replace this data with a copy of another path data object.
    /// This resets the info cache.
    auto operator=(const PathData &other) -> PathData &;
    /// Replace this data with another path data object.
    /// This moves the info cache to this object.
    auto operator=(PathData &&other) noexcept -> PathData &;

    // defaults
    ~PathData();
    PathData(const PathData &other);
    PathData(PathData &&other) noexcept;

public:
    /// Access the lazily created information cache for this path value.
    [[nodiscard]] auto infoCache() const -> PathInfoCache *;

public:
    /// Test if this path has an absolute root.
    [[nodiscard]] auto isAbsolute() const noexcept -> bool;
    /// Test if this path is its root.
    [[nodiscard]] auto isRoot() const noexcept -> bool;
    /// Access the normalized path format.
    [[nodiscard]] auto format() const noexcept -> PathFormat;
    /// Access the normalized root element.
    [[nodiscard]] auto root() const noexcept -> text::String;
    /// Access the normalized non-root path elements.
    [[nodiscard]] auto elements() const noexcept -> text::StringList;
    /// Get the number of public path elements.
    [[nodiscard]] auto publicItemCount() const noexcept -> unit::ItemCount;
    /// Assemble the public element list, including the root if present.
    [[nodiscard]] auto publicElements() const -> text::StringList;

public:
    /// Set the normalized path format.
    void setFormat(PathFormat format) noexcept;
    /// Set the normalized root element.
    void setRoot(text::String root) noexcept;
    /// Set the normalized non-root path elements.
    void setElements(text::StringList elements) noexcept;

public: // conversion
    /// Assemble this path with slash separators.
    [[nodiscard]] auto toString() const -> text::String;
    /// Assemble this path in Windows text format.
    [[nodiscard]] auto toWindows(PathWindowsFormat format) const -> text::String;

public:
    /// Create path data from an explicit root and non-root elements.
    /// @tparam tData Internal indirection to instantiate the shared pointer after `PathData` is complete.
    template <typename tData = PathData>
    [[nodiscard]] static auto create(PathFormat format, const text::String &root, text::StringList elements) noexcept
        -> mem::SharedDataPointer<tData>;
    /// Create path data by appending one already separated path element.
    [[nodiscard]] static auto createJoined(const PathData &base, const text::String &element) noexcept
        -> std::unique_ptr<PathData>;
    /// Return the non-root elements from a public element list.
    [[nodiscard]] static auto nonRootElementsFromPublicSlice(
        const text::StringList &elements, bool &sliceStartsWithRoot) -> text::StringList;
    /// Access the backend.
    [[nodiscard]] static auto backend() noexcept -> PathBackend &;

private:
    /// Discard the lazily generated information cache.
    void resetInfoCache() noexcept;
    /// Recalculate the cached character length.
    void updateCharacterLength() noexcept;
    /// Get the byte length of the Windows root representation.
    [[nodiscard]] auto windowsRootLength(PathWindowsFormat format) const noexcept -> unit::ByteLength;
    /// Append the Windows root representation to a string editor.
    void appendWindowsRoot(text::StringEditor &result, PathWindowsFormat format) const;
    /// Append a root with Windows separators to a string editor.
    static void appendRootWithWindowsSeparators(
        text::StringEditor &result, const text::String &root, bool skipFirstCharacter);

private:
    PathFormat _format = PathFormat::Generic;          ///< The determined path format.
    text::String _root;                                ///< The normalized root element, if any.
    text::StringList _elements;                        ///< The path elements without the root.
    unit::CpLength _characterLength;                   ///< Cached character length of the normalized path.
    mutable std::atomic<PathInfoCache *> _infoCache{}; ///< Lazily allocated path-information cache owned by this data.
};

using PathDataPtr = mem::SharedDataPointer<PathData>;

}
