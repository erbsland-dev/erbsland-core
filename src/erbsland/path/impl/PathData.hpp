// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathBackend.hpp"

#include "../PathFormat.hpp"
#include "../PathWindowsFormat.hpp"

#include "../../mem/SharedData.hpp"
#include "../../mem/SharedDataPointer.hpp"
#include "../../text/String.hpp"
#include "../../text/StringEditor_fwd.hpp"
#include "../../text/StringList.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ElementCount.hpp"

#include <utility>

namespace erbsland::path::impl {

/// The shared data of the path object.
/// @tested{PathConstructionTest PathAccessTest PathModificationTest PathConversionTest}
class PathData final : public mem::SharedData {
public:
    PathData() = default;
    PathData(PathFormat format, text::String root, text::StringList elements);

    // defaults
    ~PathData() = default;
    PathData(const PathData &) = default;
    PathData(PathData &&) noexcept = default;
    auto operator=(const PathData &) -> PathData & = default;
    auto operator=(PathData &&) noexcept -> PathData & = default;

public:
    [[nodiscard]] auto isAbsolute() const noexcept -> bool;
    [[nodiscard]] auto isRoot() const noexcept -> bool;
    [[nodiscard]] auto format() const noexcept -> PathFormat;
    [[nodiscard]] auto root() const noexcept -> text::String;
    [[nodiscard]] auto elements() const noexcept -> text::StringList;
    [[nodiscard]] auto publicElementCount() const noexcept -> unit::ElementCount;
    /// Assemble the public element list, including the root if present.
    [[nodiscard]] auto publicElements() const -> text::StringList;

public:
    void setFormat(PathFormat format) noexcept;
    void setRoot(text::String root) noexcept;
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
    /// Return the non-root elements from a public element list.
    [[nodiscard]] static auto nonRootElementsFromPublicSlice(
        const text::StringList &elements, bool &sliceStartsWithRoot) -> text::StringList;
    /// Access the backend.
    [[nodiscard]] static auto backend() noexcept -> PathBackend &;

private:
    [[nodiscard]] auto windowsRootLength(PathWindowsFormat format) const noexcept -> unit::ByteLength;
    void appendWindowsRoot(text::StringEditor &result, PathWindowsFormat format) const;
    static void appendRootWithWindowsSeparators(
        text::StringEditor &result, const text::String &root, bool skipFirstCharacter);

private:
    PathFormat _format = PathFormat::Generic; ///< The determined path format.
    text::String _root;                       ///< The normalized root element, if any.
    text::StringList _elements;               ///< The path elements without the root.
};

using PathDataPtr = mem::SharedDataPointer<PathData>;

}
