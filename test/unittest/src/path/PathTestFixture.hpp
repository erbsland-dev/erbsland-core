// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathTestHelper.hpp"

#include <chrono>
#include <filesystem>
#include <string>

namespace erbsland::test::pathtest {

/// Temporary filesystem fixture for path unit tests.
/// @notest{Used only by path unit tests.}
class PathTestFixture final {
public:
    /// Create a fresh temporary directory for one named test.
    explicit PathTestFixture(const std::string &name) : _path{createPath(name)} {
        std::filesystem::remove_all(_path);
        std::filesystem::create_directories(_path);
    }
    /// Remove the temporary directory and its contents.
    ~PathTestFixture() { std::filesystem::remove_all(_path); }

    // defaults
    PathTestFixture(const PathTestFixture &) = delete;
    PathTestFixture(PathTestFixture &&) = delete;
    auto operator=(const PathTestFixture &) -> PathTestFixture & = delete;
    auto operator=(PathTestFixture &&) -> PathTestFixture & = delete;

public:
    /// Access the temporary directory as a native filesystem path.
    [[nodiscard]] auto stdPath() const noexcept -> const std::filesystem::path & { return _path; }
    /// Access the temporary directory as an Erbsland Core path.
    [[nodiscard]] auto path() const -> el::path::Path { return el::path::Path{_path}; }
    /// Get a fixture path below this fixture's root.
    [[nodiscard]] auto child(const std::filesystem::path &suffix) const -> el::path::Path {
        return el::path::Path{_path / suffix};
    }

private:
    /// Create a native fixture path for a name.
    [[nodiscard]] static auto createPath(const std::string &name) -> std::filesystem::path {
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        return std::filesystem::temp_directory_path() / ("erbsland-core-path-" + name + "-" + std::to_string(now));
    }

private:
    std::filesystem::path _path;
};

}
