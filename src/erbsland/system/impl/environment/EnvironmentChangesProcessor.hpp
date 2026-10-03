// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EnvironmentChangesProcessor_fwd.hpp"
#include "EnvironmentPaths.hpp"
#include "EnvironmentVariableBackend.hpp"

#include "../../../text/CharSet.hpp"
#include "../../../text/impl/StringSplitter.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringConverter.hpp"
#include "../../../unit/ItemCount.hpp"
#include "../../../unit/ItemIndex.hpp"
#include "../../EnvironmentChanges.hpp"

#include <map>
#include <type_traits>

namespace erbsland::system::impl {

/// Shared environment transformation for UTF-8 application maps and native UTF-16 launch maps.
/// @tested{EnvironmentChangesTest EnvironmentSubprocessInteropTest}
class EnvironmentChangesProcessor final {
public:
    /// Order names with the selected platform's comparison rules.
    /// @tparam T The native Core string type.
    template <typename T>
    struct NameLess {
        std::reference_wrapper<const EnvironmentVariableBackend> backend =
            EnvironmentVariableBackend::native(); ///< Native platform policy.
        /// Order two native variable names under this policy.
        /// @param left The first name.
        /// @param right The second name.
        /// @return Whether the first name sorts before the second.
        auto operator()(const T &left, const T &right) const -> bool { return backend.get().compare(left, right) < 0; }
    };
    /// A native string map that preserves untouched code units.
    /// @tparam T The native Core string type.
    template <typename T>
    using Environment = std::map<T, T, NameLess<T>>;

public: // application
    /// Apply a descriptor without converting inherited names or values.
    /// @tparam T The native Core string type.
    /// @param changes The declarative changes.
    /// @param base The source map, whose comparator supplies the platform policy.
    /// @return The transformed native map.
    template <typename T>
    [[nodiscard]] static auto apply(const EnvironmentChanges &changes, const Environment<T> &base) -> Environment<T>;

private:
    /// Encode explicitly configured text in the native string type.
    template <typename T>
    [[nodiscard]] static auto encode(const text::String &value) -> T;
    /// Decode one native value without replacing malformed code units.
    template <typename T>
    [[nodiscard]] static auto decode(const T &value) -> std::optional<text::String>;
    /// Apply PATH filtering, additions, and stable deduplication.
    template <typename T>
    static void applyPath(const EnvironmentChanges &changes, Environment<T> &environment);
};

}

#include "EnvironmentChangesProcessor.tpp"
