// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/text/Literals.hpp>
#include <erbsland/text/render/Loader.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/text/StringMap.hpp>
#include <erbsland/text/ToString.hpp>

#include <mutex>

namespace erbsland::test {

/// A deterministic, thread-safe in-memory layout loader for renderer tests.
/// @notest{Test utility.}
class MemoryLayoutLoader final : public erbsland::text::render::Loader {
public:
    /// Add or replace a layout and increment its revision.
    void set(const erbsland::text::String &name, erbsland::text::String source) {
        using namespace erbsland::text::literals;
        std::unique_lock lock{_mutex};
        ++_revision;
        _layouts.set(
            name,
            std::make_shared<erbsland::text::render::LayoutSource>(
                std::move(source),
                erbsland::text::StringFormat{"memory:{}"_el}.build(name),
                erbsland::text::toString(static_cast<uint64_t>(_revision))));
    }

    /// Remove a layout.
    void remove(const erbsland::text::String &name) {
        std::unique_lock lock{_mutex};
        _layouts.remove(name);
    }

    /// Get the total number of load calls.
    [[nodiscard]] auto loadCount() const noexcept -> std::size_t {
        std::unique_lock lock{_mutex};
        return _loadCount;
    }

public: // implement Loader
    [[nodiscard]] auto load(const erbsland::text::String &layout)
        -> std::optional<erbsland::text::render::LayoutSource> override {
        std::unique_lock lock{_mutex};
        ++_loadCount;
        const auto source = _layouts.get(layout);
        if (!source.has_value()) {
            return std::nullopt;
        }
        return **source;
    }

private:
    mutable std::mutex _mutex;                                                                 ///< Protects all state.
    erbsland::text::StringMap<std::shared_ptr<erbsland::text::render::LayoutSource>> _layouts; ///< Current sources.
    std::size_t _revision{0U};  ///< Monotonic revision source.
    std::size_t _loadCount{0U}; ///< Number of load calls.
};

}
