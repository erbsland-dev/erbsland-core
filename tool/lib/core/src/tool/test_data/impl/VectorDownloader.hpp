// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteBlock.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/HttpClientSession.hpp>
#include <erbsland/Url.hpp>

#include <exception>

namespace tool::test_data::impl {

/// Download verified-source candidates using one native HTTP session.
class VectorDownloader final {
public:
    /// Create a session with bounded response storage and an overall deadline.
    /// @param maximumBytes Maximum bytes in one response.
    /// @param timeout Overall deadline for each source.
    VectorDownloader(el::ByteCount maximumBytes, el::TimeDelta timeout);
    /// Abort outstanding activity and drain queued shutdown work on the owner loop.
    ~VectorDownloader();

public: // downloads
    /// Fetch one source, preserving URL and transport details on failure.
    /// @param url The validated absolute HTTP or HTTPS URL.
    /// @return The response body after a successful HTTP status.
    /// @throws el::ApplicationError If downloading fails.
    [[nodiscard]] auto download(const el::Url &url) -> el::ByteBlock;

private:
    /// Shared completion state survives queued transport callbacks on exceptional exits.
    struct Download {
        el::ByteBlock data;
        std::exception_ptr error;
        bool finished{};
    };

private:
    el::EventLoopPtr _loop;            ///< Owner loop driven on the command thread.
    el::HttpClientSessionPtr _session; ///< Session reused across sequential downloads.
    el::ByteCount _maximumBytes;       ///< Bounded response length.
};

}
