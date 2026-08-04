// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TextInputStream.hpp"
#include "TextOutputStream.hpp"

#include "impl/StandardStreamRedirectData_fwd.hpp"

#include <memory>

namespace erbsland::stream {

/// A scoped replacement for one or both process standard text streams.
/// Destroying or resetting this guard restores the stream targets that were active when the guard was created.
/// @tested{StandardStreamsTest}
class StandardStreamRedirect final {
public:
    /// Create an inactive guard.
    StandardStreamRedirect() noexcept = default;

    /// Move the standard-stream redirection into this instance.
    auto operator=(StandardStreamRedirect &&other) noexcept -> StandardStreamRedirect &;

    /// Restores the standard streams when this guard is active.
    ~StandardStreamRedirect();

    // defaults/deletions
    StandardStreamRedirect(const StandardStreamRedirect &) = delete;
    auto operator=(const StandardStreamRedirect &) -> StandardStreamRedirect & = delete;
    StandardStreamRedirect(StandardStreamRedirect &&other) noexcept;

public:
    /// Test if this guard still owns an active replacement.
    [[nodiscard]] auto isActive() const noexcept -> bool;
    /// Restore the previous stream target now.
    void reset() noexcept;

private:
    friend auto redirectStdIn(TextInputStreamPtr input) -> StandardStreamRedirect;
    friend auto redirectStdOut(TextOutputStreamPtr output) -> StandardStreamRedirect;
    friend auto redirectStdErr(TextOutputStreamPtr error) -> StandardStreamRedirect;
    friend auto redirectStandardStreams(TextOutputStreamPtr output, TextOutputStreamPtr error)
        -> StandardStreamRedirect;

    /// Create an active redirect from its shared state.
    explicit StandardStreamRedirect(std::shared_ptr<impl::StandardStreamRedirectData> data) noexcept;

private:
    std::shared_ptr<impl::StandardStreamRedirectData> _data; ///< The active redirect data.
};

}
